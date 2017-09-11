#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "lamp.h"

#define ALARM_LENGTH 30

#define SEC_MICRO 1000000UL

#define BUTTON_DEBOUNCE 30

/* Order of parts matters, otherwise the operation is not
 * done right due to 32bit limit. I think. */
#define TIMER0_OVF_period \
    (((SEC_MICRO * 256UL) / F_CPU) * 64UL)

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
	TCCR0B |= (1<<CS01)|(1<<CS00);
	TIMSK0 |= (1<<TOIE0)|(1<<OCIE0A);
}

ISR(TIMER0_OVF_vect)
{
	static uint32_t button_count = 0;
	
	st.u += TIMER0_OVF_period;
	if (st.u >= SEC_MICRO) {
		st.u -= SEC_MICRO;
		st.s++;
	}
	
	if ((PIN_BUTTON & (1<<BUTTON_PIN_N)) && button_down) {
		if (button_count++ == BUTTON_DEBOUNCE) {
			button_down = false;
		}
	} else if (!(PIN_BUTTON & (1<<BUTTON_PIN_N)) && !button_down) {
		if (button_count++ == BUTTON_DEBOUNCE) {
			button_down = true;
		}
	} else {
		button_count = 0;
	}
	
	if (0 < lamp_brightness && lamp_brightness < 0xff) {
		PORT_LAMP |= (1<<LAMP_PIN_N);
		OCR0A = lamp_brightness;
	}
}

ISR(TIMER0_COMPA_vect)
{
	if (0 < lamp_brightness && lamp_brightness < 0xff) {
		PORT_LAMP &= ~(1<<LAMP_PIN_N);
	}
}

void
set_lamp_brightness(uint8_t b)
{
	lamp_brightness = b;
	
	if (b == 0) {
		PORT_LAMP &= ~(1<<LAMP_PIN_N);
		
	} else if (b == 0xff) {
		PORT_LAMP |= (1<<LAMP_PIN_N);
	}
}

uint8_t
get_lamp_brightness(void)
{
	return lamp_brightness;
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
	uint8_t th, tm, ah, am;	
	uint32_t td;
	
	if (button_down) {
		return STATE_button_down_cancel;
	}
	
	get_time(&th, &tm);
	get_alarm(&ah, &am);
	
	set_display_state(true);
	display_draw(true, 
	             th / 10, th % 10,
	             tm / 10, tm % 10);
		
	set_lamp_brightness(0xff);
	
	td = (th - ah) * 60 + (tm - am);
	
	if (td > 30) {
		return STATE_wait;
	} else {
		return STATE_alarm;
	}
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
	
	set_lamp_brightness(0xff);
	on = false;
	lt = 0;
	
	/* Set time. */
	
	do {
		if (t > lt) {
			lt = t + SEC_MICRO / 4;
			on = !on;
			set_display_state(on);
		}
	
		if (!button_down) {
			return STATE_set_time;
		}
		
		t = st_diff(&s);
	} while (t < 5 * SEC_MICRO);
	
	/* Set alarm. */
	
	get_alarm(&h, &m);
	display_draw(true, 
	             h / 10, h % 10,
	             m / 10, m % 10);
	
	do {
		if (t > lt) {
			lt = t + SEC_MICRO / 8;
			on = !on;
			set_display_state(on);
		}
	
		if (!button_down) {
			return STATE_set_alarm;
		}
		
		t = st_diff(&s);
	} while (t < 8 * SEC_MICRO);
	
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
					
					on = !on;
					if (on || (!ishour && t > SEC_MICRO)) {
						display_draw(true, 
						             *h / 10, *h % 10,
						             *m / 10, *m % 10);
						             				            
					} else if (!ishour || t > SEC_MICRO) {
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
		
	get_alarm(&h, &m);
	
	if (get_setting_hour_minutes(&h, &m, 8)) {
		set_alarm(h, m);
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
	
	if (get_setting_hour_minutes(&h, &m, 4)) {
		set_time(h, m);
		return STATE_on;
	} else {
		return STATE_button_down_cancel;
	}
}

state_t
state_wait(void)
{
	uint8_t ah, am, th, tm;
	
	if (button_down) {
		return STATE_button_down;
	
	} else {
		set_lamp_brightness(0);
		set_display_state(false);
		
		get_alarm(&ah, &am);
		get_time(&th, &tm);
		
		if (th == ah && tm == am) {
			return STATE_alarm;
		} else {
			return STATE_wait;
		}
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
	
	DDR_LAMP |= (1<<LAMP_PIN_N);
	PORT_BUTTON |= (1<<BUTTON_PIN_N);
	
	init_timers();
	init_display();
	init_i2c();
	
	sei();
	
	s = STATE_wait;
	while (true) {
		s = states[s]();
	}
}
