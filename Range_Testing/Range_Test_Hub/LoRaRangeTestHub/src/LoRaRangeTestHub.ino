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
#include "LoRa_common/tpp_LoRa.h"

// The following system directives are for Particle devices.  Not needed for Arduino.
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_TRACE);

#define VERSION 1.00
#define STATION_NUM 1 // housekeeping; not used ini the code

String NODATA = "NODATA";
tpp_LoRa LoRa;


void logToParticle(String message, String deviceNum, String msgNum, String payload, String RSSIhub1, String SNRhub1, String SNRhub2, String SNRsensor) {
    // create a JSON string to send to the cloud
    String data = "message=" + message + "|msgNum=" + String(msgNum) 
        + "|deviceNum=" + deviceNum + "|payload=" + payload + "|RSSIHub1=" + RSSIhub1
        + "|SNRhub1=" + String(SNRhub1) + "|SNRhub2=" + String(SNRhub2) 
        + "|SNRsensor=" + String(SNRsensor);

    Serial.println("cloudLogging:" + data);
    long rtn = Particle.publish("LoRaHubLogging", data, PRIVATE);
    Serial.println("cloudLogging return: " + String(rtn));
}


void setup() {

    pinMode(D7, OUTPUT);
    digitalWrite(D7, HIGH);
    Serial.begin(9600); // the USB serial port 
    Serial1.begin(115200);  // the LoRa device

    LoRa.readSettings(); 

    Serial.println("Hub ready for testing .../n");
    Serial.print("waiting for data ...");

    digitalWrite(D7, LOW);

} // end of setup()


void loop() {

    
    static String receivedData = "";  // string to hold the received LoRa dat

    // wait for a message from the tester
    LoRa.checkForReceivedMessage();
    switch (LoRa.receivedMessageState) {
        case -1: // error
            Serial.println("Error reading data from LoRa module");
            break;
        case 0: // no message
            break;
        case 1: // message received
            if ((LoRa.receivedData.indexOf("+OK") == 0) && LoRa.receivedData.length() == 5) {

                // this is the normal OK from LoRa that the previous command succeeded
                Serial.println("received data is +OK");

            } else {
                
                String logMessage = "";
                String messageSent = "";
                digitalWrite(D7, HIGH);

                if(LoRa.receivedData.indexOf("HELLO") > 0) { // will be -1 if "HELLO" not in the string

                    // HELLO is the message from our sensors
                    // send a message back to the sensor
                    if (LoRa.sendCommand("AT+SEND=0,6,TESTOK")) {
                        Serial.println("sent TESTOK to sensor");
                        logMessage = "TESTOK";
                        messageSent = "TESTOK";
                    } else {
                        Serial.println("error sending TESTOK to sensor");
                        logMessage = "Send of TESTOK failed";
                    };

                } else {

                    Serial.println("received data is not HELLO or +OK");
                    LoRa.sendCommand("AT+SEND=0,4,NOPE");
                    logMessage = "NOPE";
                    messageSent = "NOPE";

                } // end of if(receivedData.indexOf("HELLO") > 0)

                // log the data to the cloud
                Serial.println("sent message: " + messageSent);
                logToParticle(logMessage, LoRa.deviceNum, NODATA, LoRa.payload, LoRa.RSSI, LoRa.SNR, NODATA, NODATA);

                digitalWrite(D7, LOW);
            }
            break;
    } // end of switch

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
