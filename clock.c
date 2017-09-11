#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "lamp.h"

static uint8_t hour, min;

void
set_time(uint8_t h, uint8_t m)
{
	hour = h;
	min = m;
}

void
set_alarm(uint8_t h, uint8_t m)
{

}

void
get_time(uint8_t *h, uint8_t *m)
{
	*h = hour;
	*m = min;
}

void
get_alarm(uint8_t *h, uint8_t *m)
{
	*h = 0;
	*m = 0;
}


void
clock_test(void)
{
	uint8_t addr = 0b1101000;
	uint8_t wm[1] = { 0 };
	uint8_t rm[2] = { 0 };
	
	set_display_state(true);
	
	while (true) {
		i2c_message(addr, 
		            wm, sizeof(wm),
		            rm, sizeof(rm));
	
		display_draw(true,
		             rm[1] / 10, rm[1] % 10,
		             rm[0] / 10, rm[0] % 10);
	}
}
