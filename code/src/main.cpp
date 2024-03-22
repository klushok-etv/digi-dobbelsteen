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

int animateRolling();
void nonBlockingDelay(int ms);
void flashNumber(int num);
void displayNumber(int num);
//Initial brightness levels for the rows
bool Level[4] = {0,0,0,0};
const uint8_t animation[4] = {1,2,3,2};

//Matrix to map numbers 1-6 to the correct leds
const bool Number[6][4] = {
  {1,0,0,0},
  {0,1,0,0},
  {1,1,0,0},
  {0,1,0,1},
  {1,1,0,1},
  {0,1,1,1},
};
bool interrupt = false;
bool awake = true;
static uint8_t row, ramp;
void(* resetFunc) (void) = 0;

ISR (PCINT0_vect)        // Interrupt service routine 
{
  MCUCR&=~(1<<SE);      //Disabling sleep mode inside interrupt routine
  if(awake && digitalRead(PB4) == LOW){
    // resetFunc();
    interrupt=true;
  }
}


// Timer overflow interrupt
ISR(TIM1_COMPA_vect) {
  static uint8_t dir, out;
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
  if (Level[row] == 0){            // Turn off LED if brightness is 0
    out = 0b000;
    dir=0b000;
  }
  
  DDRB = (DDRB & 0xF8) | dir;         // Set Pin direction. 1 is output, 0 is input (High impedance)
  PORTB = (PORTB & 0xF8) | out;       // Set Pin. 1 is high, 0 is low
}

void setup() {
  if(F_CPU == 8000000) clock_prescale_set(clock_div_1); // Set clock to 8MHz

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
  GIMSK |= (1 << PCIE);
  int randomness = analogRead(PB3);// Get a bit of randomness from analog pin that is not being used
  randomSeed(randomness);    // Set seed for the random number generator
}
void loop () {
  int number = animateRolling();

  delay(300);
  flashNumber(number);
  interrupt=false;
  nonBlockingDelay(1500);
  if(interrupt){
    interrupt = false;
    return;
  }
  goToSleep();
  
}

int animateRolling(){
  int randomNumber = 0;
  int length = random(6,15);
  for(int i=0; i<length; i++){
    randomNumber = random(0,6);
    displayNumber(randomNumber);
    nonBlockingDelay(50 + (i * 350 / length));
    if(interrupt){
      length = random(6,15);
      i=0;
      interrupt = false;
      delay(50);
    }
  }
  return randomNumber;
}

void nonBlockingDelay(int ms) {
  for(int i=0; i<ms; i++){
    delay(1);
    if(interrupt)
     return;
  }
  return;
}

void flashNumber(int num){
  for(int i=0; i<2; i++){
    displayNumber(num);
    delay(400);

    displayNumber(-1);
    delay(400);
  }
  displayNumber(num);
}

void displayNumber(int num){
  if(num == -1){
    for(int i=0; i<4; i++){
      Level[i] = 0;
    }
  } else {
    for(int i=0; i<4; i++){
      Level[i] = Number[num][i];
    }
  }
  
}

void goToSleep () {
  for(int i=0; i<4; i++){
    Level[i] = 0;
  }
  MCUCR|=(1<<SM1);      // Enabling sleep mode and powerdown sleep mode
  MCUCR|= (1<<SE);     //Enabling sleep enable bit
  DDRB = (DDRB & 0xF8) | 0b000;
  PORTB = (PORTB & 0xF8) | 0b000;
  delay(10);
  ramp=0;
  row=0;
  awake = false;
  backtosleep: 
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
  if(digitalRead(PB4) == HIGH){
    goto backtosleep;
  }
  awake = true;
}