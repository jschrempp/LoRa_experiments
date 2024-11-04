
/* Low_power)ATmega328_LoRa_integration
 *  Program to test the integration of ATmega328(P), RYLR998 LoRa module, and low power contact sensor, all with power down
 *    to minimal power while waiting for the contact to close.  Measured current when the circuit is powered down is about
 *    15 - 16 microampers.
 *    
 *  The interrupt used is interrupt 0, which is pin 2 on Arduino or pin 4 on
 *    the ATmega328.
 *    
 *  This is a low power test, based upon published work by Nick Gammon (https://www.gammon.com.au/power). 
 *  
 *  If the line "#define DEBUG" is not ocmmented out, the LEDs will flash out the status of LoRa command responses and also
 *    indicate when the message is sent and the circuit is about to return to low power.  Normally, this line of code is 
 *    commented out, in which case the only use of the LEDs is to blink twice when setup() is complete.  The LoRa module
 *    is put into its sleep mode in setup().
 *    
 *    After setup(), the initial code in loop puts the microcontroller itself to sleep.  When interrupted by a falling edge on
 *    interrupt 0, the microcontroller wakes up.  Upon waking up, the microcontroller:
 *    - wakes up the LoRa module
 *    - sends out the contact closed LoRa message
 *    - puts the LoRa module back to sleep.
 *    
 *    The microcontroller then returns to sleep mode itself, until the contact is again closed.  NOTE: it does not 
 *    matter how long the sensor contact remains closed.  At some point, it must return to open again before it can be 
 *    closed and generate a falling edge interrupt.
 *  
 *  HARDWARE SETUP:
 *    A 3 volt (nominal) power supply powers all electronics.  Note that the LoRa module consumes huge spikes in power, so
 *    good power and ground distribution as well as copious decoupling is essental for reliable hardware operation.
 *    
 *    WIRING:
 *    
 *      Power Connections.
 *    
 *    - Microcontroller pins 7 and 20 are connected to Vcc (3 volt supply)
 *    - Microcontroller pins 8 and 22 are connected to GND
 *    - LoRa VDD pin is connected to Vcc (3 volt supply)
 *    - LoRa GND pin is connected to GND
 *    - 74HC14 chip pin 14 is connected to Vcc (3 volt supply)
 *    - 74HC14 chip pin 7 is connected to GND
 *    
 *      LoRa connections.
 *      
 *    - Microcontroller pin 2 (RXD) is connected to LoRa TXD
 *    - Microcontroller pin 3 (TXD) is connected to LoRa RXD
 *    - LoRa NRST is left open
 *    
 *      Reset.
 *    - Microcontroller pin 1 (NRST) is jumpered to GND to reset the microcontroller.  Otherwise, this pin is left open.
 *    
 *    
 *      Contact sensor connections.  NOTE: a NO pushbutton is used to simpulate a magnetic red switch contact sensor.
 *      
 *      - one end of the contact sensor is connected to GND
 *      - the other end of the contact sensor is connected to a 1 Mohm resistor.
 *      - the other end of the 1 Mohm resistor is connected to VCC (3 volt supply)
 *      - a 0.1 uF capacitor is connected across the contact sensor (between the sense line and GND)
 *      - the sense line (junction of contact sensor, 1 Mohm resistor and 0.1 uF capacitor) is connected to 74HC14 pin 1 (INV 1 input)
 *      - 74HC14 pin 2 is connected to 74HC14 pin 3 (INV 1 output to INV 2 input)
 *      - 74HC14 pin 4 is connected to microcontroller pin 4 (intrurrupt 0, Arduino DIO pin 2).
 *    
 *    version 1.10, by: Bob Glicksman, 11/03/24
 *      this version adds support for green and red LEDs
 *    
 *    version 1.20, by Bob Glicksman, 11/03/24
 *      this version delays 500 ms after transmission before puting LoRa module to sleep
 *    (c) 2024, Bob Glicksman, Jim Schrempp, Team Practical Projects.  All rights reserved.
 */

#include <avr/sleep.h>  // the official avr sleep library

#define VERSION 1.20

//#define DEBUG

// CONSTANTS
const int BUTTON_PIN = 2; // the pushbutton is on digital pin 2 which is chip pin 4
const int GRN_LED_PIN = 9;  // the Green LED is on digital pin 9 which is chip pin 15
const int RED_LED_PIN = 8;  // the Red LED is on digital pin 8 which is chip pin 14

