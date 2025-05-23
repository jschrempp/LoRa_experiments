/*
    tpp_LoRa.h - routines for communication with the LoRa module
    created by Bob Glicksman and Jim Schrempp 2024
    as part of Team Practical Projects (tpp)

    20241212 - works on Particle Photon 2
    v 2.1 pulled all string searches out of if() clause
    v 2.2 removed version as a #define
    20241218 works on AMmega328 
    20241222 added setAddress
    20250114 added CRFOP parameter to header file

*/

#include "tpp_LoRa.h"


#define TPP_LORA_DEBUG 0  // Do NOT enable this for ATmega328

bool mg_LoRaBusy = false;

String tempString; 

// define the parameter as const String& to avoid copying the string
// which important on the ATmega328
void tpp_LoRa::debugPrintln(const String& message) {
    #if TPP_LORA_DEBUG
        String msg = "tpp_LoRa: "; // if we don't declare a string here the println fails
        msg += message;
        DEBUG_SERIAL.println(msg);
    #endif
}
void tpp_LoRa::debugPrintNoHeader(const String& message){
    #if TPP_LORA_DEBUG
        String msg  = message;
        DEBUG_SERIAL.println(msg);
    #endif
}
void tpp_LoRa::debugPrint(const String& message){
    #if TPP_LORA_DEBUG
        String msg  = message;
        DEBUG_SERIAL.print(msg);
    #endif
}

void tpp_LoRa::clearConfigVariables() {
    LoRaCRFOP = 0;
    LoRaBandwidth = 0;
    LoRaSpreadingFactor = 0;
    LoRaCodingRate = 0;
    LoRaDeviceAddress = 0;
    LoRaNetworkID = 0;
    LoRaPreamble = 0;
    UID = "";
}  


void tpp_LoRa::clearClassVariables() {
    LoRaStringBuffer = "";
    receivedData = "";
    payload = "";
    RSSI = 0;
    SNR = 0;
    receivedMessageState = 0;
    tempString = "";
}   

// Do some class initialization stuff
// and make sure LoRa will respond
int tpp_LoRa::begin() {
    LoRaStringBuffer.reserve(100);  // reserve some space for the LoRa string buffer so it is not constantly reallocating
    UID.reserve(30);
    receivedData.reserve(100);
    payload.reserve(75);
    tempString.reserve(50);

    debugPrintln(F("Start LoRa initialization")); // so this AFTER tempString is reserved

    LORA_SERIAL.begin(38400);
    LORA_SERIAL.setTimeout(10);

    // check that LoRa is ready
    LoRaStringBuffer = F("AT");
    int errRtn = sendCommand(LoRaStringBuffer);
    if(errRtn) {
        delay(1000);
        errRtn = sendCommand(LoRaStringBuffer);
        if(errRtn) { // try again for photon 1
            return errRtn;
        } 
    }

    isLoRaAwake = true;
    return 0;

}
// set just the device address
// rtn True if failure
bool tpp_LoRa::setAddress(unsigned int deviceAddress) {

    if(wake() != 0) {
        return 1;
    }

    debugPrintln(F("Start LoRa address set"));

    LoRaStringBuffer = F("AT+ADDRESS=");
    LoRaStringBuffer += deviceAddress;
    if(sendCommand(LoRaStringBuffer) != 0) {   // xxx should this be &lorastirngbuffer;
        debugPrintln(F("Device number not set"));
        return 1;
    } 

    return 0;
}

// Configure the LoRa module with settings 
// rtn True if failure
bool tpp_LoRa::configDevice(int deviceAddress) {

    if(wake() != 0) {
        return true;
    }

    debugPrintln(F("Start LoRa configuration"));

    clearConfigVariables();

    LoRaStringBuffer = F("AT+NETWORKID=");
    LoRaStringBuffer += LoRa_NETWORK_ID;
    if(sendCommand(LoRaStringBuffer) != 0) {
        debugPrintln(F("Network ID not set"));
        return true;
    }

    LoRaStringBuffer = F("AT+ADDRESS=");
    LoRaStringBuffer += deviceAddress;
    if(sendCommand(LoRaStringBuffer) != 0) {   // xxx should this be &lorastirngbuffer;
        debugPrintln(F("Device number not set"));
        return true;
    } 
        
    LoRaStringBuffer = F("AT+PARAMETER=");
    LoRaStringBuffer += LoRa_SPREADING_FACTOR;
    LoRaStringBuffer += F(",");
    LoRaStringBuffer += LoRa_BANDWIDTH;
    LoRaStringBuffer += F(",");
    LoRaStringBuffer += LoRa_CODING_RATE;
    LoRaStringBuffer += F(",");
    LoRaStringBuffer += LoRa_PREAMBLE;
    if(sendCommand(LoRaStringBuffer) != 0) {
        debugPrintln(F("Parameters not set"));
        return true;
    } 

    LoRaStringBuffer = F("AT+MODE=0");
    if (sendCommand(LoRaStringBuffer) != 0) {
        debugPrintln(F("Tranciever mode not set"));
        return true;
    } 

    LoRaStringBuffer = F("AT+BAND=915000000");
    if (sendCommand(LoRaStringBuffer) != 0) {
        debugPrintln(F("Band not set"));
        return true;
    } 

    LoRaStringBuffer = F("AT+CRFOP=");
    LoRaStringBuffer += LoRa_CRFOP;
    if (sendCommand(LoRaStringBuffer) != 0) {
        debugPrintln(F("Power not set"));
        return true;
    } 
    
    debugPrintln(F("LoRa module is initialized"));

    return false;

}

