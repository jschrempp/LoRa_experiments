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
SYSTEM_THREAD(ENABLED);

#define VERSION 1.00
#define STATION_NUM 1 // housekeeping; not used ini the code

String NODATA = "NODATA";

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

void logToParticle(String message, String devNum, String msgNum, String SNRhub1, String SNRhub2, String SNRsensor) {
    // create a JSON string to send to the cloud
    String data = "message=" + message + "|msgNum=" + String(msgNum) 
        + "|SNRhub1=" + String(SNRhub1) + "|SNRhub2=" + String(SNRhub2) 
        + "|SNRsensor=" + String(SNRsensor);

    Serial.println("cloudLogging:" + data);
    long rtn = Particle.publish("LoRaHubLogging", data, PRIVATE);
    Serial.println("cloudLogging return: " + String(rtn));
}



void setup() {

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

    digitalWrite(D7, HIGH);
    delay(500);
    digitalWrite(D7, LOW);
    delay(1000);
    Serial.println("Hub ready for testing .../n");
    Serial.print("waiting for data ...");

} // end of setup()


void loop() {

    
    static String receivedData = "";  // string to hold the received LoRa dat

    // wait for a message from the tester
    if(Serial1.available()) { // data is in the Serial1 buffer
        Serial.println("");
        Serial.println("--------------------");
        delay(5); // wait a bit for the complete message to have been received
        receivedData = Serial1.readString();
        Serial.println("received data = " + receivedData);

        if(receivedData.indexOf("HELLO") > 0) { // will be -1 if "HELLO" not in the string
            // HELLO is the message from our sensors

            // find the commas in received data
            int commas[5];
            int commaCount = 0;
            for(int i = 0; i < receivedData.length(); i++) {
                if(receivedData.charAt(i) == ',') {
                    commaCount++;
                    commas[commaCount] = i;
                    if (commaCount > 5) {
                        // should never happen
                        Serial.println("ERROR: received data from sensor has more than 5 commas");
                        break;
                    }   
                }
            }
            if (commaCount != 4) {
                // should never happen
                Serial.println("ERROR: received data from sensor does not have 5 commas");
            } else {
                // create substrings from received data
                String loraStatus = receivedData.substring(0, commas[1]);
                String deviceNum = receivedData.substring(commas[1] + 1, commas[2]);
                String payload = receivedData.substring(commas[2] + 1, commas[3]);
                String RSSI = receivedData.substring(commas[3] + 1, commas[4]);
                String SNRHub = receivedData.substring(commas[4] + 1, receivedData.length()-2); // -1 to remove the newline

                digitalWrite(D7, HIGH);
                Serial1.println("AT+SEND=0,6,TESTOK");
                Serial.println("sent:  TESTOK");
                logToParticle("sent:  TESTOK", deviceNum, NODATA, SNRHub, NODATA, NODATA);
                digitalWrite(D7, LOW);
            }

        } else if(receivedData.indexOf("+OK") == 0) {
            // was not hello but was the normal OK from LoRa that the previous
            // command succeeded
            
            Serial.println("received data is +OK");
    
        } else {

            digitalWrite(D7, HIGH);
            Serial.println("received data is not HELLO or +OK");
            Serial1.println("AT+SEND=0,4,NOPE");
            Serial.println("sent:  NOPE");
            logToParticle("sent:  NOPE",NODATA, NODATA, NODATA, NODATA, NODATA);
            digitalWrite(D7, LOW);

        }
    }  // end of if(Serial1.available())

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
