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

