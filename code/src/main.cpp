/* Attiny Dice v1
  Ryan Kolk - Electrotechnische Vereeniging - 2024
  ATtiny85 @ 8 MHz (internal oscillator; BOD disabled)
   
  CC BY 4.0
  Licensed under a Creative Commons Attribution 4.0 International license: 
  http://creativecommons.org/licenses/by/4.0/

  Atmet Attiny 25/45/85 pinout
                  +-\/-+
  Ain0 (D 5) PB5  1|    |8  Vcc
  Ain3 (D 3) PB3  2|    |7  PB2 (D 2) Ain1
  Ain2 (D 4) PB4  3|    |6  PB1 (D 1) pwm1
            GND  4|    |5  PB0 (D 0) pwm0
                  +----+
*/

#include <Arduino.h>
#include <avr/sleep.h>    // Sleep Modes
#include <avr/power.h>    // Power management

void goToSleep ();

//Initial brightness levels for the rows
uint8_t Level[4] = {0,0,0,0};

//Matrix to map numbers 1-6 to the correct leds
const bool Number[6][4] = {
  {1,0,0,0},
  {0,1,0,0},
  {1,1,0,0},
  {0,1,0,1},
  {1,1,0,1},
  {0,1,1,1},
};

static uint8_t row, ramp;

ISR (PCINT0_vect)        // Interrupt service routine 
{
  MCUCR&=~(1<<SE);      //Disabling sleep mode inside interrupt routine
}

// Timer overflow interrupt
ISR(TIM1_COMPA_vect) {
  static uint8_t dir, out;
  ramp = (ramp+1) & 0x3F;             // Count from 0 to 63
  if (ramp == 0) {                    // Turn on row leds
    row = (row + 1) & 0x03;
    if(row == 0){
      dir=0b101;
      out=0b100;
    } else if(row == 1){
      dir=0b101;
      out=0b001;
    } else if(row == 2){
      dir=0b011;
      out=0b001;
    } else if(row == 3){
      dir=0b011;
      out=0b010;
    }
  }
  if (Level[row] == ramp){            // Turn off LED when duty cycle reached
    out = 0b000;
    dir=0b000;
  }
  
  DDRB = (DDRB & 0xF8) | dir;         // Set Pin direction. 1 is output, 0 is input (High impedance)
  PORTB = (PORTB & 0xF8) | out;       // Set Pin. 1 is high, 0 is low
}

void setup() {
  // Set up Timer/Counter1 to contol the LEDs
  TCCR1 = 1<<CTC1 | 1<<CS10;          // Divide by 2
  GTCCR = 0;                          // No PWM
  OCR1A = 0;
  OCR1C = 125-1;                      // 16kHz
  TIMSK = TIMSK | 1<<OCIE1A;          // Compare Match A interrupt

  //Setup sleep with interrupt on PB4
  pinMode (PB4, INPUT);     
  digitalWrite (PB4, HIGH);           // Set internal pullup
  PCMSK  |= bit (PCINT4);             // interrupt on PB4
  GIFR   |= bit (PCIF);               // Clear any outstanding interrupts
  GIMSK  |= bit (PCIE);               // Enable pin change interrupts 
}

void loop () {
  int randomness = analogRead(PB3);   // Get a bit of randomness from analog pin that is not being used
  randomSeed(randomness*millis());    // Set seed for the random number generator
  int num = random(0,6);              // Generate number between 0-5

  for(int i=0; i<4; i++){
    Level[i] = Number[num][i]*63;
  }
  
  delay(1000);
  for(int i=0; i<4; i++){
    Level[i] = 0;
  }


  goToSleep();
}

void goToSleep () {
  MCUCR|=(1<<SM1);      // Enabling sleep mode and powerdown sleep mode
  MCUCR|= (1<<SE);     //Enabling sleep enable bit
  DDRB = (DDRB & 0xF8) | 0b000;
  PORTB = (PORTB & 0xF8) | 0b000;
  delay(10);
  ramp=0;
  row=0;
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  ADCSRA = 0;                         // turn off ADC
  power_all_disable ();               // power off ADC, Timer 0 and 1, serial interface
  sleep_enable();
  sei();  
  sleep_cpu();   
  cli();                            
  sleep_disable();  
  sei();       
  power_all_enable();                 // power everything back on
}