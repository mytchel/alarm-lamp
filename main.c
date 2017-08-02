#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define LAMP    (1 << 0)

int
main(void)
{
	DDRB |= LAMP;
	
	TCCR1 |= (1<<CS13)|(1<<CS12)|(1<<CS11)|(1<<CS10);
	TCNT1 = 0;
	OCR1A = 200;
	
	while (1) {
		if (TIFR & (1<<TOV1)) {
			PORTB |= LAMP;
			TIFR = (1<<TOV1);
		}
		
		if (TIFR & (1<<OCF1A)) {
			PORTB &= ~LAMP;
			TIFR = (1<<OCF1A);
		}
	}
}

