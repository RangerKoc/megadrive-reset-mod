#MCU_TARGET = at90s2313
#MCU_TARGET = at90s2333
#MCU_TARGET = at90s4414
#MCU_TARGET = at90s4433
#MCU_TARGET = at90s4434
#MCU_TARGET = at90s8515
#MCU_TARGET = at90s8535
#MCU_TARGET = atmega128
#MCU_TARGET = atmega1280
#MCU_TARGET = atmega1281
#MCU_TARGET = atmega8
#MCU_TARGET = atmega163
#MCU_TARGET = atmega164p
#MCU_TARGET = atmega165
#MCU_TARGET = atmega165p
#MCU_TARGET = atmega168
#MCU_TARGET = atmega168p
#MCU_TARGET = atmega169
#MCU_TARGET = atmega169p
#MCU_TARGET = atmega32
#MCU_TARGET = atmega324p
#MCU_TARGET = atmega325
#MCU_TARGET = atmega3250
MCU_TARGET = atmega328
#MCU_TARGET = atmega328p
#MCU_TARGET = atmega329
#MCU_TARGET = atmega3290
#MCU_TARGET = atmega48
#MCU_TARGET = atmega64
#MCU_TARGET = atmega640
#MCU_TARGET = atmega644
#MCU_TARGET = atmega644p
#MCU_TARGET = atmega645
#MCU_TARGET = atmega6450
#MCU_TARGET = atmega649
#MCU_TARGET = atmega6490
#MCU_TARGET = atmega8
#MCU_TARGET = atmega8515
#MCU_TARGET = atmega8535
#MCU_TARGET = atmega88
#MCU_TARGET = attiny2313
#MCU_TARGET = attiny24
#MCU_TARGET = attiny25
#MCU_TARGET = attiny26
#MCU_TARGET = attiny261
#MCU_TARGET = attiny44
#MCU_TARGET = attiny45
#MCU_TARGET = attiny461
#MCU_TARGET = attiny84
#MCU_TARGET = attiny85
#MCU_TARGET = attiny861

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
