
/* Interrupt_Test2
 *  Program to test powering down an ATmega328 and wake it up with an edge truggered interrupt.
 *  The interrupt used is interrupt 0, which is pin 2 on Arduino or pin 4 on
 *    the ATmega328.
 *    
 *  This is a low power test, based upon published workm by Nick Gammon (https://www.gammon.com.au/power). 
 *  
 *  The microcontroller goes through setup() and blinks a D9 LED 3 times slowly.  It enters loop() and sets up deep
 *    sleep mode (as deep as possible) and goes to sleep.  Before doing so,interrupt 0 ia attached as falling edge truggered.  
 *    A pushbutton is attached to interrupt 0 which grounds the pulled-up line when pressed.  A falling edge (pressing the button) should
 *    then wake up the microcontroller which causes a triple fast blink of the LED, and continues with loop() to go back
 *    to deep sleep.
 *    
 *    In this test, any press of the button should cause one triple blink of the LED, regardless of how
 *    long that the button is pressed.  After the triple blink and until the button is pressed again, the micontroller should
 *    be in deep sleep.  TEST RESULT: verified.  Deep sleep measures a few microamps using a ATmega328 not-P (accurate measurement
 *    is not possible with my cheap multimeter).
 *    
 *    NOTE: The initial tests performed on 8/1/24 used an internal pullup on the interrupt 0 line.  Deep sleep current measured
 *    oen to two tenths of a microamp when the button was not pressed (contact open).  When the button was pressed (contact closed),
 *    the deep sleep current went up to about 85 microamps.  This is due to the internal pullup being about 40 Kohms.
 *    
 *    8/5/24:  I changed the code to not use the internal pullup (version 1.1).  I used an external 1 Mohm resistor as the pullup, with a 0.1 uF
 *    capacitor between the switched line and ground.  The capacitor is to filter out noise.  Because the RC combination results in 
 *    a long rise time when the switch is released, I wired the switched line to pin 1 of a 74HC14 schmitt trigger inverter input.
 *    I wired the output (74HC14 pin 2) to the input of the next inverter (74HC14 pin 3) and took the output of this second inverter
 *    (74HC14 pin 4) as the interrup signal (microcontroller pin 4).  I connected thew inputs to the 4 remaining inverters to 
 *    ground to minimize supply current draw.  This reduced the deep sleep current draw to about 3.4 microamps
 *    when the contact is closed!  NOTE:  it is possible that external schmitt trigger gates are not required as the microcontroller
 *    may have schmitt triggers on the input pins.  
 *    
 *    version 1.10, by: Bob Glicksman, 08/05/24
 *    (c) 2024, Bob Glicksman, Jim Schrempp, Team Practical Projects.  All rights reserved.
 */

#include <avr/sleep.h>

// CONSTANTS
const int BUTTON_PIN = 2; // the pushbutton is on digital pin 2 which is chip pin 4
const int LED_PIN = 9;  // the LED is on digital pin 9 which is chip pin 15



void setup() {
//  pinMode(BUTTON_PIN, INPUT_PULLUP); // Interrupt 0 is pin 2 is chip pin 4
  pinMode(BUTTON_PIN, INPUT); // Interrupt 0 is pin 2 is chip pin 4, external pullup with schmitt trigger is used.
  pinMode(LED_PIN, OUTPUT); // the LED pin is 9, chip pin 15.
  blink3(500);  // indicate that setup() is over and loop() begins
  pinMode(LED_PIN, INPUT);  // change the LED pin for lowest power consumption while sleeping

} // end of setup()

void loop() {
  // disable the adc
  ADCSRA = 0;
  
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  noInterrupts(); // disable interrupts until we actually go to sleep.
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), isr, FALLING); // ready the wakeup interrupt
  EIFR = bit(INTF0);  // clear flag for interrupt 0

  // turn off brown-out enable in software
  // BODS must be set to one and and BODSE must be set to zero within 4 clock cycles
  MCUCR = bit(BODS) | bit (BODSE);
  // the BODS bit is automatically cleared after 3 clock cycles
  MCUCR = bit(BODS);

  interrupts(); // enable interrupts just before sleeping
  sleep_cpu();

  // This part of loop() happens only after an interrupt 0

  pinMode(LED_PIN, OUTPUT);
  blink3(250);  // indicate that the cpu is awake
  delay(10);  // wait a few milliseconds before changing pinMode()
  pinMode(LED_PIN, INPUT);
  
} // end of loop()

// simple function to blink the LED 3 times at interval "speed" (ms)
void blink3(int speed) {
  for (int i = 0; i < 3; i++) {
     digitalWrite(9, HIGH);
     delay(speed);
     digitalWrite(9, LOW);
     delay(speed);
  }
} // end of blink3()

// the interrupt service routine that wakes up the microcontroller
void isr () {
  sleep_disable();  // cancel sleep mode for now
  detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));  // preclude more interrupts due to bounce, or other
  
} // end of isr()
