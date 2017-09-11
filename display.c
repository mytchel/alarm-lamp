#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

static uint8_t segments[] = {
	0x3f, 0x06, 0x5b, 0x4f,
	0x66, 0x6d, 0x7d, 0x07,
	0x7f, 0x6f, 0x77, 0x7c,
	0x39, 0x5e, 0x79, 0x71
};

static bool display_on;

static uint8_t DCK, DIO;

void
init_display(int dck, int dio)
{
	DCK = 1 << dck;
	DIO = 1 << dio;
	
	DDRB |= DCK;
	DDRB |= DIO;
	PORTB &= ~DCK;
	PORTB &= ~DIO;
	display_on = false;
}

void
display_send_byte(uint8_t b)
{
	uint8_t count;
	int i;
	
	for (i = 0; i < 8; i++) {
		PORTB &= ~DCK;
	
		if (b & 1)
			PORTB |= DIO;
		else
			PORTB &= ~DIO;
				
		b >>= 1;
		PORTB |= DCK;
	}
	
	PORTB &= ~DCK;
	PORTB |= DIO;
	PORTB |= DCK;
	
	DDRB &= ~DIO;
	
	while (PINB & DIO) {
		if (count++ == 200) {
			DDRB |= DIO;
			PORTB &= ~DIO;
			
			count = 0;
			
			DDRB &= ~DIO;
		}
	}
	
	DDRB |= DIO;
}

void
display_start(void)
{
	PORTB |= DCK;
	PORTB |= DIO;
	PORTB &= ~DIO;
	PORTB &= ~DCK;
}

void
display_stop(void)
{
	PORTB &= ~DCK;
	PORTB &= ~DIO;
	PORTB |= DCK;
	PORTB |= DIO;
}

void
display_draw(bool sep, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{	
	display_start();
	display_send_byte(0x40);
	display_stop();
	
	display_start();
	display_send_byte(0xc0);
	display_send_byte(a == 0x10 ? 0 : segments[a & 0xf]);
	display_send_byte(b == 0x10 ? 0 : segments[b & 0xf] | (sep ? 0x80 : 0));
	display_send_byte(c == 0x10 ? 0 : segments[c & 0xf]);
	display_send_byte(d == 0x10 ? 0 : segments[d & 0xf]);
	display_stop();
}

void
set_display_state(bool on)
{
	display_on = on;
	
	display_start();
	display_send_byte((0x80 + 7) | (on ? 0x08 : 0));
	display_stop();
}

bool
get_display_state(void)
{
	return display_on;
}
