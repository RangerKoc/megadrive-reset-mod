#MCU_TARGET = atmega168
#MCU_TARGET = atmega168p
MCU_TARGET = atmega328
#MCU_TARGET = atmega328p

F_CPU = 16000000L

PRG = megadrive-reset-mod
OBJ = main.o

CC = avr-g++
OBJCOPY = avr-objcopy

#OPTIMIZE = -O3
OPTIMIZE = -Os

# Override is only needed by avr-lib build system.
override CFLAGS = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) -DF_CPU=$(F_CPU)

build: $(PRG).elf hex bin

# Default target
all: clean build

$(PRG).elf: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^
	rm -f $^

hex: $(PRG).hex
bin: $(PRG).bin

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

clean:
	rm -rf *.bak *.o *.d *.lst *.lss *.bin *.hex *.map *.eep *.elf *.eps *.srec *.sym
