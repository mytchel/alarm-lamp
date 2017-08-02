#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define LAMP    (1 << 0)

volatile uint8_t lamp_brightness = 20;
volatile uint32_t t = 0;

int
main(void)
{
	DDRB |= LAMP;
	
	TCCR1 |= (1<<CS13)|(1<<CS12)|(1<<CS11)|(1<<CS10);
	TCNT1 = 0;
	
	while (1) {
		if (TIFR & (1<<TOV1)) {
			PORTB |= LAMP;
		} else {
			PORTB &= ~LAMP;
		}
	}
}

