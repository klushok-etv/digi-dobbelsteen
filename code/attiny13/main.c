/* Attiny Dice v1 using t13
   Jeremy de Waal - Electrotechnische Vereeniging - 2025
   sub 1k binary developed for use with attiny13
   Feature compatible with version from Ryan Kolk
 */
// C":\Program Files (x86)\Arduino\hardware\tools\avr\bin/avrdude" -C"C:\Program Files (x86)\Arduino\hardware\tools\avr/etc/avrdude.conf" -pattiny13 -cusbasp-clone -B16kHz -Uflash:w:t13dobbelsteen.elf
// C":\Program Files (x86)\Arduino\hardware\tools\avr\bin/avrdude" -C"C:\Program Files (x86)\Arduino\hardware\tools\avr/etc/avrdude.conf" -pattiny13 -cusbasp-clone -B16kHz -Ulfuse:w:0x79:m
// C":\Program Files (x86)\Arduino\hardware\tools\avr\bin/avrdude" -C"C:\Program Files (x86)\Arduino\hardware\tools\avr/etc/avrdude.conf" -pattiny13 -cusbasp-clone -B100kHz -e -Uflash:w:t13dobbelsteen.elf -Ulfuse:w:t13dobbelsteen.elf -Uhfuse:w:t13dobbelsteen.elf -Ueeprom:w:t13dobbelsteen.elf -Ulock:w:t13dobbelsteen.elf

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <avr/power.h>

char _[] EEMEM = "Version 20250525\nDigi Dobbelsteen\nElectrotechnische Vereeniging";
LOCKBITS = 0x3e;


/* function definitions */
uint8_t randomDieRoll();
void rngUpdate();
void auxRngUpdate();
void goToSleep ();
void displayRandomNumber();
void roll();
void setDelayLeft(uint16_t newDelay);
void myDelay(uint16_t delay);



/* Global Variables */
uint16_t rngState = 153;
uint8_t auxRngState = 4; // (VO)
//uint8_t volatile charlieSlot = 0;
enum CharlieState {STATE0, STATE1, STATE2};
enum CharlieState volatile charlieSlot = STATE0;
uint8_t ledOn, ledOff;
uint8_t const ledNumber[6] = {0b00000001, 0b00000010, 0b00000011, 0b00001010, 0b00001011, 0b00001110};
uint8_t volatile lenght, progress;
uint16_t volatile delayLeft;


int main(void)
{
    //clock_prescale_set(clock_div_1);
    TCCR0B = (0 << CS02 | 0 << CS01 | 1 << CS00); // No prescaling // div8
    TIMSK0 = (1 << TOIE0); // Timer/Counter0 Overflow Interrupt Enable
    PCMSK = (1 << PCINT4); // mask pin change interupt 4
    GIMSK = (1 << PCIE); // Pin Change Interrupt Enable
    sei();
    while(1)
    {
        int16_t delayTime;
        roll();
        do // The animation part
        {
            displayRandomNumber();
            progress += 1;
            delayTime = 50 + (350 * progress / lenght);
            //delayTime = 100; //TODO need to fix TODO
            setDelayLeft((uint16_t)(0.001*F_CPU/256)*delayTime);
            while (delayLeft && progress)
            {
                if (!(PINB & (1 << PIN4)))
                {
                    // get some extra user randomness
                    auxRngUpdate();
                }
            }
        }
        while (progress < lenght);
        // the flashing part
        ledOff = ledOn;
        //_delay_ms(300);
        myDelay(0.3*F_CPU/256);
        ledOn = 0;
        //_delay_ms(300);
        myDelay(0.3*F_CPU/256);
        ledOn = ledOff;
        //_delay_ms(300);
        myDelay(0.3*F_CPU/256);
        ledOn = 0;
        //_delay_ms(300);
        myDelay(0.3*F_CPU/256);
        ledOn = ledOff;
        // non block delay 1500
        //delayTime = 1500;
        progress = 1;
        setDelayLeft(1.5*F_CPU/256);
        while (delayLeft && progress);
        if (progress)  // skip sleep if interrupted
        {
            goToSleep();
        }
    }
    return 0;
}

