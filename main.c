#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define LAMP      (1 << 0)

#define BUTTON    (1 << 4)
#define SDA       (1 << 0)
#define SCL       (1 << 2)

#define TM1637_comm1  0x40
#define TM1637_comm2  0xc0
#define TM1637_comm3  0x80

#define ALARM_LENGTH 30

#define SEC_MICRO 1000000UL

/* Order of parts matters, otherwise the operation is not
 * done right due to 32bit limit. I think. */
#define TIMER0_OVF_period \
    (((SEC_MICRO * 256UL) / F_CPU) * 1UL)

struct time {
	uint32_t u, s;
};

volatile struct time time = {0};

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
	time.u += TIMER0_OVF_period;
	
	if (time.u >= SEC_MICRO) {
		time.u -= SEC_MICRO;
		time.s++;
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

uint32_t
time_diff(struct time *s)
{
	return (time.s - s->s) * SEC_MICRO
	        + (time.u - s->u);
}

void
get_time(struct time *dest)
{
	dest->s = time.s;
	dest->u = time.u;
}

void
delay(uint32_t t)
{
	struct time s;
	
	get_time(&s);
	while (time_diff(&s) < t)
		;
}

void
init_i2c(void)
{
	PORTB |= (1<<SDA);
	PORTB |= (1<<SCL);
	
	DDRB |= (1<<SDA);
	DDRB |= (1<<SCL);
	
	USIDR = 0xff;
	USICR = (1<<USIWM1)|(1<<USICLK);
	
	USISR = (1<<USISIF)|(1<<USIOIF)|
	        (1<<USIPF)|(1<<USIDC);
}

char
i2c_transfer(bool byte)
{
	char d = (1<<USISIF)|(1<<USIOIF)|
	         (1<<USIPF)|(1<<USIDC);
	        
	if (byte) {
		USISR = d | 0;
	} else {
		USISR = d | 0xe;
	}
	
	d = (1<<USIWM1)|(1<<USICLK)|(1<<USITC);
	
	do {
		USICR = d;
		while (!(PINB & (1<<SCL)))
			;
		delay(50);
		USICR = d;
	} while (!(USISR & (1<<USIOIF)));

	delay(50);
	d = USIDR;
	USIDR = 0xff;
	DDRB |= (1<<SDA);
	
	return d;
}

int
i2c_send(char *data, int len)
{
	bool addr = true, write;
	
	write = !(data[0] & 1);
	
	/* Start */
	
	PORTB |= (1<<SCL);
	while (!(PINB & (1<<SCL)))
		;
	
	delay(50);
	
	PORTB &= ~(1<<SDA);
	delay(50);
	
	PORTB &= ~(1<<SCL);
	PORTB |= (1<<SDA);
	
	if (!(USISR & (1<<USISIF))) {
		return 1;
	}
	
	do {
		if (addr || write) {
			PORTB &= ~(1<<SCL);
			USIDR = *(data++);
			
			i2c_transfer(true);
			
			DDRB &= ~(1<<SDA);
			if (i2c_transfer(false) & 1) {
				return 2;
			}
			
			addr = false;
		} else {
			DDRB &= ~(1<<SDA);
			*(data++) = i2c_transfer(true);
			
			if (len == 1) {
				USIDR = 0xff; /* End of transmisison. */
			} else {
				USIDR = 0x00; /* Ack. */
			}
			
			i2c_transfer(false);
		}
	} while (len-- > 0);
	
	/* Stop */
	
	PORTB &= ~(1<<SDA);
	PORTB |= (1<<SCL);
	while (!(PINB & (1<<SCL)))
		;
	
	delay(50);
	PORTB |= (1<<SDA);
	delay(50);
	
	if (!(USISR & (1<<USIPF))) {
		return 3;
	}
	
	return 0;
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
	set_lamp_brightness(0xff);
	
	while (!button_down)
		;
	
	return STATE_button_down_cancel;
}

state_t
state_alarm(void)
{
	uint32_t lt, t;
	struct time s;
	
	lt = 0;
	get_time(&s);
	
	set_lamp_brightness(0);
	
	/* Fade on. */
	
	lt = 0;
	while (get_lamp_brightness() < 0xff) {
		if (button_down) {
			return STATE_button_down_cancel;
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
			return STATE_button_down_cancel;
		}
		
		t = time_diff(&s);
	} while (t < ALARM_LENGTH * 60 * SEC_MICRO);
	
	/* Fade off. */
	
	lt = 0;
	while (get_lamp_brightness() > 0) {
		if (button_down) {
			return STATE_button_down_cancel;
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
	
	/* Turn on. */
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
	
	return STATE_button_down_cancel;
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
		/* Do something. */
	}
	
	return STATE_wait;
}

state_t
state_set_time(void)
{
	uint8_t hour, minute;
	
	if (get_hour_minutes(&hour, &minute, 8)) {
		/* Do something. */
	}
	
	return STATE_wait;
}

state_t
state_wait(void)
{
	if (button_down) {
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
	[STATE_button_down_cancel] = state_button_down_cancel,
};

int
main(void)
{
	char m0[] = { TM1637_comm1 };
	char m1[] = { TM1637_comm2, 0xff, 0xff, 0xff, 0xff };
	char m2[] = { TM1637_comm3 + 0x4 };
	state_t s;
	
	DDRB |= LAMP;
	PORTB |= BUTTON;
	s = STATE_wait;
	
	init_timers();
	
	struct time t;
	int i;
	for (i = 0; i < 5; i++) {
		set_lamp_brightness(0xff);
		
		get_time(&t);
		while (time_diff(&t) < SEC_MICRO)
			;
	
		set_lamp_brightness(0);
		
		get_time(&t);
		while (time_diff(&t) < SEC_MICRO)
			;
	}
	
	set_lamp_brightness(0xff);
	
	/*
	init_i2c();
	
	i2c_send(m0, sizeof(m0));
	i2c_send(m1, sizeof(m1));
	i2c_send(m2, sizeof(m2));
	
	*/
	
	set_lamp_brightness(0);
	
	while (true)
		;
	
	sei();
	while (true) {
		s = states[s]();
	}
}
