#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define LAMP    (1 << 0)

uint8_t lamp_brightness = 0;
uint32_t t = 0;

ISR(TIMER1_OVF_vect)
{
	if (PORTB & LAMP) {
			PORTB &= ~LAMP;
			TCNT1 = lamp_brightness;
		} else {
			PORTB |= LAMP;
			TCNT1 = 200 - lamp_brightness;
		}
		t++;
}

int
main(void)
{
	DDRB |= LAMP;
	
	TCCR1 |= (1<<CS10);
	TCNT1 = 0;
	TIMSK |= TOIE1;
	
	while (1) {
		if (t > 20000) {
			lamp_brightness = lamp_brightness > 100 ? 20 : 180;
			t = 0;
		}
	}
}

