# Digi Dobbelsteen

Digi Dobbelsteen is a digital dice project based on the attiny85 that uses 7 LEDs and a push button.

## Features

- 7 LEDs to display the result of the dice roll
- A push button to roll the dice
- A vibration sensor to roll the dice when the device is shaken

## Usage

The code was written in PlatformIO, so you can use that to compile and upload the code to the attiny85.

The schematic was made in KiCad 7.0, so you can use that to open the schematic and PCB files.

## Schematic

![Schematic](./Schematic.png "width=100px")

## Charlieplexing

![Charlieplexing](./Charlieplexing.png)

## PCB

![PCB](./PCB.png)

## Programing

A precompiled binary is available for the attiny13 under releases. Upload to the chip using the following command:

```console
avrdude -p attiny13 -c usbasp-clone -B 100kHz -e -U flash:w:t13dobbelsteen.elf -U lfuse:w:t13dobbelsteen.elf -U hfuse:w:t13dobbelsteen.elf -U eeprom:w:t13dobbelsteen.elf -U lock:w:t13dobbelsteen.elf
```
