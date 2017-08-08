#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define LAMP      (1 << 0)
#define BUTTON    (1 << 1)

#define SEC_MICRO 1000000UL
#define DAY_SEC 86400UL

/* Order of parts matters, otherwise the operation is not
 * done right due to 32bit limit. I think. */
#define TIMER0_OVF_period \
    (((SEC_MICRO * 256UL) / F_CPU) * 256UL)

struct time {
	uint32_t u, s, d;
};

volatile struct time time = {0};

/* Time lamp should stay on for alarm in minutes.
 * Due to uint32_t constraint on times this can be
 * no longer than 70 minutes. */
 
#define ALARM_LENGTH  10

uint32_t alarm = ((12 * 60) + 30) * 60UL;
volatile bool alarmed_today = false;

/* Debounced button signal. */
volatile bool button_down = false;

volatile uint8_t lamp_brightness = 0;


typedef enum {
	STATE_wait,
	STATE_alarm,
	STATE_on,
	STATE_set_alarm,
	STATE_set_time,
	STATE_button_down,
} state_t;

void
init_timers(void)
{
	TCCR1 |= (1<<CS13);
		
	TCCR0B |= (1<<CS02);
	TCNT0 = 0;
	
	TIMSK |= (1<<TOIE0);
}

ISR(TIMER0_OVF_vect)
{
	static uint32_t button_count = 0;
	
	/* Time keeping. */
	time.u += TIMER0_OVF_period;
	
	if (time.u >= SEC_MICRO) {
		time.u -= SEC_MICRO;
		time.s++;
		
		if (time.s >= DAY_SEC) {
			time.s -= DAY_SEC;
			time.d++;
			alarmed_today = false;
		}
	}
	
	/* Button debouncing. */
	if ((PINB & BUTTON) && button_down) {
		if (button_count++ == 3) {
			button_down = false;
		}
	} else if ((PINB & BUTTON) == 0 && !button_down) {
		if (button_count++ == 3) {
			button_down = true;
		}
	} else {
		button_count = 0;
	}
}

ISR(TIMER1_OVF_vect)
{
	PORTB |= LAMP;
	OCR1A = lamp_brightness;
}

ISR(TIMER1_COMPA_vect)
{
	PORTB &= ~LAMP;
}

void
set_lamp_brightness(uint8_t b)
{
	lamp_brightness = b;
	
	if (b == 0) {
		PORTB &= ~LAMP;
		TIMSK &= ~((1<<TOIE1)|(1<<OCIE1A));
		
	} else if (b == 0xff) {
		PORTB |= LAMP;
		TIMSK &= ~((1<<TOIE1)|(1<<OCIE1A));
		
	} else {
		TIMSK |= (1<<TOIE1)|(1<<OCIE1A);
	}
}

uint8_t
get_lamp_brightness(void)
{
	return lamp_brightness;
}

uint32_t
time_diff(struct time *s)
{
	return ((time.d - s->d) * DAY_SEC + (time.s - s->s)) * SEC_MICRO
	        + (time.u - s->u);
}

void
get_time(struct time *dest)
{
	dest->d = time.d;
	dest->s = time.s;
	dest->u = time.u;
}

state_t
button_down_cancel(void)
{
	set_lamp_brightness(0);
	
	while (button_down)
		;
		
	return STATE_wait;
}

state_t
state_on(void)
{
	set_lamp_brightness(0xff);
	
	while (!button_down)
		;
	
	return button_down_cancel();
}

state_t
state_alarm(void)
{
	uint32_t lt, t;
	struct time s;
	
	lt = 0;
	get_time(&s);
	
	alarmed_today = true;
	
	set_lamp_brightness(0);
	
	/* Fade on. */
	
	lt = 0;
	while (get_lamp_brightness() < 0xff) {
		if (button_down) {
			return button_down_cancel();
		}
		
		t = time_diff(&s);
		if (t > lt) {
			lt = t + SEC_MICRO;
			set_lamp_brightness(get_lamp_brightness() + 1);
		}
	}
	
	/* Full for some time. */
	do {
		if (button_down) {
			return button_down_cancel();
		}
		
		t = time_diff(&s);
	} while (t < ALARM_LENGTH * 60 * SEC_MICRO);
	
	/* Fade off. */
	
	lt = 0;
	while (get_lamp_brightness() > 0) {
		if (button_down) {
			return button_down_cancel();
		}
		
		t = time_diff(&s);
		if (t > lt) {
			lt = t + SEC_MICRO;
			set_lamp_brightness(get_lamp_brightness() - 1);
		}
	}
		
	return STATE_wait;
}

