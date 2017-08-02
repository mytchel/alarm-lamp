#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define LAMP    (1 << 0)
#define lamp_brightness OCR1A

/* In roughly seconds. */
volatile uint64_t time = 0;

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

ISR(TIMER0_COMPA_vect)
{
	static uint8_t t = 0;
	
	TCNT0 = 0;
	if (t++ == 4) {
		time++;
		t = 0;
	}
}

int
main(void)
{
	DDRB |= LAMP;
	
	TCCR1 |= (1<<CS11);
	TCNT1 = 0;
	lamp_brightness = 200;
	
	TIMSK |= (1<<TOIE1)|(1<<OCIE1A);
	
	TCCR0B |= (1<<CS02)|(1<<CS00);
	TCNT0 = 0;
	OCR0A = 244;
	TIMSK |= (1<<OCIE0A);
	
	uint64_t t = 0;
	sei();
	while (1) {
		if (time != t) {
			t = time;
			lamp_brightness = lamp_brightness ? 0 : 255;
		}
	}
}
