#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "lamp.h"

void
init_i2c(void)
{
	PORT_SDA |= (1<<SDA_PIN_N);
	PORT_SCL |= (1<<SCL_PIN_N);
	
	DDR_SDA |= (1<<SDA_PIN_N);
	DDR_SCL |= (1<<SCL_PIN_N);
	
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
		while (!(PIN_SCL & (1<<SCL_PIN_N)))
			;
		delay(50);
		USICR = d;
	} while (!(USISR & (1<<USIOIF)));

	delay(50);
	d = USIDR;
	USIDR = 0xff;
	DDR_SDA |= (1<<SDA_PIN_N);
	
	return d;
}

int
i2c_send(char *data, int len)
{
	bool addr = true, write;
	
	write = !(data[0] & 1);
	
	/* Start */
	
	PORT_SCL |= (1<<SCL_PIN_N);
	while (!(PIN_SCL & (1<<SCL_PIN_N)))
		;
	
	delay(50);
	
	PORT_SDA &= ~(1<<SDA_PIN_N);
	delay(50);
	
	PORT_SCL &= ~(1<<SCL_PIN_N);
	PORT_SDA |= (1<<SDA_PIN_N);
	
	if (!(USISR & (1<<USISIF))) {
		return 1;
	}
	
	do {
		if (addr || write) {
			PORT_SCL &= ~(1<<SCL_PIN_N);
			USIDR = *(data++);
			
			i2c_transfer(true);
			
			DDR_SDA &= ~(1<<SDA_PIN_N);
			if (i2c_transfer(false) & 1) {
				return 2;
			}
			
			addr = false;
		} else {
			DDR_SDA &= ~(1<<SDA_PIN_N);
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
	
	PORT_SDA &= ~(1<<SDA_PIN_N);
	PORT_SCL |= (1<<SCL_PIN_N);
	while (!(PIN_SCL & (1<<SCL_PIN_N)))
		;
	
	delay(50);
	PORT_SDA |= (1<<SDA_PIN_N);
	delay(50);
	
	if (!(USISR & (1<<USIPF))) {
		return 3;
	}
	
	return 0;
}
