/*
    tpp_LoRa.h - routines for communication with the LoRa module
    created by Bob Glicksman and Jim Schrempp 2024
    as part of Team Practical Projects (tpp)

*/

#include "tpp_LoRa.h"

bool mg_LoRaBusy = false;

void tpp_LoRa::debugPrint(String message) {
    if(TPP_LORA_DEBUG) {
        Serial.print("tpp_LoRa: " + message);
    }
}
void tpp_LoRa::debugPrintNoHeader(String message) {
    if(TPP_LORA_DEBUG) {
        Serial.print(message);
    }
}
void tpp_LoRa::debugPrintln(String message) {
    if(TPP_LORA_DEBUG) {
        Serial.println("tpp_LoRa: " + message);
    }
}

void tpp_LoRa::clearClassVariabels() {
    receivedData = "";
    loraStatus = "";
    deviceNum = "";
    payload = "";
    RSSI = "";
    SNR = "";
    receivedMessageState = 0;
}   


// Initialize the LoRa module with settings 
// rtn True if successful

bool tpp_LoRa::initDevice(int deviceAddress) {

    bool error = false;

    LORA_SERIAL.begin(115200);  // the LoRa device

    // check that LoRa is ready
    if(sendCommand("AT") != 0) {
        debugPrintln("LoRa reply bad, trying again");

        if(sendCommand("AT") != 0) { // try again for photon 1
            debugPrintln("LoRa is not ready");
            error = true;
        } 
    }
    
    if(!error) {

        debugPrintln("LoRa is ready");

        // Set the network number
        if(sendCommand("AT+NETWORKID=" + String(LoRaNETWORK_NUM)) != 0) {
                debugPrintln("Network ID not set");
                error = true;
        } else if(sendCommand("AT+ADDRESS=" + String(deviceAddress)) != 0) {
                debugPrintln("Device number not set");
                error = true;
        } else if(sendCommand("AT+PARAMETER=" + String(LoRaSPREADING_FACTOR) + ","
                    + String(LoRaBANDWIDTH) + "," + String(LoRaCODING_RATE) + "," + String(LoRaPREAMBLE)) != 0) {
            debugPrintln("Parameters not set");
            error = true;
        } else if (sendCommand("AT+MODE=0") != 0) {
            debugPrintln("Tranciever mode not set");
            error = true;
        } else if (sendCommand("AT+BAND=915000000") != 0) {
            debugPrintln("Band not set");
            error = true;
        } else if (sendCommand("AT+CRFOP=22") != 0) {
            debugPrintln("Power not set");
            error = true;
        } else {
            debugPrintln("LoRo module is initialized");
        }
    }
    
    thisDeviceNetworkID = deviceAddress; 

    return error;

}

// Read current settings and print them to the serial monitor
//  If error then the D7 will blink twice
bool tpp_LoRa::readSettings() {
    // READ LoRa Settings
    debugPrintln("");
    debugPrintln("");
    debugPrintln("-----------------");
    debugPrintln("Reading back the settings");

    bool error = false;

    if(sendCommand("AT+UID?") != 0) {
        debugPrintln("error reading UID");
        error = true;
    } else {
        UID = receivedData.substring(5, receivedData.length());
        UID.trim();
    }
    
    if(sendCommand("AT+CRFOP=22?") != 0) {
        debugPrintln("error reading radio power");
        error = true;
    } else if (sendCommand("AT+NETWORKID?") != 0) {
        debugPrintln("error reading network id");
        error = true;
    } else if(sendCommand("AT+ADDRESS?") != 0) {
        debugPrintln("error reading device address");
        error = true;
    } else if(sendCommand("AT+PARAMETER?") != 0) {
        debugPrintln("error reading parameters");
        error = true;
    } else {
        // replace commas with backslashes in the parameters string
        parameters = receivedData;
        parameters.trim();
        parameters.replace(",", ":");
        parameters = "[" + parameters + "]";
    }

    return error;
}

