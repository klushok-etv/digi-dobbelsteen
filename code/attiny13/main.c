/* Attiny Dice v1 using t13
   Jeremy de Waal - Electrotechnische Vereeniging - 2025
   sub 1k binary developed for use with attiny13
   Feature compatible with version from Ryan Kolk
*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include <avr/power.h>

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
uint16_t rngState = 120;
uint8_t auxRngState = 4; // (VO)
enum CharlieState {STATE0, STATE1, STATE2};
enum CharlieState volatile charlieSlot = STATE0;
uint8_t ledOn, ledOff;
uint8_t const ledNumber[6] = {0b00000001, 0b00000010, 0b00000011, 0b00001010, 0b00001011, 0b00001110}; // lookup table
uint8_t volatile length, progress;
uint16_t volatile delayLeft;


int main(void)
{
    TCCR0B = (0 << CS02 | 0 << CS01 | 1 << CS00); // Timer/Counter0 run without prescaler
    TIMSK0 = (1 << TOIE0); // Timer/Counter0 Overflow Interrupt Enable
    PCMSK = (1 << PCINT4); // mask pin change interupt 4
    GIMSK = (1 << PCIE); // Pin Change Interrupt Enable
    sei(); // global interrupt enable
    while(1)
    {
        int16_t delayTime;
        roll(); // update global variables progress, length
        do // The animation part
        {
            displayRandomNumber();
            progress += 1;
            delayTime = 50 + (350 * progress / length); // could save some space if replaced by lookup table
            setDelayLeft((uint16_t)(0.001*F_CPU/256)*delayTime); // configure delay in ms
            while (delayLeft && progress) // check if delay as elapsed or progress has been reset
            {
                if (!(PINB & (1 << PIN4))) // keep doing as long as button is down
                {
                    // get some extra user randomness
                    auxRngUpdate();
                }
            }
        }
        while (progress < length);
        // the flashing part
        ledOff = ledOn; // store LED enable mask
        myDelay(0.3*F_CPU/256); // delay 300ms
        ledOn = 0;
        myDelay(0.3*F_CPU/256); // delay 300ms
        ledOn = ledOff;
        myDelay(0.3*F_CPU/256); // delay 300ms
        ledOn = 0;
        myDelay(0.3*F_CPU/256); // delay 300ms
        ledOn = ledOff;
        // non block delay for 1500ms
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


ISR(TIM0_OVF_vect) // Timer overflow interrupt
{
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
        delayLeft--; // decrement the delay counter
    }
}

void displayRandomNumber() // set new LED enable mask
{
    // get random number 0-5
    // use lookup table to set leds
    ledOn = ledNumber[randomDieRoll()];
}

uint8_t randomDieRoll() // return a number in range 0-5
{
    uint8_t x;
    do
    {
        rngUpdate(); // update the LFSR a few times
        rngUpdate();
        rngUpdate();
        x = (rngState ^ auxRngState) & 0b111; // truncate number to 3 bits
    }
    while (x > 5); // try again if number is above 5
    return x;
}

void rngUpdate() // perform one iteration on the main LFSR
{
    rngState |= ((rngState << 15) ^ (rngState << 14)) & 1 << 15;
    rngState = (rngState >> 1);
}
void auxRngUpdate() // perform one iteration on the auxiliary LFSR
{
    auxRngState |= ((auxRngState << 6) ^ (auxRngState << 7)) & 1 << 7;
    auxRngState = (auxRngState >> 1);
}

void goToSleep () // sleep the mcu until button pressed
{
    TIMSK0 = (0 << TOIE0); // Timer/Counter0 Overflow Interrupt Disable
    DDRB = 0x00; // ensure all pins are input
    PORTB = 0xff; // set internal pull up to prevent floating pins (saves some power)
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    do   // alternative for goto
    {
        sleep_enable();
        sleep_cpu();
        sleep_disable();
    }
    while(PINB & (1 << PIN4));

    TIMSK0 = (1 << TOIE0); // Timer/Counter0 Overflow Interrupt Enable
}

ISR (PCINT0_vect) // pin change Interrupt
{
    sleep_disable(); // exit sleep mode in case mcu was in sleep mode
    if (!(PINB & (1 << PIN4))) // check if button was pressed
    {
        roll(); // reset the global variables
    }
    rngUpdate(); // update LFSR for good measure
}

void roll() // sets amount of numbers to show and clears progress
{
    length = 6 + randomDieRoll() + randomDieRoll(); // generate some number 6 to 16
    progress = 0; // clear progress
}

void setDelayLeft(uint16_t newDelay) // safe way to acces variable
{
    cli();
    delayLeft = newDelay; // set new value
    sei();
}

void myDelay(uint16_t delay) // delay based on Timer/Counter0
{
    setDelayLeft(delay); // configure delay duration
    while (delayLeft); // stall until the timer interrupt has decremented this to zero
}
