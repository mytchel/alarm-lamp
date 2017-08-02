#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define LAMP    (1 << 0)

int
main(void)
{
	DDRB |= LAMP;
	
	TCCR1 |= (1<<CS13)|(1<<CS12)|(1<<CS11)|(1<<CS10);
	TIMSK = (1<<TOIE1)|(1<<OCIE1A);
	TCNT1 = 0;
	OCR1A = 200;
	
	sei();
	while (1) {
	
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