// Read current settings and print them to the serial monitor
//  If error then the D7 will blink twice
//  Return true if error
bool tpp_LoRa::readSettings() {

    if(wake() != 0) {
        return true;
    }

    // READ LoRa Settings
    LoRaStringBuffer = F("\r\n\r\n-----------------\r\nReading back the settings");
    debugPrintln(LoRaStringBuffer);

    if(sendCommand(F("AT+UID?")) != 0) {
        debugPrintln(F("error reading UID"));
        return true;
    } else {
        UID = receivedData.substring(5, receivedData.length());
        UID.trim();
    }
    
    if(sendCommand(F("AT+CRFOP?")) != 0) {
        debugPrintln(F("error reading radio power"));
        return true;
    } else { 
        LoRaCRFOP = receivedData.substring(7, receivedData.length()).toInt();
    }

    if (sendCommand(F("AT+NETWORKID?")) != 0) {
        debugPrintln(F("error reading network id"));
        return true;
    } else  { 
        LoRaNetworkID = receivedData.substring(11, receivedData.length()).toInt();
    }

    if(sendCommand(F("AT+ADDRESS?")) != 0) {
        debugPrintln(F("error reading device address"));
        return true;
    } else {  
        LoRaDeviceAddress = receivedData.substring(9, receivedData.length()).toInt();
    }

    if(sendCommand(F("AT+PARAMETER?")) != 0) {
        debugPrintln(F("error reading parameters"));
        return true;
    } else {
        int firstComma = receivedData.indexOf(F(","));
        int secondComma = receivedData.indexOf(F(","), firstComma + 1);
        int thirdComma = receivedData.indexOf(F(","), secondComma + 1);
        LoRaSpreadingFactor = receivedData.substring(11, firstComma).toInt();
        LoRaBandwidth = receivedData.substring(firstComma + 1, secondComma).toInt();
        LoRaCodingRate = receivedData.substring(secondComma + 1, thirdComma).toInt();
        LoRaPreamble = receivedData.substring(thirdComma + 1,receivedData.length()).toInt();
    }

    return false;
}


// function puts LoRa to sleep and turns off the power. LoRa will awaken when sent
// a message.  Returns 0 if successful, 1 if error
int tpp_LoRa::sleep(){

    LoRaStringBuffer = F("AT");
    int errRtn = sendCommand(LoRaStringBuffer);
    if (errRtn) {
        return errRtn;
    }

    LoRaStringBuffer = F("AT+MODE=1");
    errRtn = sendCommand(LoRaStringBuffer);
    if(errRtn) {
        return errRtn;
    } else { 
        
        isLoRaAwake = false; 
        return 0;
    }
};

// function to wake up the LoRa module from a low power sleep
// returns 0 if successful, otherwise error code
int tpp_LoRa::wake(){

    if (isLoRaAwake) {
        return 0;
    }

    LoRaStringBuffer = F("AT");
    int errRtn = sendCommand(LoRaStringBuffer);
    if(errRtn) {
        return errRtn;
    } else {

        LoRaStringBuffer = F("AT+MODE=0");
        errRtn = sendCommand(LoRaStringBuffer);
        if(errRtn) {
            return errRtn;

        } else { 

            isLoRaAwake = true; 
            return 0;
       }
    }
};

