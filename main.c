#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define LAMP    (1 << 0)
#define lamp_brightness OCR1A

#define BUTTON  (1 << 1)

/* In roughly quarter seconds. */
volatile uint64_t time = 0;

volatile uint64_t lamp_fade = 0;

ISR(TIMER1_OVF_vect)
{
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
do_lamp_fade(void)
{
	if (lamp_fade == 0) return;

	if (lamp_fade > 1)
		lamp_fade--;
	
	if (lamp_fade > 1) return;
	
	if (lamp_brightness > 4) {
		/* Fade out lamp. */
		lamp_brightness -= 4;
		
	} else {
		/* Finish fade. */
		lamp_brightness = 0;
		lamp_fade = 0;
	}
}

void
start_lamp_fade(uint64_t seconds_full)
{
	lamp_fade = 1 + seconds_full * 4;
	lamp_brightness = 255;
}

ISR(TIMER0_COMPA_vect)
{
	TCNT0 = 0;
	time++;
	
	do_lamp_fade();
}

int
main(void)
{
	DDRB |= LAMP;
	/* Pull button high. */
	PORTB |= BUTTON;
	
	TCCR1 |= (1<<CS10);
	TCNT1 = 0;
	lamp_brightness = 0;
	TIMSK |= (1<<TOIE1)|(1<<OCIE1A);
	
	TCCR0B |= (1<<CS02)|(1<<CS00);
	TCNT0 = 0;
	/* Magic value to get a interrupt to be roughly 
	 * a 1/4 second. */
	OCR0A = 244;
	TIMSK |= (1<<OCIE0A);
	
	start_lamp_fade(1);
	
	sei();
	while (1) {
		if (!(PINB & BUTTON) && TCNT0 == 0) {
			start_lamp_fade(10);
		}
	}
}
