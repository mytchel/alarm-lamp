#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "lamp.h"

#define PIN_LAMP    1
#define PIN_BUTTON  4
#define PIN_DIO     0
#define PIN_DCK     2
#define PIN_CSDA    7
#define PIN_CSCL    7

#define LAMP      (1 << PIN_LAMP)
#define BUTTON    (1 << PIN_BUTTON)

#define ALARM_LENGTH 30

#define SEC_MICRO 1000000UL

#define BUTTON_DEBOUNCE 30

/* Order of parts matters, otherwise the operation is not
 * done right due to 32bit limit. I think. */
#define TIMER0_OVF_period \
    (((SEC_MICRO * 256UL) / F_CPU) * 1UL)

struct st {
	uint32_t u, s;
};

volatile struct st st = {0};

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
	STATE_button_down_cancel,
} state_t;

void
init_timers(void)
{
	TCCR1 |= (1<<CS13);
		
	TCCR0B |= (1<<CS00);
	TCNT0 = 0;
	
	TIMSK |= (1<<TOIE0);
}

ISR(TIMER0_OVF_vect)
{
	static uint32_t button_count = 0;
	
	/* Time keeping. */
	st.u += TIMER0_OVF_period;
	
	if (st.u >= SEC_MICRO) {
		st.u -= SEC_MICRO;
		st.s++;
	}
	
	/* Button debouncing. */
	if ((PINB & BUTTON) && button_down) {
		if (button_count++ == BUTTON_DEBOUNCE) {
			button_down = false;
		}
	} else if ((PINB & BUTTON) == 0 && !button_down) {
		if (button_count++ == BUTTON_DEBOUNCE) {
			button_down = true;
		}
	} else {
		button_count = 0;
	}
}

uint32_t
st_diff(struct st *s)
{
	return (st.s - s->s) * SEC_MICRO
	        + (st.u - s->u);
}

void
init_st(struct st *dest)
{
	dest->s = st.s;
	dest->u = st.u;
}

