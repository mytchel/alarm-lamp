#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "lamp.h"

static uint8_t SDA, SCL;

void
init_i2c(uint8_t sda, uint8_t scl)
{
	SDA = 1 << sda;
	SCL = 1 << scl;
	
	PORTB |= SDA;
	PORTB |= SCL;
	
	DDRB |= SDA;
	DDRB |= SCL;
	
	USIDR = 0xff;
	USICR = (1<<USIWM1)|(1<<USICLK);
	
	USISR = (1<<USISIF)|(1<<USIOIF)|
	        (1<<USIPF)|(1<<USIDC);
}

char
i2c_transfer(bool byte)
{
	char d = (1<<USISIF)|(1<<USIOIF)|
	         (1<<USIPF)|(1<<USIDC);
	        
	if (byte) {
		USISR = d | 0;
	} else {
		USISR = d | 0xe;
	}
	
	d = (1<<USIWM1)|(1<<USICLK)|(1<<USITC);
	
	do {
		USICR = d;
		while (!(PINB & SCL))
			;
		delay(50);
		USICR = d;
	} while (!(USISR & (1<<USIOIF)));

	delay(50);
	d = USIDR;
	USIDR = 0xff;
	DDRB |= SDA;
	
	return d;
}

int
i2c_send(char *data, int len)
{
	bool addr = true, write;
	
	write = !(data[0] & 1);
	
	/* Start */
	
	PORTB |= SCL;
	while (!(PINB & SCL))
		;
	
	delay(50);
	
	PORTB &= ~SDA;
	delay(50);
	
	PORTB &= ~SCL;
	PORTB |= SDA;
	
	if (!(USISR & (1<<USISIF))) {
		return 1;
	}
	
	do {
		if (addr || write) {
			PORTB &= ~SCL;
			USIDR = *(data++);
			
			i2c_transfer(true);
			
			DDRB &= ~SDA;
			if (i2c_transfer(false) & 1) {
				return 2;
			}
			
			addr = false;
		} else {
			DDRB &= ~SDA;
			*(data++) = i2c_transfer(true);
			
			if (len == 1) {
				USIDR = 0xff; /* End of transmisison. */
			} else {
				USIDR = 0x00; /* Ack. */
			}
			
			i2c_transfer(false);
		}
	} while (len-- > 0);
	
	/* Stop */
	
	PORTB &= ~SDA;
	PORTB |= SCL;
	while (!(PINB & SCL))
		;
	
	delay(50);
	PORTB |= SDA;
	delay(50);
	
	if (!(USISR & (1<<USIPF))) {
		return 3;
	}
	
	return 0;
}
