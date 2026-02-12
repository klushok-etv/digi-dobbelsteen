#include <avr/io.h>
#include <avr/eeprom.h>

FUSES = {
    .low = 0x39, // 4.8 MHz, Preserve EEPROM
    .high = 0xFF // disable Debug Wire
};

char _[] EEMEM = "Digi Dobbelsteen - Electrotechnische Vereeniging - Klushok (VO)";
LOCKBITS = 0x3e; // programing disabled, verification enabled

const char myAppendedText[] = "version20260212"; // ends up stored near end of flash