void
delay(uint32_t t)
{
	struct st s;
	
	init_st(&s);
	while (st_diff(&s) < t)
		;
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

state_t
state_button_down_cancel(void)
{
	set_lamp_brightness(0);
	
	return button_down ?
	    STATE_button_down_cancel : STATE_wait;
}

state_t
state_on(void)
{
	uint8_t h, m;
	
	set_lamp_brightness(0xff);
	
	get_time(&h, &m);
	
	set_display_state(true);
	display_draw(true, 
	             h / 10, h % 10,
	             m / 10, m % 10);
	
	return button_down ? 
	      STATE_button_down_cancel
	    : STATE_on;
}

state_t
state_alarm(void)
{
	uint32_t lt, t;
	uint8_t h, m;
	struct st s;
	
	lt = 0;
	init_st(&s);
	
	set_lamp_brightness(0);
	
	get_time(&h, &m);
	
	set_display_state(true);
	display_draw(true, 
	             h / 10, h % 10,
	             m / 10, m % 10);
	
	/* Fade on. */
	
	lt = 0;
	while (get_lamp_brightness() < 0xff) {
		if (button_down) {
			return STATE_button_down_cancel;
		}
		
		t = st_diff(&s);
		if (t > lt) {
			lt = t + SEC_MICRO;
			set_lamp_brightness(get_lamp_brightness() + 1);
		}
	}
	
	/* Full for some time. */
	do {
		if (button_down) {
			return STATE_button_down_cancel;
		}
		
		t = st_diff(&s);
	} while (t < ALARM_LENGTH * 60 * SEC_MICRO);
	
	/* Fade off. */
	
	lt = 0;
	while (get_lamp_brightness() > 0) {
		if (button_down) {
			return STATE_button_down_cancel;
		}
		
		t = st_diff(&s);
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
	uint8_t h, m;
	struct st s;
	bool on;
	
	lt = 0;
	init_st(&s);
	
	/* Turn on. */
	set_lamp_brightness(0xff);
	set_display_state(true);
	
	get_time(&h, &m);
	
	display_draw(true, 
	             h / 10, h % 10,
	             m / 10, m % 10);
	             
	do {
		if (!button_down) {
			return STATE_on;
		}
		
		t = st_diff(&s);
	} while (t < 1 * SEC_MICRO);
	
	/* Cancel. */
	set_lamp_brightness(0);
	set_display_state(false);
	
	do {
		if (!button_down) {
			return STATE_wait;
		}
		
		t = st_diff(&s);
	} while (t < 2 * SEC_MICRO);
	
	/* Set alarm. */
	set_lamp_brightness(0xff);
	on = true;
	set_display_state(on);
	
	lt = 0;
	do {
		if (t > lt) {
			lt = t + SEC_MICRO / 4;
			on = !on;
			set_display_state(on);
		}
	
		if (!button_down) {
			return STATE_set_alarm;
		}
		
		t = st_diff(&s);
	} while (t < 4 * SEC_MICRO);
	
	/* Set time. */
	lt = 0;
	do {
		if (t > lt) {
			lt = t + SEC_MICRO / 8;
			on = !on;
			set_display_state(on);
		}
	
		if (!button_down) {
			return STATE_set_time;
		}
		
		t = st_diff(&s);
	} while (t < 6 * SEC_MICRO);
	
	return STATE_button_down_cancel;
}

bool
get_setting_hour_minutes_value(uint8_t *h, uint8_t *m, uint8_t rate, bool ishour)
{
	uint32_t lt, t;
	struct st s;
	bool on;
	
	lt = 0;
	init_st(&s);
	
	on = true;
	set_display_state(true);
	
	while (true) {
		if (button_down) {
			init_st(&s);
			lt = 0;
			
			while (true) {
				t = st_diff(&s);
				if (t > lt) {
					lt = t + SEC_MICRO / rate;
					if (lt > SEC_MICRO * 3) {
						return false;
					}
					
					on = !on;
					if (on || (!ishour && lt > SEC_MICRO)) {
						display_draw(true, 
						             *h / 10, *h % 10,
						             *m / 10, *m % 10);
						             				            
					} else if (!ishour || lt > SEC_MICRO) {
						display_draw(true, 
						             *h / 10, *h % 10,
						             0x10, 0x10);
						             
					} else {
						display_draw(true, 
						             0x10, 0x10,
						             *m / 10, *m % 10);
						             
					}
				}
				
				if (!button_down) {
					if (t <= SEC_MICRO) {
						break;
					} else {
						return true;
					}
				}
			}
			
			if (ishour) {
				*h = (*h + 1) % 24;
			} else {
				*m = (*m + 1) % 60;
			}
			
			init_st(&s);
			lt = 0;
		}
		
		t = st_diff(&s);
		if (t > lt) {
			lt = t + SEC_MICRO / rate;
			
			on = !on;
			if (on) {
				display_draw(true, 
				             *h / 10, *h % 10,
				             *m / 10, *m % 10);
				             				            
			} else if (ishour) {
				display_draw(true, 
				             0x10, 0x10,
				             *m / 10, *m % 10);
				             
			} else {
				display_draw(true, 
				             *h / 10, *h % 10,
				             0x10, 0x10);
			}
			
		} else if (t > 60 * SEC_MICRO) {
			return false;
		}
	}
}


bool
get_setting_hour_minutes(uint8_t *h, uint8_t *m, uint8_t rate)
{
	if (!get_setting_hour_minutes_value(h, m, rate, true)) {
		return false;
	} else {
		return get_setting_hour_minutes_value(h, m, rate, false);
	}
}

state_t
state_set_alarm(void)
{
	uint8_t h, m;
		
	get_time(&h, &m);
	
	if (get_setting_hour_minutes(&h, &m, 4)) {
		set_time(h, m);
		return STATE_on;
	} else {
		return STATE_button_down_cancel;
	}
	
}

state_t
state_set_time(void)
{
	uint8_t h, m;
		
	get_time(&h, &m);
	
	if (get_setting_hour_minutes(&h, &m, 8)) {
		set_alarm(h, m);
		return STATE_on;
	} else {
		return STATE_button_down_cancel;
	}
	
	return STATE_on;
}

state_t
state_wait(void)
{
	if (button_down) {
		return STATE_button_down;
	
	} else {
		set_lamp_brightness(0);
		set_display_state(false);
	
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
	[STATE_button_down_cancel] = state_button_down_cancel,
};

int
main(void)
{
	state_t s;
	
	DDRB |= LAMP;
	PORTB |= BUTTON;
	
	init_timers();
	init_display(PIN_DCK, PIN_DIO);
	init_clock(PIN_CSDA, PIN_CSCL);
	
	sei();
	delay(1500);
	
	s = STATE_wait;
	while (true) {
		s = states[s]();
	}
}