void setup() {

  pinMode(BUTTON_PIN, INPUT); // Interrupt 0 is Arduino pin 2 is chip pin 4, external pullup with schmitt trigger is used.
  pinMode(GRN_LED_PIN, OUTPUT); // the green LED pin is Arduino DIO 9, chip pin 15.
  pinMode(RED_LED_PIN, OUTPUT); // the red LED pin is Arduino DIO 8, chip pin 14.

  Serial.begin(38400);  // the LoRa device baud rate
  Serial.setTimeout(10);  // a full string is received after 10 ms of no new data from the LoRa device

  // put the LoRa module to sleep
  Serial.println("AT");
  waitForOK();
  Serial.println("AT+MODE=1"); 
  if(waitForOK() == -1) { // only flash the LED if did not get +OK
    #ifdef DEBUG
    blinkLed(RED_LED_PIN, 3);
    #endif
  }

  // blink the LEDs to show that setup() is complete
  blinkLed(GRN_LED_PIN, 2);
  blinkLed(RED_LED_PIN, 2);
  
  // change the LED pins for lowest power consumption while sleeping
  pinMode(GRN_LED_PIN, INPUT);  
  pinMode(RED_LED_PIN, INPUT);

} // end of setup()

void loop() {
  // the first thing that we do is put the microcontroller to deep sleep; awake only on falling edge on interrupt 0.
  
  // disable the adc
  ADCSRA = 0;
  
  set_sleep_mode (SLEEP_MODE_PWR_DOWN); // the lowest possible low power mode
  sleep_enable(); // not sleeping yet!

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

  //  everything should now be in deep sleep.
  // Interrupt 0 wakes up the ATmega328 - send the message and go back to sleep

  #ifdef DEBUG
  pinMode(GRN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  #endif
  
  // wake up the LoRa module
  Serial.println("AT+MODE=0");  // mode 0 is the normal tranceiver mode of the LoRa module
  waitForOK();  // something other than +OK comes back after the LoRa is powered down
  //  try again to make sure that we are powered up - got +OK
  Serial.println("AT+MODE=0");    
  if(waitForOK() == -1) { // only flash the LED if did not get +OK
    #ifdef DEBUG
    blinkLed(RED_LED_PIN, 1);
    #endif
  }

  // send out the contact closed message
  Serial.println("AT+SEND=57248,28,HELLO m: 2 rssi: -21 snr: 11");  // just a dummy message for testing
  if(waitForOK() == -1) { // only flash the LED if did not get +OK
    #ifdef DEBUG
    blinkLed(RED_LED_PIN, 2);
    #endif
  }

  delay(500); // need to wait for xmit to complete before sleeping the LoRa module

  // put the LoRa module to sleep
  Serial.println("AT+MODE=1");
  if(waitForOK() == -1) { // only flash the LED if did not get +OK
    #ifdef DEBUG
    blinkLed(RED_LED_PIN, 3);
    #endif
  }
    
  // indicate that the message sending process is complete
  #ifdef DEBUG
  blinkLed(GRN_LED_PIN, 5); 
  #endif

  #ifdef DEBUG
  // set the led pins to INPUT for the lowest power consumption when sleeping
  pinMode(GRN_LED_PIN, INPUT);  // set the LED_PIN to INPUT for the lowest power consumption when sleeping
  pinMode(RED_LED_PIN, INPUT);
  #endif
  
} // end of loop()

// blinkLed(): blinks the indicated LED "times" number of times

void blinkLed(int ledpin, int times) {
  for(int i = 0; i < times; i++) {
    digitalWrite(ledpin, HIGH);
    delay(250);
    digitalWrite(ledpin, LOW);
    delay(250);
  }
  return;
} // end of blinkLed()

// the interrupt service routine that wakes up the microcontroller

void isr () {
  sleep_disable();  // cancel sleep mode for now
  detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));  // preclude more interrupts due to bounce, or other
  
} // end of isr()

// waitForOK():  processes the response from the LoRa module.  It shoudl always be "+OK"
//  returns 0 if response was correct.  Otherwise, returns -1.  Note: blocks waiting for some (any) response
//  from the LoRa module.  A non-blocking timeout should be implemented for production.

int waitForOK() {
  // wait for data in the serial buffer
  while(Serial.available() <=0) {}  // loop until data in the buffer

  // read out the whole string (note: setTimeout(10) ensures that the whole string is read
  String receivedData = Serial.readString();

  // test for "+OK"
  if(receivedData.indexOf("+OK") >= 0) { // got an +OK
    return 0;
  } else {
    return -1;
  }

} // end of waitForOK()
