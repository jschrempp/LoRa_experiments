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
#include "LoRa_common/tpp_LoRa.h"

// The following system directives are to disregard WiFi for Particle devices.  Not needed for Arduino.
SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

#define VERSION 1.00
#define STATION_NUM 0 // housekeeping; not used ini the code

#define THIS_LORA_SENSOR_ADDRESS -1 // the address of the sensor

//Jim's addresses
//#define THIS_LORA_SENSOR_ADDRESS 12648 // the address of the sensor LoRaSensor
//#define THIS_LORA_SENSOR_ADDRESS 11139 // the address of the sensor  lora3


// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

tpp_LoRa LoRa; // create an instance of the LoRa class

void setup() {
    pinMode(D0, INPUT_PULLUP);
    pinMode(D7, OUTPUT);
    digitalWrite(D7, HIGH);
    Serial.begin(9600); // the USB serial port 
    Serial1.begin(115200);  // the LoRa device

    if (LoRa.initDevice(THIS_LORA_SENSOR_ADDRESS) != 0) {  // initialize the LoRa device
        Serial.println("error initializing LoRa device - Stopping");
        Serial.println("hint: did you change the LoRaSensorAddress?");
        while(1) {blinkTimes(50);};
    }; 

    if (LoRa.readSettings() != 0) {  // read the settings from the LoRa device
        Serial.println("error reading LoRa settings - Stopping");
        while(1) {blinkTimes(50);};
    }; 
    
    Serial.println("Sensor ready for testing ...\n" );    
    digitalWrite(D7, LOW);

} // end of setup()


void loop() {

    String receivedData = "";  // string to hold the received LoRa data
    String cmd = "";  // string to hold the command to send to the LoRa
    static bool awaitingResponse = false; // when waiting for a response from the hub
    static unsigned long startTime = 0;
    static String lastRSSI = "";
    static String lastSNR = "";
    static int msgNum = 0;

  // test for button to be pressed and no transmission in progress
    if(digitalRead(D0) == LOW && !awaitingResponse) { // button press detected
        digitalWrite(D7, HIGH);
        Serial.println("");
        Serial.println("--------------------");
        msgNum++;
        String payload = "";
        switch (msgNum) {
            case 1:
                payload = "HELLO m: " + String(msgNum) + " uid: " + LoRa.UID;
                break;
            case 2:
                payload = "HELLO m: " + String(msgNum) + " p: " + LoRa.parameters;
                break;
            default:
                payload = "HELLO m: " + String(msgNum) + " rssi: " + lastRSSI + " snr: " + lastSNR;
                break;
        }
        LoRa.transmitMessage(String(TPP_LORA_HUB_ADDRESS), payload);
        awaitingResponse = true;
        startTime = millis();
        digitalWrite(D7, LOW);
    }


    if(awaitingResponse) {

        if (millis() - startTime > 5000 ) { // wait 5 seconds for a response from the hub
            awaitingResponse = false;  // timed out
            blinkTimes(1);
            Serial.println("timeout waiting for hub response");
        }
        LoRa.checkForReceivedMessage();
        switch (LoRa.receivedMessageState) {
            case -1: // error
                awaitingResponse = false;  // error
                blinkTimes(7);
                Serial.println("error while waiting for response");
                break;
            case 0: // no message
                delay(5); // wait a little while before checking again
                break;
            case 1: // message received
                Serial.println("received data = " + LoRa.receivedData);
                lastRSSI = LoRa.RSSI;
                lastSNR = LoRa.SNR;

                // test for received data from the hub (denoted by "+RCV")
                if(LoRa.receivedData.indexOf("+RCV") >= 0) { // will be -1 of "+RCV" not in the string
                    
                    awaitingResponse = false; // we got a response
                    Serial.println("response received");

                    if (LoRa.receivedData.indexOf("TESTOK") >= 0) {
                        Serial.println("response is TESTOK");
                        blinkTimes(3, 150);
                    } else if (LoRa.receivedData.indexOf("NOPE") >= 0) {
                        Serial.println("response is NOPE");
                        blinkTimes(4);
                    } else {
                        Serial.println("response is unrecognized");
                        blinkTimes(5);
                    }
                } 
                break;

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