// function to send AT commands to the LoRa module
// returns 0 if successful, error code if not
// prints message and result to the serial monitor
int tpp_LoRa::sendCommand(const String& command) {

    // DO NOT check for wake here. This is called by wake and sleep
    // and will cause a recursive loop.
 
    if (mg_LoRaBusy) {
        debugPrintln(F("LoRa is busy"));
        return 1;
    }   
    mg_LoRaBusy = true;

    int retcode = 0;
    unsigned long timeoutMS = 15000; // xxx see below - do we still need this?
    receivedData = "";

    tempString = F("cmd: ");
    tempString += command;
    debugPrintln(tempString);
    LORA_SERIAL.println(command);
    
    // wait for data available, which should be +OK or +ERR
    unsigned long starttimeMS = millis();  // xxx do we still need this timeout now that we use the
                                          // xxx timeout in the serial port? Is this a good safety?
    int dataAvailable = 0;
    // delay(100);
    do {
        dataAvailable = LORA_SERIAL.available();
        delay(10);
    } while ((dataAvailable == 0) && ( millis() - starttimeMS < timeoutMS)) ;
    

    delay(100); // wait for the full response  //xxx we might not need this any more

    // Get the response if there is one
    if(dataAvailable > 0) {

        receivedData = LORA_SERIAL.readString();
        // received data has a newline at the end
        receivedData.trim();
        tempString = F("received data = ");
        tempString += receivedData;
        debugPrintln(tempString);
        int errIndex = receivedData.indexOf(F("+ERR"));
        if(errIndex >= 0) {
            debugPrintln(F("LoRa returned +ERR"));
            retcode = 1;
        } else { 
            //int rcvIndex = receivedData.indexOf(F("+OK"));
            //if (rcvIndex == 0) {
                retcode = 0;
            //} else {
                // unknown string from LoRa
              //  retcode = 2;
            //}
        }
    } else {
        debugPrintln(F("No response from LoRa"));
        retcode =  3;
    }
    mg_LoRaBusy = false;
    return retcode;
};

// function to transmit a message to another LoRa device
// returns 0 if successful, 1 if error, -1 if no response
// prints message and result to the serial monitor
int tpp_LoRa::transmitMessage(long int toAddress, const String& message){

    if(wake() != 0) {
        return true;
    }

    LoRaStringBuffer = F("AT+SEND=");
    LoRaStringBuffer += toAddress;
    LoRaStringBuffer += F(",");
    LoRaStringBuffer += message.length(); 
    LoRaStringBuffer += F(",");
    LoRaStringBuffer += message;

    int errRtn = sendCommand(LoRaStringBuffer);
    return errRtn;

}


// If there is data on Serial1 then read it and parse it into the class variables. 
// Set receivedMessageState to 1 if successful, 0 if no message, -1 if error
// If there is no data on Serial1 then clear the class variables.
void tpp_LoRa::checkForReceivedMessage() {

    ReceivedDeviceAddress = 0;

    if(wake() != 0) {
        return;
    }

    if (mg_LoRaBusy) {
        receivedMessageState = 0;
        return;
    }   
    mg_LoRaBusy = true;

    clearClassVariables();

    if(LORA_SERIAL.available()) { // data is in the Serial1 buffer

        debugPrintln(F("\n\r--------------------"));
        delay(100); // wait a bit for the complete message to have been received
        receivedData = LORA_SERIAL.readString();
        // received data has a newline at the end
        receivedData.trim();
        tempString = F("received data = ");
        tempString += receivedData;
        debugPrintln(tempString);

        int okIndex = receivedData.indexOf(F("+OK"));
        if ((okIndex == 0) && receivedData.length() == 3) {

            // this is the normal OK from LoRa that the previous command succeeded
            debugPrintln(F("received data is +OK"));
            receivedMessageState = 1;

        } else {

            int rcvIndex = receivedData.indexOf(F("+RCV"));
            if (rcvIndex < 0) {
                // We are expecting a +RCV message
                debugPrintln(F("received data is not +RCV"));
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
                            debugPrintln(F("ERROR: received data from sensor has weird comma count"));
                            break;
                            commaCountError = true;
                        }   
                        commas[commaCount] = i;
                    }
                }
                
                if (commaCountError) {

                    // error in the received data
                    debugPrintln(F("ERROR: received data from sensor has odd comma count"));

                    receivedMessageState = -1;

                } else {
                    
                    // create substrings from received data
                    ReceivedDeviceAddress = receivedData.substring(5, commas[0]).toInt();  // skip the "+RCV="
                    //charCount = receivedData.substring(commas[1] + 1, commas[2]);
                    payload = receivedData.substring(commas[2] + 1, commas[3]);
                    RSSI = receivedData.substring(commas[3] + 1, commas[4]).toInt();
                    SNR = receivedData.substring(commas[4] + 1, receivedData.length()).toInt(); 

                    receivedMessageState = 1;

                } // end of if (commaCount != 4) 
            } // end of if(receivedData.indexOf("+RCV") < 0)
        } // end of if ((receivedData.indexOf("+OK") == 0) && receivedData.length() == 5)

    } else {

        // no data in the Serial1 buffer
        clearClassVariables();
    }

    mg_LoRaBusy = false;

    return;
}
