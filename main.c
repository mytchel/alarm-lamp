#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define LAMP    (1 << PORTB0)
#define BUTTON  (1 << 1)

volatile uint8_t t;

ISR(TIMER1_OVF_vect)
{
	t++;
	
	if (t > 250) {
		if (PORTB & LAMP) {
			PORTB &= ~LAMP;
		} else {
			PORTB |= LAMP;
		}
		
		t = 0;
	}
}
  
int
main(void)
{
	DDRB |= LAMP;
	PORTB |= BUTTON;
		
	TCCR1 |= (1<<CS11);
	TCNT1 = 0;
	TIMSK |= (1<<TOIE1);
	
	sei();
	
	while (1) {
	
	}
}