state_t
state_button_down(void)
{
	uint32_t lt, t;
	struct time s;
	
	lt = 0;
	get_time(&s);
	
	/* Goto fade. */
	set_lamp_brightness(0xff);
	do {
		if (!button_down) {
			return STATE_on;
		}
		
		t = time_diff(&s);
	} while (t < 1 * SEC_MICRO);
	
	/* Cancel. */
	set_lamp_brightness(0);
	do {
		if (!button_down) {
			return STATE_wait;
		}
		
		t = time_diff(&s);
	} while (t < 2 * SEC_MICRO);
	
	/* Set alarm. */
	lt = 0;
	do {
		if (t > lt) {
			lt = t + SEC_MICRO / 4;
			set_lamp_brightness(
			    get_lamp_brightness() > 0 ? 0 : 0xff);
		}
	
		if (!button_down) {
			return STATE_set_alarm;
		}
		
		t = time_diff(&s);
	} while (t < 4 * SEC_MICRO);
	
	/* Set time. */
	lt = 0;
	do {
		if (t > lt) {
			lt = t + SEC_MICRO / 8;
			set_lamp_brightness(
			    get_lamp_brightness() > 0 ? 0 : 0xff);
		}
	
		if (!button_down) {
			return STATE_set_time;
		}
		
		t = time_diff(&s);
	} while (t < 6 * SEC_MICRO);
	
	/* Cancel. */
	set_lamp_brightness(0);
	while (button_down)
		;
	
	return STATE_wait;
}

int
get_hour_minutes_button_down(int rate)
{
	uint32_t lt, t;
	struct time s;
	
	lt = 0;
	get_time(&s);
	
	/* Short. */
	do {
		if (!button_down) {
			return 0;
		}
		
		t = time_diff(&s);
		if (t > lt) {
			lt = t + SEC_MICRO / rate;
			
			/* Should flash display instead of lamp. */
			set_lamp_brightness(
			    get_lamp_brightness() > 0 ? 0 : 0xff);
		}
	} while (t < 1 * SEC_MICRO);
	
	/* Should set display to full on. */
	set_lamp_brightness(0xff);
	
	/* Long. */
	do {
		if (!button_down) {
			return 1;
		}
		
		t = time_diff(&s);
	} while (t < 5 * SEC_MICRO);
	
	/* Cancel */
	
	/* Should turn display off. */
	set_lamp_brightness(0);
	while (button_down)
		;
	
	return 2;
}

bool
get_hour_minutes_value(uint8_t *v, uint8_t rate, bool ishour)
{
	uint32_t lt, t;
	struct time s;
	
	lt = 0;
	get_time(&s);
	*v = 0;
	
	while (true) {
		if (button_down) {
			switch (get_hour_minutes_button_down(rate)) {
			case 0:
				*v = (*v + 1) % (ishour ? 24 : 60);
				break;
			case 1:
				return true;
			case 2:
				return false;
			}
			
			get_time(&s);
			lt = 0;
		}
		
		t = time_diff(&s);
		if (t > lt) {
			lt = t + SEC_MICRO / rate;
			
			/* Should flash display instead of lamp.
			 * Either high two if hour is true or low two. */
			set_lamp_brightness(
			    get_lamp_brightness() > 0 ? 0 : 0xff);
			    
		} else if (t > 60 * SEC_MICRO) {
			return false;
		}
	}
}

bool
get_hour_minutes(uint8_t *hour, uint8_t *minute, uint8_t rate)
{
	if (!get_hour_minutes_value(hour, rate, true)) {
		return false;
	}
	
	return get_hour_minutes_value(minute, rate, false);
}

state_t
state_set_alarm(void)
{
	uint8_t hour, minute;
	
	if (get_hour_minutes(&hour, &minute, 4)) {
		alarm = (hour * 60 + minute) * 60;
		if (alarm >= time.s) {
			alarmed_today = true;
		} else {
			alarmed_today = false;
		}
	}
	
	return STATE_wait;
}

state_t
state_set_time(void)
{
	uint8_t hour, minute;
	
	if (get_hour_minutes(&hour, &minute, 8)) {
		time.d = 0;
		time.s = (hour * 60 + minute) * 60;
		time.u = 0;
	}
	
	return STATE_wait;
}

state_t
state_wait(void)
{
	/* Last alarm was at least 23 hours ago and it is
	 * past alarm time. */ 
	if (!alarmed_today && time.s >= alarm) {
		return STATE_alarm;
		
	} else if (button_down) {
		return STATE_button_down;
	
	} else {
		set_lamp_brightness(0);
	
		return STATE_wait;
	}
}

state_t ((*states[])(void)) = {
	[STATE_wait]        = state_wait,
	[STATE_alarm]       = state_alarm,
	[STATE_on]          = state_on,
	[STATE_set_alarm]   = state_set_alarm,
	[STATE_set_time]    = state_set_time,
	[STATE_button_down] = state_button_down,
};

int
main(void)
{
	state_t s;
	
	DDRB |= LAMP;
	PORTB |= BUTTON;
	s = STATE_wait;
	
	init_timers();
	
	sei();
	while (true) {
		s = states[s]();
	}
}
