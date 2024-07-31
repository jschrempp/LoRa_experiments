/*
    tpp_LoRa.h - routines for communication with the LoRa module
    created by Bob Glicksman and Jim Schrempp 2024
    as part of Team Practical Projects (tpp)

*/



#include "Particle.h"
#include "tpp_LoRa.h"


// Read current settings and print them to the serial monitor
//  If error then the D7 will blink twice
bool tpp_LoRa::readSettings() {
    // READ LoRa Settings
    Serial.println("");
    Serial.println("");
    Serial.println("-----------------");
    Serial.println("Reading back the settings");

    bool error = false;
    if(sendCommand("AT+NETWORKID?") != 0) {
        Serial.println("error reading network id");
        error = true;
    }
    if(sendCommand("AT+ADDRESS?") != 0) {
        Serial.println("error reading device address");
        error = true;
    }
    if(sendCommand("AT+PARAMETER?") != 0) {
        Serial.println("error reading parameters");
        error = true;
    }
    return error;
}

// function to send AT commands to the LoRa module
// returns 0 if successful, -1 if not successful
// prints message and result to the serial monitor
int tpp_LoRa::sendCommand(String command) {
    int timeoutMS = 50;
    if(command.indexOf("SEND") > 0) {
        timeoutMS = 500;
    }

    Serial.println("");
    Serial.println(command);
    LORA_SERIAL.println(command);
    delay(timeoutMS); // wait for the response
    int retcode = 0;
    if(LORA_SERIAL.available()) {
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

// If there is data on Serial1 then read it and parse it into the class variables
// If there is no data on Serial1 then clear the class variables.
void tpp_LoRa::checkForReceivedMessage() {

    if(LORA_SERIAL.available()) { // data is in the Serial1 buffer

        Serial.println("");
        Serial.println("--------------------");
        delay(5); // wait a bit for the complete message to have been received
        receivedData = LORA_SERIAL.readString();
        Serial.println("received data = " + receivedData);

        if ((receivedData.indexOf("+OK") == 0) && receivedData.length() == 5) {

            // this is the normal OK from LoRa that the previous command succeeded
            Serial.println("received data is +OK");
            receivedMessageState = 1;

        } else {
            
            // find the commas in received data
            unsigned int commas[5];
            bool commaCountError = false;   

            // find first comma
            for(unsigned int i = 0; i < receivedData.length(); i++) {
                if(receivedData.charAt(i) == ',') {   
                    commas[0] = i;
                    break;
                }
            }

            // find other commas from the end to the front
            int commaCount = 5;
            for(unsigned int i = receivedData.length()-1; i >= commas[0]; i--) {
                if(receivedData.charAt(i) == ',') {
                    commaCount--;
                    if (commaCount < 1) {
                        // should never happen
                        Serial.println("ERROR: received data from sensor has weird comma count");
                        break;
                        commaCountError = true;
                    }   
                    commas[commaCount] = i;
                }
            }
            
            if (commaCountError) {

                // error in the received data
                Serial.println("ERROR: received data from sensor has odd comma count");

                receivedMessageState = -1;

            } else {
                // xxx should read backwards from the end of the string to get commas
                // create substrings from received data
                loraStatus = receivedData.substring(0, commas[1]);
                deviceNum = receivedData.substring(commas[1] + 1, commas[2]);
                payload = receivedData.substring(commas[2] + 1, commas[3]);
                RSSI = receivedData.substring(commas[3] + 1, commas[4]);
                SNR = receivedData.substring(commas[4] + 1, receivedData.length()-2); // -1 to remove the newline

                receivedMessageState = 1;

            } // end of if (commaCount != 4) 
            
        } // end of if ((receivedData.indexOf("+OK") == 0) && receivedData.length() == 5)

    } else {

        // no data in the Serial1 buffer
        receivedData = "";
        loraStatus = "";
        deviceNum = "";
        payload = "";
        RSSI = "";
        SNR = "";
        receivedMessageState = 0;
    }

    return;
}


