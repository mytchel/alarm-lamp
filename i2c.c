#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#include "lamp.h"

void
init_i2c(void)
{	
	TWSR = 0;
	TWBR = 22;
	TWCR = (1<<TWEN);
}

bool
i2c_message(uint8_t addr,
            uint8_t *sdata, int slen,
            uint8_t *rdata, int rlen)
{
	int i;
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	
	while (!(TWCR & (1<<TWINT)))
		;
	
	if ((TWSR & 0xf8) != TW_START) {
		return false;
	}
	
	if (slen > 0) {
		TWDR = (addr << 1);
		TWCR = (1<<TWINT)|(1<<TWEN);
		
		while (!(TWCR & (1<<TWINT)))
			;
		
		if ((TWSR & 0xf8) != TW_MT_SLA_ACK) {
			return false;
		}
		
		for (i = 0; i < slen; i++) {
			TWDR = sdata[i];
			TWCR = (1<<TWINT)|(1<<TWEN);
			
			while (!(TWCR & (1<<TWINT)))
				;
		
			if ((TWSR & 0xf8) != TW_MT_DATA_ACK) {
				return false;
			}
		}
		
		if (rlen > 0) {
			TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
			
			while (!(TWCR & (1<<TWINT)))
				;
			
			if ((TWSR & 0xf8) != TW_REP_START) {
				return false;
			}
		}
	}
	
	if (rlen > 0) {
		TWDR = (addr << 1) | 1;
		TWCR = (1<<TWINT)|(1<<TWEN);
		
		while (!(TWCR & (1<<TWINT)))
			;
		
		if ((TWSR & 0xf8) != TW_MR_SLA_ACK) {
			return false;
		}
		
		for (i = 0; i < rlen; i++) {
			if (i + 1 == rlen) {
				TWCR = (1<<TWINT)|(1<<TWEN);
			} else {
				TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
			}
			
			while (!(TWCR & (1<<TWINT)))
				;
			
			if (i + 1 == rlen) {
				if ((TWSR & 0xf8) != TW_MR_DATA_NACK) {
					return false;
				}
			} else if ((TWSR & 0xf8) != TW_MR_DATA_ACK) {
				return false;
			}
				
			rdata[i] = TWDR;
		}
	}
	
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	
	return true;
}