// Timer overflow interrupt
ISR(TIM0_OVF_vect)
{
    //charlieSlot = (charlieSlot + 1) % 3;
    PORTB = (1 << PORTB4);
    uint8_t temp = 0;
    switch (charlieSlot){
    case STATE0:
        charlieSlot = STATE1;
        if (ledOn & 1 << 0)
        {
            temp = 1 << 0;
        }
        else if (ledOn & 1 << 1)
        {
            temp = 1 << 1;
        }
        break;
    case STATE1:
        charlieSlot = STATE2;
        if (ledOn & 1 << 2)
        {
            temp = 1 << 2;
        }
        if ((ledOn & 1 << 0) && (ledOn & 1 << 1)){
            temp = 1 << 1;
        }
        break;
    case STATE2:
    default: // this line saved 2 bytes
        charlieSlot = STATE0;
        if (ledOn & 1 << 3)
        {
            temp = 1 << 3;
        }
        break;
    }
    if (temp == 1 << 0)
    {
        DDRB = (1 << DDB2 | 1 << DDB0);
        PORTB = (1 << PORTB4 | 1 << PORTB2);
    }
    if (temp == 1 << 1)
    {
        DDRB = (1 << DDB0 | 1 << DDB2);
        PORTB = (1 << PORTB4 | 1 << PORTB0);
    }
    if (temp == 1 << 2)
    {
        DDRB = (1 << DDB0 | 1 << DDB1);
        PORTB = (1 << PORTB4 | 1 << PORTB0);
    }
    if (temp == 1 << 3)
    {
        DDRB = (1 << DDB1 | 1 << DDB0);
        PORTB = (1 << PORTB4 | 1 << PORTB1);
    }
    if (delayLeft){
        delayLeft--;
    }
}

void displayRandomNumber()
{
    ledOn = ledNumber[randomDieRoll()];
}

uint8_t randomDieRoll()
{
    uint8_t x;
    do
    {
        rngUpdate();
        rngUpdate();
        rngUpdate();
        x = (rngState ^ auxRngState) & 0b111;
    }
    while (x > 5);
    return x;
}

void rngUpdate()
{
    rngState |= ((rngState << 15) ^ (rngState << 14)) & 1 << 15;
    rngState = (rngState >> 1);
}
void auxRngUpdate()
{
    auxRngState |= ((auxRngState << 6) ^ (auxRngState << 7)) & 1 << 7;
    auxRngState = (auxRngState >> 1);
}

void goToSleep ()
{
    MCUCR|=(1<<SM1);      // Enabling sleep mode and powerdown sleep mode
    MCUCR|= (1<<SE);     //Enabling sleep enable bit
    TIMSK0 = (0 << TOIE0); // Timer/Counter0 Overflow Interrupt Disable
    DDRB = (DDRB & 0xF8) | 0b000;
    do   // alternative for goto
    {
        set_sleep_mode(SLEEP_MODE_PWR_DOWN); // also sets the MCUCR? can be removed?
        sleep_enable();
        sleep_cpu();
        sleep_disable();
    }
    while(PINB & (1 << PIN4));

    TIMSK0 = (1 << TOIE0); // Timer/Counter0 Overflow Interrupt Enable
}

ISR (PCINT0_vect)        // Interrupt service routine
{
    sleep_disable();
    if (!(PINB & (1 << PIN4)))
    {
        roll();
    }
    rngUpdate();
}

void roll()
{
    lenght = 6 + randomDieRoll() + randomDieRoll(); // some number 6 to 16
    progress = 0;
}

void setDelayLeft(uint16_t newDelay)
{
    cli();
    delayLeft = newDelay;
    sei();
}

void myDelay(uint16_t delay)
{
    setDelayLeft(delay);
    while (delayLeft);
}
