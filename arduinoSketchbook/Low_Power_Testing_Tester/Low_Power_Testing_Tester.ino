/* 
 * Project LowPowerTestingTx
 * Author: Bob Glicksman
 * Date: 5/04/24
 * 
 * Description:  This is code for a tester of LoRa module low power operation.  The tester 
 *  is based upon a Particle Photon that is set up to not need Wifi (any Arduino can be used 
 *  in its place). In addition to the Photon, a N.O. pushbutton switch is connected to ground 
 *  at one end and Photon pin D0 on the other.  The LoRa module (RYLR998) is connected as follows:
 *  * Vcc to Photon 3.3v (through an ammeter to measure supply current to the LoRa module).
 *  * GND to Photon GND
 *  * Tx to Photon Rx (Serial1)
 *  * Rx to Photon Tx (Serial1)
 *  * Reset is not connected
 * 
 * The testing concept is to produce a companion device - the "hub".  The hub uses its LoRa module
 *  to listen for a message from the tester.  When a message is received, the hub prints the message
 *  to the USB serial port so that receipt of a proper message can be confirmed.  NOTE: the same
 *  hub as used for LoRa range testing is used here, but the tester does not wait to receive a 
 *  response message before powering down.
 * 
 * The tester is assigned device number 0 and the hub is assigned device number 1.  The network
 *  number used for testing is 3 and the baud rate to/from the LoRa modem is 115200.  Otherwise,
 *  the default LoRa module values are used.  NOTE:  the tester code does not set up these
 *  values.  The LoRa modules are set up using a PC and an FTDI USB-serial board.
 * 
 * The software senses pressing of the pushbutton and powers up the LoRA module (mode = 0).
 *  After verifying power up ("+OK" received) the tester sends a short message to a companion LoRa
 *  module in the hub.  After an "+OK" response is received from the LoRa module (indicating that
 *  the message has been sent to the hub, the LoRa module is powered down (mode = 1).  
 * 
 * version 1.0; 5/04/24
 */

//#include "Particle.h"

// The following system directives are to disregard WiFi for Particle devices.  Not needed for Arduino.
SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

#define VERSION 1.00
#define STATION_NUM 0 // housekeeping; not used in the code

#define PUSHBUTTON D2

void setup() {
  pinMode(PUSHBUTTON, INPUT_PULLUP);


  Serial1.begin(115200);  // the LoRa device
  Serial1.setTimeout(10);  // a full string is received after 10 ms of no new data from the serial buffer
  delay(1000);
} // end of setup()


void loop() {
  // test for button to be pressed
  if(digitalRead(PUSHBUTTON) == LOW) { // button press detected
    // power up the LoRa module
    Serial1.println("AT+MODE=0");
    waitForOK();

    // send the sensor message
    Serial1.println("AT+SEND=1,5,HELLO");
    waitForOK();

    // power down the LoRa module
    Serial1.println("AT+MODE=1");
    waitForOK();

  }

  while(digitalRead(PUSHBUTTON) == LOW); // wait for button to be released
  delay(100); // wait a little while before sampling the button again

} // end of loop()

void waitForOK() {
  // wait for data in the serial buffer
  while(Serial1.available() <=0 ) {}

  // read out the whole string (note: setTimeout ensure that the whole string is read)
  String receivedData = Serial1.readString();

  // we could test for "+OK" but what would we do if we got an error?
  //  so we are satisfied that the LoRa module responsed to the command message

  return;
} // end of waitForOK()
