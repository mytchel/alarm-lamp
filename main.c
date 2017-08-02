#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define LAMP    (1 << 0)

uint32_t t = 0;

ISR(TIMER1_OVF_vect)
{
	PORTB |= LAMP;
}

ISR(TIMER1_COMPA_vect)
{
	PORTB &= ~LAMP;
}

int
main(void)
{
	DDRB |= LAMP;
	
	TCCR1 |= (1<<CS10);
	TIMSK = (1<<TOIE1)|(1<<OCIE1A);
	TCNT1 = 0;
	OCR1A = 200;
	
	TCCR0B |= (1<<CS01)|(1<<CS00);
	TCNT0 = 0;
	
	sei();
	while (1) {
		if (TCNT0 > 200) {
			TCNT0 = 0;
			t++;
		}
		
		if (t > 200) {
			OCR1A = OCR1A > 100 ? 10 : 240;
			t = 0;
		}
	}
}