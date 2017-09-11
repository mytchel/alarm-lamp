CLOCK = 6000000UL

CFLAGS = -Wall -O0
CFLAGS += -mmcu=atmega328 -DF_CPU=$(CLOCK)

LDFLAGS = -mmcu=atmega328

AVRDUDE = avrdude -c usbtiny -p m328
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump

SRC = main.c display.c clock.c i2c.c
OBJ = $(SRC:%.c=%.o)

.SUFFIXES:
.SUFFIXES: .c .S .h .o .elf .hex .list

all: out.hex out.list

.elf.list:
	$(OBJDUMP) -j .text -j .data -S $< > $@

.elf.hex:
	$(OBJCOPY) -j .text -j .data -O ihex $< $@
	
out.elf: lamp.h $(OBJ)
	$(CC) -o out.elf $(OBJ) $(LDFLAGS)


flash: out.hex
	$(AVRDUDE) -U flash:w:out.hex:i

clean: 
	rm -f *.elf *.hex *.list $(OBJ)
