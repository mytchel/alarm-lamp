#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "lamp.h"

#define DS1307_addr  0b1101000
#define alarm_addr 0x08

void
set_time(uint8_t h, uint8_t m)
{
	uint8_t wm[3] = { 1, 0, 0 };
	uint8_t hh, hl, mh, ml;
	
	mh = m / 10;
	ml = m % 10;
	hh = h / 10;
	hl = h % 10;
	
	wm[1] = mh << 4 | ml;
	wm[2] = hh << 4 | hl | (0<<6);	
	
	i2c_message(DS1307_addr, 
	            wm, sizeof(wm),
	            0, 0);
}

void
get_time(uint8_t *h, uint8_t *m)
{
	uint8_t wm[1] = { 1 };
	uint8_t rm[2] = { 0 };
	uint8_t hh, hl, mh, ml;
	
	i2c_message(DS1307_addr, 
	            wm, sizeof(wm),
	            rm, sizeof(rm));
	
	hh = (rm[1] >> 4) & 0x3;
	hl = rm[1] & 0xf;
		
	mh = (rm[0] >> 4) & 0x7;
	ml = rm[0] & 0xf;

	*h = hh * 10 + hl;
	*m = mh * 10 + ml;
}

void
set_alarm(uint8_t h, uint8_t m)
{
	uint8_t wm[3] = { alarm_addr, 0, 0 };
	uint8_t hh, hl, mh, ml;
	
	mh = m / 10;
	ml = m % 10;
	hh = h / 10;
	hl = h % 10;
	
	wm[1] = mh << 4 | ml;
	wm[2] = hh << 4 | hl | (0<<6);	
	
	i2c_message(DS1307_addr, 
	            wm, sizeof(wm),
	            0, 0);
}

void
get_alarm(uint8_t *h, uint8_t *m)
{
	uint8_t wm[1] = { alarm_addr };
	uint8_t rm[2] = { 0 };
	uint8_t hh, hl, mh, ml;
	
	i2c_message(DS1307_addr, 
	            wm, sizeof(wm),
	            rm, sizeof(rm));
	
	hh = (rm[1] >> 4) & 0x3;
	hl = rm[1] & 0xf;
		
	mh = (rm[0] >> 4) & 0x7;
	ml = rm[0] & 0xf;

	*h = hh * 10 + hl;
	*m = mh * 10 + ml;
}
