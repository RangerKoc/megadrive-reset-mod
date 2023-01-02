## Megadrive Reset Mod
Is a mod for SEGA Megadrive/Genesis with in-game reset and region switching (language and 50/60 Hz framerate) functions based on Arduino compatible boards.

### Features
- In-game reset using player one controller:
  - <kbd>START + A + B + C</kbd> - Soft Reset
  - <kbd>START + A + B + C + DOWN</kbd> - Hard Reset (if enabled, read below)
- Region switching using player one controller:
  - <kbd>START + B + LEFT</kbd>  - Switch to North American region (Blue)
  - <kbd>START + B + RIGHT</kbd> - Switch to European region (Red)
  - <kbd>START + B + UP</kbd>    - Switch to Japanese region (Purple)
  - <kbd>START + B + DOWN</kbd>  - Switch to Asian region (Green)
- Only language switching mode (if configured, read below):
  - <kbd>START + B + LEFT</kbd>  - Switch to English (Red)
  - <kbd>START + B + RIGHT</kbd> - Switch to Japan (Green)
- Only framerate switching mode (if configured, read below):
  - <kbd>START + B + UP</kbd>   - Switch to 60 Hz (Red)
  - <kbd>START + B + DOWN</kbd> - Switch to 50 Hz (Green)
- Region switching by keep pushed <kbd>RESET</kbd> button to cycle through regions/modes.
- Last used mode is saving and reused on next power on.
- Supported only RGB led.
- Configurable mode switching - only language switching or only 50/60 Hz framerate switching (read below).

### Building
Source code can be used by Arduino IDE or can be compiled using avr-toolchain.

### Configuration
Program behavior can be configured by defines that can be found in main.c file:
- `ONLY_LANGUAGE_SWITCH` - Use only language switching;
- `ONLY_FREQUENCY_SWITCH` - Use only framerate switching;
- `USE_RGB_LED` - Using RGB LED;
- `RGB_LED_ANODE` - Using RGB LED with common ANODE, else with common CATHODE;
- `POLL_BY_MCU_ENABLED` - If no pad polling by Megadrive, uC will poll pad itself (usefull if console is halted);
- `ATMEGA_CLONE_USED` - Using cloned ATmega uC (eg LGT328P), it's adding few NOP's to add some delay before reading buttons state in interrupt handler.
>Note: It's possible need to add few NOP's manually for uC you using.

### Installation
Here is a simplest connection pinout Arduino board to Megadrive board (without illustration):
| Arduino   |    | Megadrive board  |
|-----------|----|------------------|
|       5V  | <- | Pad Port Pin 5 or another +5V source point (VCC) |
|       GND | <- | GND |
| (PD2) D2  | <- | Pad Port Pin 7 (TH, SEL) |
| (PD3) D3  | <- | Reset Button Pin/Leg (Reset Input) |
| (PD4) D4  | <- | Pad Port Pin 6 (TL, DAT4) |
| (PD5) D5  | <- | Pad Port Pin 9 (TR, DAT5) |
| (PD6) D6  | -> | Reset Button Point on Board (Soft Reset Output) |
| (PD7) D7  | -> | Cart Slot Pin B2 (#MRES, Hard Reset) `optional` |
| (PB0) D8  | <- | Pad Port Pin 1 (DAT0) |
| (PB1) D9  | <- | Pad Port Pin 2 (DAT1) |
| (PB2) D10 | <- | Pad Port Pin 3 (DAT2) |
| (PB3) D11 | <- | Pad Port Pin 4 (DAT3) |
| (PB4) D12 | -- | `X` |
| (PC0) A0  | -> | LED Blue  `optional` |
| (PC1) A1  | -> | LED Green `optional` |
| (PC2) A2  | -> | LED Red   `optional` |
| (PC3) A3  | -> | JP1/2 (Language) |
| (PC4) A4  | -> | JP3/4 (Video Mode) |
| (PC5) A5  | -- | `X` |