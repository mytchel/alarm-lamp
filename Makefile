DEVICE = attiny85
PROGRAMMER = usbtiny
CLOCK = 1000000L

CFLAGS = -Wall -O0 -mmcu=$(DEVICE) -DF_CPU=$(CLOCK)
LDFLAGS =

AVRDUDE = avrdude -c $(PROGRAMMER) -p $(DEVICE)
CC = avr-gcc
LD = avr-ld
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump

SRC = main.c 
OBJ = $(SRC:%.c=%.o)

.SUFFIXES:
.SUFFIXES: .c .S .h .o .elf .hex .list

all: out.hex out.list

.elf.list:
	$(OBJDUMP) -S $< > $@

.elf.hex:
	$(OBJCOPY) -j .text -j .data -O ihex $< $@
	
out.elf: $(OBJ)
	$(LD) -o out.elf $(OBJ) $(LDFLAGS)


flash: out.hex
	$(AVRDUDE) -U flash:w:out.hex:i

clean: 
	rm -f *.elf *.hex *.list $(OBJ)
