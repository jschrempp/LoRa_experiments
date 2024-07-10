/* 
 * Project RangeTestTx
 * Author: Bob Glicksman
 * Date: 4/25/24
 * 
 * Description:  This is code for a tester of LoRa signal range.  The tester is based upon
 *  a Particle Photon that is set up to not need Wifi (any Arduino can be used in its place).
 *  In addition to the Photon, a N.O. pushbutton switch is connected to ground at one end
 *  and Photon pin D0 on the other.  The LoRa module (RYLR998) is connected as follows:
 *  * Vcc to Photon 3.3v
 *  * GND to Photon GND
 *  * Tx to Photon Rx (Serial1)
 *  * Rx to Photon Tx (Serial1)
 *  * Reset is not connected
 * 
 * The testing concept is to produce a companion device - the "hub".  The hub uses its LoRa module
 *  to listen for a message from the tester.  When a message is received, the hub responds with a
 *  message of its own.  If the tester receives the response message, it is still in range of the
 *  hub.
 * 
 * The tester is assigned device number 0 and the hub is assigned device number 1.  The network
 *  number used for testing is 3 and the baud rate to/from the LoRa modem is 115200.  Otherwise,
 *  the default LoRa module values are used.  NOTE:  the tester code does not set up these
 *  values.  The LoRa modules are set up using a PC and an FTDI USB-serial board.
 * 
 * The software senses pressing of the pushbutton and sends a short message to a companion LoRa
 *  module in the hub.  The message is also printed on the Photon's USB serial port for debugging
 *  purposes.  The unit then waits 3 seconds for a response.  If a response is received 
 *  (data received from the hub, beginning with +RCV), the D7 LED is flashed three times.  If no
 *  data is received from the hub (distance too far), the D7 LED is flashed once.
 * 
 * version 1.0; 4/25/24
 */

#include "Particle.h"

// The following system directives are to disregard WiFi for Particle devices.  Not needed for Arduino.
SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

#define VERSION 1.00
#define STATION_NUM 0 // housekeeping; not used ini the code


// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

// function to send AT commands to the LoRa module
// returns 0 if successful, -1 if not successful
// prints message and result to the serial monitor
int LoRaCommand(String command) {
    Serial.println("");
    Serial.println(command);
    Serial1.println(command);
    delay(50); // wait for the response
    int retcode = 0;
    if(Serial1.available()) {
        String receivedData = Serial1.readString();
        // received data has a newline at the end
        Serial.print("received data = " + receivedData);
        if(receivedData.indexOf("ERR") > 0) {
            Serial.println("LoRa error");
            retcode = -1;
        } else {
            Serial.println("command worked");
            retcode = 0;
        }
    } else {
        Serial.println("No response from LoRa");
        retcode =  -1;
    }
    return retcode;
};

void setup() {
    pinMode(D0, INPUT_PULLUP);
    pinMode(D7, OUTPUT);
    Serial.begin(9600); // the USB serial port 
    Serial1.begin(115200);  // the LoRa device

    // READ LoRa Settings
    Serial.println("");
    Serial.println("");
    Serial.println("-----------------");
    Serial.println("Reading back the settings");

    bool error = false;
    if(LoRaCommand("AT+NETWORKID?") != 0) {
        Serial.println("error reading network id");
        error = true;
    }
    if(LoRaCommand("AT+ADDRESS?") != 0) {
        Serial.println("error reading device address");
        error = true;
    }
    if(LoRaCommand("AT+PARAMETER?") != 0) {
        Serial.println("error reading parameters");
        error = true;
    }

    if(error) {
        while(true){
            blinkTimes(2, 100);
        }
    }
    
    Serial.println("Sensor ready for testing .../n");
    digitalWrite(D7, HIGH);
    delay(500);
    digitalWrite(D7, LOW);
    delay(1000);

} // end of setup()


void loop() {

    String receivedData = "";  // string to hold the received LoRa data
    String cmd = "";  // string to hold the command to send to the LoRa
    static bool awaitingOK = false;  // when waiting for a response from the LoRa to our send command
    static bool awaitingResponse = false; // when waiting for a response from the hub
    static unsigned long startTime = 0;

  // test for button to be pressed and no transmission in progress
    if(digitalRead(D0) == LOW && !awaitingResponse && !awaitingOK) { // button press detected
        Serial.println("");
        Serial.println("--------------------");
        cmd = "AT+SEND=1,5,HELLO";
        Serial.println(cmd);
        Serial1.println(cmd);
        awaitingOK = true;
        startTime = millis();
    }
 
    if(awaitingOK) {

        if (millis() - startTime > 1000)  { // 1 second timeout is a long time
            awaitingOK = false;  // timed out
            blinkTimes(6);
            Serial.println("timeout waiting for +OK");
        }

        if(Serial1.available()) {
            Serial.println("delta time = " + String(millis() - startTime) + " ms");
            receivedData = Serial1.readString();
            Serial.println("received data = " + receivedData);

            if(receivedData.indexOf("+OK") >= 0) { // will be -1 of "+OK" not in the string
                blinkTimes(1,50); // signal that data received from the hub
                awaitingOK = false; // we got the OK
                awaitingResponse = true; // we need a response from the hub
                startTime = millis();
            } 
        } 
    } // end of if(awaitingOK)


    if(awaitingResponse) {

        if (millis() - startTime > 5000 ) { // wait 5 seconds for a response from the hub
            awaitingResponse = false;  // timed out
            blinkTimes(1);
            Serial.println("timeout waiting for hub response");
        }

        // is there received data?  process it
        if(Serial1.available()) {
            receivedData = Serial1.readString();
            Serial.println("received data = " + receivedData);

            // test for received data from the hub (denoted by "+RCV")
            if(receivedData.indexOf("+RCV") >= 0) { // will be -1 of "+RCV" not in the string
                
                awaitingResponse = false; // we got a response
                Serial.println("response received");

                if (receivedData.indexOf("TESTOK") >= 0) {
                    Serial.println("response is TESTOK");
                    blinkTimes(3);
                } else if (receivedData.indexOf("NOPE") >= 0) {
                    Serial.println("response is NOPE");
                    blinkTimes(4);
                } else {
                    Serial.println("response is unrecognized");
                    blinkTimes(5);
                }
            } 

        } // end of if(Serial1.available())

    } // end of if(awaitingResponse)

    //while(digitalRead(D0) == LOW); // wait for button to be released
    //delay(1); // wait a little while before sampling the button again

} // end of loop()

void blinkTimes(int number) {
    blinkTimes(number, 250);
}

void blinkTimes(int number, int delayTimeMS) {
    for(int i = 0; i < number; i++) {
        digitalWrite(D7, HIGH);
        delay(delayTimeMS);
        digitalWrite(D7, LOW);
        delay(delayTimeMS);
    }
    return;
} // end of blinkTimes()
