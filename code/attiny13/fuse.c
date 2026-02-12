#include <avr/io.h>

FUSES = {
    //.low = LFUSE_DEFAULT,
    //.high = HFUSE_DEFAULT
    //.low = 0x65,
    .low = 0x39,
    .high = 0xFF
};
