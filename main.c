#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define LAMP      (1 << 1)
#define BUTTON    (1 << 4)
#define DIO       (1 << 0)
#define DCK       (1 << 2)

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
set_lamp_brightness(uint8_t b);

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


#if 0
/* The display is not i2c. I have no need of this
   until I have got the display working. */
   
void
init_i2c(void)
{
	PORTB |= SDA;
	PORTB |= SCL;
	
	DDRB |= SDA;
	DDRB |= SCL;
	
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
		while (!(PINB & SCL))
			;
		delay(50);
		USICR = d;
	} while (!(USISR & (1<<USIOIF)));

	delay(50);
	d = USIDR;
	USIDR = 0xff;
	DDRB |= SDA;
	
	return d;
}

int
i2c_send(char *data, int len)
{
	bool addr = true, write;
	
	write = !(data[0] & 1);
	
	/* Start */
	
	PORTB |= SCL;
	while (!(PINB & SCL))
		;
	
	delay(50);
	
	PORTB &= ~SDA;
	delay(50);
	
	PORTB &= ~SCL;
	PORTB |= SDA;
	
	if (!(USISR & (1<<USISIF))) {
		return 1;
	}
	
	do {
		if (addr || write) {
			PORTB &= ~SCL;
			USIDR = *(data++);
			
			i2c_transfer(true);
			
			DDRB &= ~SDA;
			if (i2c_transfer(false) & 1) {
		set_lamp_brightness(0);
		while (true)
			;
				return 2;
			}
			
			addr = false;
		} else {
			DDRB &= ~SDA;
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
	
	PORTB &= ~SDA;
	PORTB |= SCL;
	while (!(PINB & SCL))
		;
	
	delay(50);
	PORTB |= SDA;
	delay(50);
	
	if (!(USISR & (1<<USIPF))) {
		return 3;
	}
	
	return 0;
}

#endif

uint8_t segments[] = {
	0x3f, 0x06, 0x5b, 0x4f,
	0x66, 0x6d, 0x7d, 0x07,
	0x7f, 0x6f, 0x77, 0x7c,
	0x39, 0x5e, 0x79, 0x71
};

void
init_display(void)
{
	DDRB |= DCK;
	DDRB |= DIO;
	PORTB &= ~DCK;
	PORTB &= ~DIO;
}

void
display_send_byte(uint8_t b)
{
	uint8_t count;
	int i;
	
	for (i = 0; i < 8; i++) {
		PORTB &= ~DCK;
	
		if (b & 1)
			PORTB |= DIO;
		else
			PORTB &= ~DIO;
				
		b >>= 1;
		PORTB |= DCK;
	}
	
	PORTB &= ~DCK;
	PORTB |= DIO;
	PORTB |= DCK;
	
	DDRB &= ~DIO;
	
	while (PINB & DIO) {
		if (count++ == 200) {
			DDRB |= DIO;
			PORTB &= ~DIO;
			
			count = 0;
			
			DDRB &= ~DIO;
		}
	}
	
	DDRB |= DIO;
}

void
display_start(void)
{
	PORTB |= DCK;
	PORTB |= DIO;
	PORTB &= ~DIO;
	PORTB &= ~DCK;
}

void
display_stop(void)
{
	PORTB &= ~DCK;
	PORTB &= ~DIO;
	PORTB |= DCK;
	PORTB |= DIO;
}

void
display_draw(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{	
	display_start();
	display_send_byte(0x40);
	display_stop();
	
	display_start();
	display_send_byte(0xc0);
	display_send_byte(segments[a & 0xf]);
	display_send_byte(segments[b & 0xf]);
	display_send_byte(segments[c & 0xf]);
	display_send_byte(segments[d & 0xf]);
	display_stop();
}

void
display_off(void)
{
	display_start();
	display_send_byte(0x80);
	display_stop();
}

void
display_on(void)
{
	display_start();
	display_send_byte(0x88 + 7);
	display_stop();
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
	state_t s;
	
	DDRB |= LAMP;
	
	init_timers();
	
	sei();
	
	int i;
	for (i = 0; i < 5; i++) {
		set_lamp_brightness(0xff);
		
		delay(100000);
	
		set_lamp_brightness(0);
		
		delay(100000);
	}
	
	set_lamp_brightness(0xff);
	
	init_display();
	
	delay(1500);
		
	uint8_t a, b, c, d;
	int j;
	
		
	display_on();
	i = 0;
	while (true) {
		j = i++;
		d = j % 16;
		j /= 16;
		c = j % 16;
		j /= 16;
		b = j % 16;
		j /= 16;
		a = j % 16;
	
		display_draw(a, b, c, d);
		
		if (i % 10 == 0) {
			display_on();
		} else if (i % 5 == 0) {
			display_off();
		}
		
		delay(500000);
	}
			
	while (true) {
	
		set_lamp_brightness(0xff);
		
		delay(1000000);
	
		set_lamp_brightness(0);
		
		delay(1000000);
	}
	
	sei();
	s = STATE_wait;
	while (true) {
		s = states[s]();
	}
}
