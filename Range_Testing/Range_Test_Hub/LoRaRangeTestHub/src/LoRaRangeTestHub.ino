/* 
 * Project RangeTestHub
 * Author: Bob Glicksman
 * Date: 4/25/24
 * 
 * Description:  This is code for a tester of LoRa signal range.  This code is for the hub, the code 
 *  for the range test sensor is in a companion folder.  The tester is based upona Particle Photon 
 *  (any Arduino can be used in its place).  The LoRa module (RYLR998) is connected as follows:
 *  * Vcc to Photon 3.3v
 *  * GND to Photon GND
 *  * Tx to Photon Rx (Serial1)
 *  * Rx to Photon Tx (Serial1)
 *  * Reset is not connected
 * 
 * The hub uses its LoRa module to listen for a message from the tester.  When a message is 
 *  received, the hub responds with a message of its own.  If the tester receives the response 
 *  message, it is still in range of the hub.
 * 
 * The tester is assigned device number 0 and the hub is assigned device number 1.  The network
 *  number used for testing is 3 and the baud rate to/from the LoRa modem is 115200.  Otherwise,
 *  the default LoRa module values are used.  NOTE:  the hub code does not set up these
 *  values.  The LoRa modules are set up using a PC and an FTDI USB-serial board.
 * 
 * The hub software waits for a received message from the tester.  It then sends back (to the tester)
 *  a response message.  The hub echos its LoRa communications to the USB serial port for debugging
 *  purposes.
 * 
 * version 1.0; 4/25/24
 */

#include "Particle.h"

// The following system directives are for Particle devices.  Not needed for Arduino.
SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

#define VERSION 1.00
#define STATION_NUM 1 // housekeeping; not used ini the code

void setup() {

  pinMode(D7, OUTPUT);
  Serial.begin(9600); // the USB serial port 
  Serial1.begin(115200);  // the LoRa device

  Serial.println("Ready for testing .../n");
  digitalWrite(D7, HIGH);
  delay(500);
  digitalWrite(D7, LOW);
  delay(1000);
  Serial.print("waiting for data ...");
} // end of setup()


void loop() {
  String receivedData = "";  // string to hold the received LoRa dat

  // wait for a message from the tester
  if(Serial1.available()) { // data is in the Serial1 buffer
    delay(5); // wait a bit for the complete message to have been received
    receivedData += Serial1.readString();
    Serial.println("received data = " + receivedData);

    // test for received data from the hub (denoted by "+RCV")
    if(receivedData.indexOf("HELLO") > 0) { // will be -1 if "HELLO" not in the string

      digitalWrite(D7, HIGH);
      receivedData = "";  // clear out the received data string after printing it
      Serial1.println("AT+SEND=0,4,TEST");
      Serial.println("sent:  TEST");
      digitalWrite(D7, LOW);
    }
  } 
} // end of loop()

