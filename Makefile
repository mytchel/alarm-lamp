DEVICE = attiny85
PROGRAMMER = usbtiny
CLOCK = 1000000L

CFLAGS = -Wall -mmcu=$(DEVICE) -DF_CPU=$(CLOCK)
LDFLAGS =

AVRDUDE = avrdude -c $(PROGRAMMER) -p $(DEVICE)
CC = avr-gcc
LD = avr-ld

SRC = main.c 
OBJ = $(SRC:%.c=%.o)

all: out.hex

out.elf: $(OBJ)
	$(LD) -o out.elf $(OBJ) $(LDFLAGS)

out.hex: out.elf
	avr-objcopy -j .text -j .data -O ihex out.elf out.hex

flash: out.hex
	$(AVRDUDE) -U flash:w:out.hex:i

clean: 
	rm -f out.elf out.hex $(OBJ)
