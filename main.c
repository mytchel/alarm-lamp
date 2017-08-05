#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>

#define LAMP      (1 << 0)

#define BUTTON    (1 << 1)

/* Time in seconds that fade should have the lamp full. */
#define FADE_FULL_TIME  1

volatile uint32_t time_u = 0;
volatile uint32_t time_s = 0;

/* Time lamp should stay on for alarm in minutes.
 * Due to uint32_t constraint on times this can be
 * no longer than 70 minutes. */
#define ALARM_LENGTH  30
uint32_t alarm = 0xffffffff;

volatile bool button_down = false;

#define SEC_MICRO 1000000UL

/* Order of parts matters, otherwise the operation is not
 * done right due to 32bit limit. I think. */
#define TIMER1_OVF_period \
    (((SEC_MICRO * 256UL) / F_CPU) * 256UL)


typedef enum {
	STATE_wait,
	STATE_alarm,
	STATE_fade,
	STATE_set_alarm,
	STATE_set_time,
	STATE_button_down,
} state_t;

void
init_timers(void)
{
	TCCR1 |= (1<<CS13);
		
	TCCR0B |= (1<<CS02)|(1<<CS00);
	TCNT0 = 0;
	
	TIMSK |= (1<<TOIE0);
}

ISR(TIMER0_OVF_vect)
{
	static uint32_t button_count = 0;
	
	/* Time keeping. */
	time_u += TIMER1_OVF_period;
	if (time_u >= SEC_MICRO) {
		time_s++;
		time_u -= SEC_MICRO;
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
}

ISR(TIMER1_COMPA_vect)
{
	PORTB &= ~LAMP;
}

void
set_lamp_brightness(uint8_t b)
{
	uint8_t mask = TIMSK;
	
	if (b == 0) {
		PORTB &= ~LAMP;
		mask &= ~((1<<TOIE1)|(1<<OCIE1A));
		
	} else if (b == 0xff) {
		PORTB |= LAMP;
		mask &= ~((1<<TOIE1)|(1<<OCIE1A));
		
	} else {
		TCNT1 = 0;
		mask |= (1<<TOIE1)|(1<<OCIE1A);
	}
	
	OCR1A = b;
	TIMSK = mask;
}

uint8_t
get_lamp_brightness(void)
{
	return OCR1A;
}

uint32_t
time_diff(uint32_t ts_u, uint32_t ts_s,
          uint32_t te_u, uint32_t te_s)
{
	return (te_s - ts_s) * SEC_MICRO + (te_u - ts_u);
}

state_t
state_fade(void)
{
	uint32_t ts_u, ts_s, t, lt;
	
	ts_u = time_u;
	ts_s = time_s;
	
	set_lamp_brightness(0xff);
	
	/* Full for some time. */
	do {
		if (button_down) {
			return STATE_button_down;
		}
		
		t = time_diff(ts_u, ts_s, time_u, time_s);
	} while (t < FADE_FULL_TIME * SEC_MICRO);
	
	/* Fade away. */
	
	lt = 0;
	while (get_lamp_brightness() > 0) {
		if (button_down) {
			return STATE_button_down;
		}
		
		t = time_diff(ts_u, ts_s, time_u, time_s);
		if (t > lt) {
			lt = t + SEC_MICRO / 128;
			set_lamp_brightness(get_lamp_brightness() - 1);
		}
	}
	
	return STATE_wait;
}

state_t
state_alarm(void)
{
	uint32_t ts_u, ts_s, t, lt;
	
	ts_u = time_u;
	ts_s = time_s;
	
	set_lamp_brightness(0);
	
	/* Fade on. */
	
	lt = 0;
	while (get_lamp_brightness() < 0xff) {
		if (button_down) {
			return STATE_button_down;
		}
		
		t = time_diff(ts_u, ts_s, time_u, time_s);
		if (t > lt) {
			lt = t + SEC_MICRO;
			set_lamp_brightness(get_lamp_brightness() + 1);
		}
	}
	
	/* Full for some time. */
	do {
		if (button_down) {
			return STATE_button_down;
		}
		
		t = time_diff(ts_u, ts_s, time_u, time_s);
	} while (t < ALARM_LENGTH * 60 * SEC_MICRO);
	
	/* Fade off. */
	
	lt = 0;
	while (get_lamp_brightness() > 0) {
		if (button_down) {
			return STATE_button_down;
		}
		
		t = time_diff(ts_u, ts_s, time_u, time_s);
		if (t > lt) {
			lt = t + SEC_MICRO;
			set_lamp_brightness(get_lamp_brightness() - 1);
		}
	}
		
	return STATE_wait;
}

state_t
state_set_alarm(void)
{
	set_lamp_brightness(0);
	
	alarm = time_s + 20;
	
	return STATE_wait;
}

state_t
state_set_time(void)
{
	set_lamp_brightness(0);
	return STATE_wait;
}

state_t
state_button_down(void)
{
	uint32_t ts_u, ts_s, t, lt;
	
	t = 0;
	ts_u = time_u;
	ts_s = time_s;
	
	/* Do nothing. */
	do {
		if (!button_down) {
			return STATE_wait;
		}	
	
		t = time_diff(ts_u, ts_s, time_u, time_s);
	} while (t <= 0.1 * SEC_MICRO);
	
	/* Goto fade. */
	set_lamp_brightness(0xff);
	do {
		if (!button_down) {
			return STATE_fade;
		}
		
		t = time_diff(ts_u, ts_s, time_u, time_s);
	} while (t <= 1 * SEC_MICRO);
	
	/* Cancel. */
	set_lamp_brightness(0);
	do {
		if (!button_down) {
			return STATE_wait;
		}
		
		t = time_diff(ts_u, ts_s, time_u, time_s);
	} while (t <= 2 * SEC_MICRO);
	
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
		
		t = time_diff(ts_u, ts_s, time_u, time_s);
	} while (t <= 4 * SEC_MICRO);
	
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
		
		t = time_diff(ts_u, ts_s, time_u, time_s);
	} while (t <= 6 * SEC_MICRO);
	
	/* Cancel. */
	set_lamp_brightness(0);
	while (button_down)
		;
	
	return STATE_wait;
}

state_t
state_wait(void)
{
	if (time_s > alarm) {
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
	[STATE_fade]        = state_fade,
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
	
	init_timers();
	
	sei();
	s = STATE_fade;
	while (true) {
		s = states[s]();
	}
}
