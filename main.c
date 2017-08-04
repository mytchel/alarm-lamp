#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define LAMP      (1 << 0)

#define BUTTON    (1 << 1)

typedef enum {
	ACTION_none,
	ACTION_fade,
	ACTION_set_time,
	ACTION_set_alarm,
	ACTION_alarm,
} action_t;

volatile uint32_t time_n = 0;
volatile uint32_t time_s = 0;

volatile action_t action = ACTION_none;

ISR(TIMER1_OVF_vect)
{
	time_n += 42667;
	if (time_n >= 1000000000L) {
		time_s++;
		time_n -= 1000000000L;
	}
	
	if (OCR1A > 0) {
		PORTB |= LAMP;
	} else {
		PORTB &= ~LAMP;
	}
}

ISR(TIMER1_COMPA_vect)
{
	if (OCR1A < 0xff) {
		PORTB &= ~LAMP;
	} else {
		PORTB |= LAMP;
	}
}

void
set_lamp_brightness(uint8_t b)
{
	OCR1A = b;
}

uint8_t
get_lamp_brightness(void)
{
	return OCR1A;
}

volatile uint32_t fade_state;

void
start_fade(uint64_t seconds_full)
{
	action = ACTION_fade;
	
	fade_state = seconds_full * 40;
	set_lamp_brightness(0xff);
}

void
action_fade(void)
{
	if (fade_state > 0) {
		fade_state--;
		return;
	}
	
	if (get_lamp_brightness() > 1) {
		/* Fade away. */
		set_lamp_brightness(get_lamp_brightness() - 1);
		
	} else {
		/* Stop fade. */
		fade_state = 0;
		set_lamp_brightness(0);
		action = ACTION_none;
	}
}

/* Length of alarm in minutes. */
#define ALARM_LEN 1

volatile uint32_t alarm = 30;//0xffffffff;
volatile uint32_t prev_alarm = 0;

void
start_alarm(void)
{
	action = ACTION_alarm;
	
	prev_alarm = time_s;
	
	alarm += 60 * 60 * 24;
	
	set_lamp_brightness(0xff);
}

void
action_alarm(void)
{
	if (time_s > prev_alarm + 60 * ALARM_LEN) {
		/* Fade out. */
		start_fade(0);
	}
}

ISR(TIMER0_COMPA_vect)
{
	TCNT0 = 0;
	
	switch (action) {
	case ACTION_none:
		if (time_s > alarm) {
			start_alarm();
		}
		break;
		
	case ACTION_fade:
		action_fade();
		break;
		
	case ACTION_set_time:
		break;
		
	case ACTION_set_alarm:
		break;
		
	case ACTION_alarm:
		action_alarm();
		break;
	}
}

void
init_timers(void)
{
	/* Timer1 wants to go as fast as possible. */
	TCCR1 |= (1<<CS10);
	TCNT1 = 0;
	TIMSK |= (1<<TOIE1)|(1<<OCIE1A);
	
	/* Timer0 wants to be relitively slow. Diviser is 1024. */
	TCCR0B |= (1<<CS02)|(1<<CS00);
	TCNT0 = 0;
	/* Compare value to make it interrupt every 25ms. */
	OCR0A = F_CPU / 1024 / 40;
	TIMSK |= (1<<OCIE0A);
}

int
main(void)
{
	DDRB |= LAMP;
	PORTB |= BUTTON;
	
	init_timers();
	
	start_fade(1);
	
	sei();
	while (1) {
		if (!(PINB & BUTTON) && TCNT0 == 0) {
			start_fade(10);
		}
	}
}
