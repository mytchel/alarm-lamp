#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define LAMP    (1 << 0)

volatile uint8_t lamp_brightness = 0;
volatile uint32_t t = 0;

int
main(void)
{
	DDRB |= LAMP;
	
	TCCR1 |= (1<<CS10);
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
		
		if (t > 20000) {
			lamp_brightness = lamp_brightness > 100 ? 20 : 180;
			t = 0;
		}
	}
}

