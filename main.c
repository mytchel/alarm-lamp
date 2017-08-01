#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>

#define LAMP (1 << 0)

uint32_t t = 0;

int
main(void)
{
	DDRB |= LAMP;
	
	TCCR0B |= (1<<CS00);
	TCNT0 = 0;

	while (1) {
		if (TCNT0 > 200) {
			t++;
			TCNT0 = 0;
		}
		
		if (t > 500) {
			t = 0;
			PORTB ^= LAMP;
		}
	}
}

