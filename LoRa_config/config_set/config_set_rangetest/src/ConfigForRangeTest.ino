/* 
 * Project Configuration of LoRa modules for range testing
 * Author: Jim Schrempp and Bob Glicksman
 * Date: 7/5/24
 * 
 * Operation:
 *    With a LoRa module in the configuration jig, boot the Photon. This will configure the LoRa module
 *        for network 3 and device 0 
 *    Press and hold the button during boot and the LoRa module will be configured for network 3 and device 1
 * 
 *    When the D& LED blinks rapidly, the LoRa module has been configured.
 * 
 * Description:  
 *  This is code for a configuration jig for LoRa.  The jig is based upon
 *  a Particle Photon (any Arduino can be used in its place).
 *  The LoRa module (RYLR998) is connected as follows:
 *  * Vcc to Photon 3.3v
 *  * GND to Photon GND
 *  * Tx to Photon Rx (Serial1)
 *  * Rx to Photon Tx (Serial1)
 *  * Reset is not connected
 * 
 * Connect a button between D0 and GND.  
 * 
 * This software is meant to run in the RangeTest Sensor breadboard.
 * 
 * Each LoRa module needs to be configured with a unique network number and device number.
 * The network number is a number from 0 to 255 and the device number is a number from 0 to 255.
 * The network number is used to filter out unwanted messages from other LoRa modules.  The device
 * number is used to identify the LoRa module.  The device number is used in the AT+SEND command
 * to specify the destination of the message.  The device number is also used in the AT+RCV command
 * to specify the source of the message.
 * 
 * The LoRa module is configured using the AT commands.  The AT commands are sent to the LoRa module
 * using the Photon's Serial1 port.  The LoRa module responds to the AT commands with an "OK" or
 * "ERROR" response. 
 * 
 * The LoRa module may be configured using a PC and an FTDI USB-serial board.  The LoRa module is
 * connected to the FTDI board and the FTDI board is connected to the PC.  The PC runs a terminal
 * program (e.g., Tera Term) to send AT commands to the LoRa module.  The LoRa module is configured
 * with the desired network number and device number.  The LoRa module is also configured with the
 * desired baud rate.  The baud rate is used to communicate with the LoRa module.  
 * 
 * This code can be used to configure the LoRa module with the desired network number and device number.
 * The code sends AT commands to the LoRa module to configure the network number and device number.
 * The LoRa module is assumed to be factory set to 115200 for this code. If not, then you will need
 * to change the baud rate in the code. 
 * 
 * For our range tests we use a network number of 3.  The device number for the tester is 0 and the
 * device number for the hub is 1.  
 * 
 * You put the LoRa module in the configuration jig. Configure a serial monitor to receive status messages
 * from the Photon. At boot the software will configure the LoRa module and provide status messages. These
 * are useful in case of an error.
 * 
 * version 1.0; 7/5/24
 */

#include "Particle.h"

// The following system directives are to disregard WiFi for Particle devices.  Not needed for Arduino.
//SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

#define VERSION 1.00
#define LoRaNETWORK_NUM 3
#define LoRaADDRESS_SENSOR 0
#define LoRaADDRESS_HUB 1
#define LoRaBANDWIDTH 7   // 7:125, 8:250, 9:500 kHz   lower is better for range; default 7
#define LoRaSPREADING_FACTOR 9  // 7 - 11  larger is better for range; default 9
                        // SF7to SF9 at 125kHz, SF7 to SF10 at 250kHz, and SF7 to SF11 at 500kHz
#define LoRaCODING_RATE 1  // 1 - 4  1 is faster; default 1
#define LoRaPREAMBLE 12  // max unless network number is 18; 

#define ms_TO_WAIT_FOR_RESPONSE 1000

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
//SerialLogHandler logHandler(LOG_LEVEL_INFO);

// function to send AT commands to the LoRa module
// returns 0 if successful, -1 if not successful
// prints message and result to the serial monitor
int LoRaCommand(String command) {
    Serial.println("");
    Serial.println(command);
    Serial1.println(command);
    delay(ms_TO_WAIT_FOR_RESPONSE); // wait for the response
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
    Serial.begin(38400); // the USB serial port 
    Serial1.begin(115200);  // the LoRa device

    Serial.println("");
    Serial.println("-----------------");
    Serial.println("ConfigForRangeTest version " + String(VERSION));

    delay(1000); // wait for the LoRa module to boot up

  }; // end of void setup()

void loop() {

    bool error = false;

    // check that LoRa is ready
    if(LoRaCommand("AT") == 0) {
        Serial.println("LoRa is ready");
    } else {
        Serial.println("LoRa is not ready");
        error = true;
    }
 
    // Set the network number
    if(LoRaCommand("AT+NETWORKID=" + String(LoRaNETWORK_NUM)) == 0) {
        Serial.println("Network number set");
    } else {
        Serial.println("Network number not set");
        error = true;
    }

    // Set the device number based on button state
    int deviceNumber = LoRaADDRESS_SENSOR;
    if(digitalRead(D0) == LOW) { // button press detected
        deviceNumber = LoRaADDRESS_HUB;
    }
    if(LoRaCommand("AT+ADDRESS=" + String(deviceNumber)) == 0) {
        Serial.println("Device number set");
    } else {
        Serial.println("Device number not set");
        error = true;
    }

    // set the parameters for the LoRa module

    if(LoRaCommand("AT+PARAMETER=" + String(LoRaSPREADING_FACTOR) + ","
        + String(LoRaBANDWIDTH) + "," + String(LoRaCODING_RATE) + "," + String(LoRaPREAMBLE)) == 0) {
        Serial.println("Parameters set");
    } else {
        Serial.println("Parameters not set");
        error = true;
    }

    // READ THEM BACK
    Serial.println("");
    Serial.println("");
    Serial.println("-----------------");
    Serial.println("Reading back the settings");

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
  

    Serial.println("");
    Serial.println("");
    Serial.println("-----------------");
    Serial.println("");
    // blink the D7 LED to show that the LoRa module has been configured
    int blinkTime = 100;
    if(error) {
        Serial.println("Error configuring LoRa module");
    } else {
        Serial.println("LoRa module configured");
        blinkTime = 500;
    }
    while (true) {
        digitalWrite(D7, HIGH);
        delay(blinkTime);
        digitalWrite(D7, LOW);
        delay(blinkTime);
    }
}

