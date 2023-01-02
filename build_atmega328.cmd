@echo off

set srcname=main
set basename=megadrive-reset-mod
set mcu=atmega328

@echo on

avr-g++ -g -Wall -Os -mmcu=%mcu% -DF_CPU=16000000L -c -o %srcname%.o %srcname%.c

avr-g++ -g -Wall -Os -mmcu=%mcu% -DF_CPU=16000000L -o %basename%.elf %srcname%.o

avr-objcopy -j .text -j .data -O ihex   %basename%.elf %basename%.hex

avr-objcopy -j .text -j .data -O binary %basename%.elf %basename%.bin

del /f /q %srcname%.o

pause