// function to send AT commands to the LoRa module
// returns 0 if successful, 1 if error, -1 if no response
// prints message and result to the serial monitor
int tpp_LoRa::sendCommand(String command) {

    if (mg_LoRaBusy) {
        debugPrintln("LoRa is busy");
        return 1;
    }   
    mg_LoRaBusy = true;

    int retcode = 0;
    system_tick_t timeoutMS = 1000;
    receivedData = "";

    debugPrintln("");
    debugPrintln("cmd: " + command);
    LORA_SERIAL.println(command);
    
    // wait for data available, which should be +OK or +ERR
    system_tick_t starttimeMS = millis();
    int dataAvailable = 0;
    debugPrint("waiting ");
    do {
        dataAvailable = LORA_SERIAL.available();
        delay(10);
        debugPrintNoHeader(".");
    } while ((dataAvailable == 0) && (millis() - starttimeMS < timeoutMS)) ;
    debugPrintNoHeader("\n");

    delay(100); // wait for the full response

    // Get the response if there is one
    if(dataAvailable > 0) {
        receivedData = LORA_SERIAL.readString();
        // received data has a newline at the end
        receivedData.trim();
        debugPrintln("received data = " + receivedData);
        if(receivedData.indexOf("+ERR") >= 0) {
            debugPrintln("LoRa error");
            retcode = 1;
        } else {
            debugPrintln("command worked");
            retcode = 0;
        }
    } else {
        debugPrintln("No response from LoRa");
        retcode =  -1;
    }
    mg_LoRaBusy = false;
    return retcode;
};

// function to transmit a message to another LoRa device
// returns 0 if successful, 1 if error, -1 if no response
// prints message and result to the serial monitor
int tpp_LoRa::transmitMessage(String devAddress, String message){

    String cmd = "AT+SEND=" + devAddress + "," + String(message.length()) + "," + message;

    return sendCommand(cmd);

}


// If there is data on Serial1 then read it and parse it into the class variables. 
// Set receivedMessageState to 1 if successful, 0 if no message, -1 if error
// If there is no data on Serial1 then clear the class variables.
void tpp_LoRa::checkForReceivedMessage() {

    if (mg_LoRaBusy) {
        debugPrintln("LoRa is busy");
        receivedMessageState = 0;
        return;
    }   
    mg_LoRaBusy = true;

    clearClassVariabels();

    if(LORA_SERIAL.available()) { // data is in the Serial1 buffer

        debugPrintln("");
        debugPrintln("--------------------");
        delay(100); // wait a bit for the complete message to have been received
        receivedData = LORA_SERIAL.readString();
        // received data has a newline at the end
        receivedData.trim();
        debugPrintln("received data = " + receivedData);

        if ((receivedData.indexOf("+OK") == 0) && receivedData.length() == 3) {

            // this is the normal OK from LoRa that the previous command succeeded
            debugPrintln("received data is +OK");
            receivedMessageState = 1;

        } else {

            if (receivedData.indexOf("+RCV") < 0) {
                // We are expecting a +RCV message
                debugPrintln("received data is not +RCV");
                receivedMessageState = -1;
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
                            debugPrintln("ERROR: received data from sensor has weird comma count");
                            break;
                            commaCountError = true;
                        }   
                        commas[commaCount] = i;
                    }
                }
                
                if (commaCountError) {

                    // error in the received data
                    debugPrintln("ERROR: received data from sensor has odd comma count");

                    receivedMessageState = -1;

                } else {
                    
                    // create substrings from received data
                    deviceNum = receivedData.substring(5, commas[0]);  // skip the "+RCV="
                    //charCount = receivedData.substring(commas[1] + 1, commas[2]);
                    payload = receivedData.substring(commas[2] + 1, commas[3]);
                    RSSI = receivedData.substring(commas[3] + 1, commas[4]);
                    SNR = receivedData.substring(commas[4] + 1, receivedData.length()); // -1 to remove the newline

                    receivedMessageState = 1;

                } // end of if (commaCount != 4) 
            } // end of if(receivedData.indexOf("+RCV") < 0)
        } // end of if ((receivedData.indexOf("+OK") == 0) && receivedData.length() == 5)

    } else {

        // no data in the Serial1 buffer
        clearClassVariabels();
    }

    mg_LoRaBusy = false;

    return;
}


