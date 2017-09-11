#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "lamp.h"

static uint8_t segments[] = {
	0x3f, 0x06, 0x5b, 0x4f,
	0x66, 0x6d, 0x7d, 0x07,
	0x7f, 0x6f, 0x77, 0x7c,
	0x39, 0x5e, 0x79, 0x71
};

static bool display_on;

void
init_display(void)
{
	DDR_DCK |= (1<<DCK_PIN_N);
	DDR_DIO |= (1<<DIO_PIN_N);
	PORT_DCK &= ~(1<<DCK_PIN_N);
	PORT_DIO &= ~(1<<DIO_PIN_N);
	display_on = false;
}

void
display_send_byte(uint8_t b)
{
	uint8_t count;
	int i;
	
	for (i = 0; i < 8; i++) {
		PORT_DCK &= ~(1<<DCK_PIN_N);
	
		if (b & 1)
			PORT_DIO |= (1<<DIO_PIN_N);
		else
			PORT_DIO &= ~(1<<DIO_PIN_N);
				
		b >>= 1;
		PORT_DCK |= (1<<DCK_PIN_N);
	}
	
	PORT_DCK &= ~(1<<DCK_PIN_N);
	PORT_DIO |= (1<<DIO_PIN_N);
	PORT_DCK |= (1<<DCK_PIN_N);
	
	DDR_DIO &= ~(1<<DIO_PIN_N);
	
	while (PIN_DIO & (1<<DIO_PIN_N)) {
		if (count++ == 200) {
			DDR_DIO |= (1<<DIO_PIN_N);
			PORT_DIO &= ~(1<<DIO_PIN_N);
			
			count = 0;
			
			DDR_DIO &= ~(1<<DIO_PIN_N);
		}
	}
	
	DDR_DIO |= (1<<DIO_PIN_N);
}

void
display_start(void)
{
	PORT_DCK |= (1<<DCK_PIN_N);
	PORT_DIO |= (1<<DIO_PIN_N);
	PORT_DIO &= ~(1<<DIO_PIN_N);
	PORT_DCK &= ~(1<<DCK_PIN_N);
}

void
display_stop(void)
{
	PORT_DCK &= ~(1<<DCK_PIN_N);
	PORT_DIO &= ~(1<<DIO_PIN_N);
	PORT_DCK |= (1<<DCK_PIN_N);
	PORT_DIO |= (1<<DIO_PIN_N);
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
