#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define LAMP    (1 << 0)

uint8_t lamp_brightness = 0;
uint32_t t = 0;

int
main(void)
{
	DDRB |= LAMP;
	
	TCCR1 |= (1<<CS13)|(1<<CS12)|(1<<CS10);
	TCNT1 = 0;
	
	while (1) {
		if (TCNT1 > 200) {
			if (PORTB & LAMP) {
				PORTB &= ~LAMP;
				TCNT1 = lamp_brightness;
			} else {
				PORTB |= LAMP;
				TCNT1 = 200 - lamp_brightness;
			}
			
			t++;
		}
		
		if (t > 10) {
			lamp_brightness = lamp_brightness > 100 ? 20 : 180;
			t = 0;
		}
	}
}

