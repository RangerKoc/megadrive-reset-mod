@echo off

rem del /f /q /s *.bak
rem del /f /q /s *.o
rem del /f /q /s *.d
rem del /f /q /s *.lst
rem del /f /q /s *.lss
rem del /f /q /s *.bin
rem del /f /q /s *.hex
rem del /f /q /s *.map
rem del /f /q /s *.eep
rem del /f /q /s *.elf
rem del /f /q /s *.eps
rem del /f /q /s *.srec
rem del /f /q /s *.sym

make clean

rmdir /s /q obj

rem pause
