/* 
 * Project RangeTestSensor
 * Author: Bob Glicksman
 * Date: 4/25/24
 * 
 * Description:  This is code for a tester of LoRa signal range.  The tester is based upon
 *  a Particle Photon that is set up to not need Wifi (an ATmega328 can be used in its place).
 *  In addition to the Photon, a N.O. pushbutton switch is connected to ground at one end
 *  and Photon pin D0 on the other.  On the ATMega a N.O. pushbutton is connected to Vcc at one
 *  end and pin D2 on the other.
 
    The LoRa module (RYLR998) is connected as follows:
 *  * Vcc to Photon 3.3v
 *  * GND to Photon GND
 *  * Tx to Photon Rx (Serial1) (Serial on ATmega)
 *  * Rx to Photon Tx (Serial1) (Serial on ATmega) 
 *  * Reset is not connected
 * 
 *  The testing concept is to produce a companion device - the "hub".  The hub uses its LoRa module
 *  to listen for a message from the tester.  When a message is received, the hub responds with a
 *  message of its own.  If the tester receives the response message, it is still in range of the
 *  hub.
 * 
 * The tester is assigned any device number and the hub is assigned device number 57248, as defined
    in tpp_LoRa.h   (it can be any arbitrary number in the range  0 - 65535).  The network
 *  number used for testing is 18 and the baud rate to/from the LoRa modem is 38400 to be 
    compatible with the ATmega328.  Otherwise,
 *  the default LoRa module values are used.  NOTE:  the sensor code does not set up these
 *  values if run on an ATmega.  The LoRa modules can be set up using a PC and an FTDI USB-serial
    board. If the LoRa module is already set to 38400 baud, then they can be further configured
    by connecting one to a Particle Photon 2 sensor. LoRa modules can also be set up using 
    the Particle Photon 2 hub. 
 * 
 *  The software senses the falling (P2) or rising (ATmega) signal from the pressing of 
    the pushbutton and sends a short message to a companion LoRa
 *  module in the hub.  The message is also printed on the Photon's USB serial port for debugging
 *  purposes.  The unit then waits 3 seconds for a response.  If a response is received 
 *  (data received from the hub, beginning with +RCV), the Green LED (onboard D7 of the Photon) 
    is flashed three times.  If no
 *  data is received from the hub, the Green LED (onboard D7 of the Photon) is flashed once.
 * 
 * version 2.0; 4/25/24

    20241212 - version 2. works on Particle Photon 2 Verified on ATMega328

 */

#include "tpp_LoRaGlobals.h"

#include "tpp_loRa.h" // include the LoRa class

// The following system directives are to disregard WiFi for Particle devices.  Not needed for Arduino.
#if PARTICLEPHOTON
    SYSTEM_MODE(SEMI_AUTOMATIC);
    SYSTEM_THREAD(ENABLED);
    // Show system, cloud connectivity, and application logs over USB
    // View logs with CLI using 'particle serial monitor --follow'
    // SerialLogHandler logHandler(LOG_LEVEL_INFO);
#endif

#define VERSION 2.00
#define STATION_NUM 0 // housekeeping; not used ini the code

#define THIS_LORA_SENSOR_ADDRESS 5 // the address of the sensor

//Jim's addresses
//#define THIS_LORA_SENSOR_ADDRESS 12648 // the address of the sensor LoRaSensor
//#define THIS_LORA_SENSOR_ADDRESS 11139 // the address of the sensor  lora3

tpp_LoRa LoRa; // create an instance of the LoRa class

// module global scope for mimimal string memboery allocation
int mglastRSSI = 0;
int mglastSNR = 0;
bool mgFatalError = false;
volatile bool mgButtonPressed = false;  // set true in the ISR_buttonPressed() function
String mgpayload;
String mgTemp;

// all debug prints through here so it can be disabled when ATmega328 is used
void debugPrintln(const String message) {
    #if PARTICLEPHOTON
        DEBUG_SERIAL.println(message);
    #endif
}

// blinkLED(): blinks the indicated LED "times" number of times
void blinkLED(int ledpin, int number, int delayTimeMS) {
    for(int i = 0; i < number; i++) {
        digitalWrite(ledpin, HIGH);
        delay(delayTimeMS);
        digitalWrite(ledpin, LOW);
        delay(delayTimeMS);
    }
    return;
} // end of blinkLED()

// blinkLEDsOnBoot() blinks the LEDs to indicate the system is booting
void blinkLEDsOnBoot() {
    digitalWrite(GRN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    delay(100);
    for(int i = 0; i < 3; i++) {
        digitalWrite(GRN_LED_PIN, HIGH);
        digitalWrite(RED_LED_PIN, HIGH);
        delay(150);
        digitalWrite(GRN_LED_PIN, LOW);
        digitalWrite(RED_LED_PIN, LOW);
        delay(100);
    }
    return;
}

void ISR_buttonPressed() {
    mgButtonPressed = true;
}

void setup() {

    #if PARTICLEPHOTON
        pinMode(BUTTON_PIN, INPUT_PULLUP);
        attachInterrupt(BUTTON_PIN, ISR_buttonPressed, FALLING);
    #else
        pinMode(BUTTON_PIN, INPUT);
        attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), ISR_buttonPressed, FALLING);
    #endif

    pinMode(GRN_LED_PIN, OUTPUT); 
    pinMode(RED_LED_PIN, OUTPUT); 

    digitalWrite(GRN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, HIGH);

    mgpayload.reserve(50);
    mgTemp.reserve(50);

    #if PARTICLEPHOTON
        DEBUG_SERIAL.begin(115200); // the USB serial port
        waitFor(DEBUG_SERIAL.isConnected, 15000);
    #else
        // ATMega328 has only one serial port, so no debug serial port
    #endif

    int err = LoRa.begin();  // initialize the LoRa class
    if (err) {
        if (PARTICLEPHOTON) {
            debugPrintln(F("error initializing LoRa device - Stopping"));
        }
        mgFatalError = true;
        blinkLED(RED_LED_PIN, 20, 50);
    }


    if (!mgFatalError && PARTICLEPHOTON) {
        if (LoRa.configDevice(THIS_LORA_SENSOR_ADDRESS) != 0) {  // initialize the LoRa device
            debugPrintln(F("error configuring LoRa device - Stopping"));
            debugPrintln(F("hint: did you set THIS_LORA_SENSOR_ADDRESS?"));
            mgFatalError = true;
            blinkLED(RED_LED_PIN, 20, 50);
        } else {
            if (LoRa.readSettings() != 0) {  // read the settings from the LoRa device
                debugPrintln(F("error reading LoRa settings - Stopping"));
                mgFatalError = true;
                blinkLED(RED_LED_PIN, 20, 75);      
            }; 

            mgTemp =  F("LoRa Network ID = ");
            mgTemp += LoRa.LoRaNetworkID;
            debugPrintln(mgTemp);
            mgTemp =  F("LoRa UID = ");
            mgTemp += LoRa.UID;
            debugPrintln(mgTemp);
            mgTemp =  F("LoRa device address = ");
            mgTemp += LoRa.LoRaDeviceAddress;
            debugPrintln(mgTemp);
            mgTemp =  F("LoRa parameters = ");
            mgTemp += LoRa.LoRaSpreadingFactor;
            mgTemp += F(", ");
            mgTemp += LoRa.LoRaBandwidth;
            mgTemp += F(", ");
            mgTemp += LoRa.LoRaCodingRate;
            mgTemp += F(", ");
            mgTemp += LoRa.LoRaPreamble;
            debugPrintln(mgTemp);
            mgTemp = F("LoRa CRFOP = ");
            mgTemp += LoRa.LoRaCRFOP;
            debugPrintln(mgTemp);
        }
    }
    
    if (!mgFatalError) {
        LoRa.sleep(); // put the LoRa module to sleep

        debugPrintln(F("Sensor ready for testing ...\n" ));   
        
        blinkLEDsOnBoot();

        digitalWrite(GRN_LED_PIN, LOW);
        digitalWrite(RED_LED_PIN, LOW);
    }
    

} // end of setup()


void loop() {

    static bool awaitingResponse = false; // when waiting for a response from the hub
    static unsigned long startTime = 0;
    static int msgNum = 0;
    bool needToSleep = false;

    // XXX if fatal error then power down the ATmega328
    if (mgFatalError) {
        delay(1000);
        return;
    }   

    // test for button to be pressed and no transmission in progress
    // this is where the power down code will go
    //if(digitalRead(BUTTON_PIN) == LOW && !awaitingResponse) { // button press detected  
    if(mgButtonPressed && !awaitingResponse) { // button press detected 
        digitalWrite(GRN_LED_PIN, HIGH);
        debugPrintln(F("\n\r--------------------")); 
        msgNum++;
        mgpayload = F("HELLO m: ");
        mgpayload += msgNum;
        switch (msgNum) {
            case 1:
                mgpayload += F(" uid: ");
                mgpayload += LoRa.UID;
                break;
            case 2:
                mgpayload += F(" p: ");
                mgpayload +=  F("LoRa parameters = ");
                mgpayload += LoRa.LoRaSpreadingFactor;
                mgpayload += F(":");
                mgpayload += LoRa.LoRaBandwidth;
                mgpayload += F(":");
                mgpayload += LoRa.LoRaCodingRate;
                mgpayload += F(":");
                mgpayload += LoRa.LoRaPreamble;
                break;
            default:
                mgpayload += F(" rssi: ");
                mgpayload += mglastRSSI;
                mgpayload += F(" snr: ");
                mgpayload += mglastSNR;
                break;
        }
        LoRa.transmitMessage(String(TPP_LORA_HUB_ADDRESS), mgpayload); /// send the address as an int 
        mgButtonPressed = false;
        awaitingResponse = true;  
        startTime = millis();
        digitalWrite(GRN_LED_PIN, LOW);
    }

    if(awaitingResponse) {

        if (millis() - startTime > 5000 ) { // wait 5 seconds for a response from the hub
            awaitingResponse = false;  // timed out
            blinkLED(RED_LED_PIN, 1, 250);
            debugPrintln(F("timeout waiting for hub response"));
            needToSleep = true;
        }

        LoRa.checkForReceivedMessage();
        switch (LoRa.receivedMessageState) {
            case -1: // error
                awaitingResponse = false;  // error
                blinkLED(RED_LED_PIN, 7, 250);
                debugPrintln(F("error while waiting for response"));
                needToSleep = true;
                break;
            case 0: // no message
                delay(5); // wait a little while before checking again
                break;
            case 1: // message received
                mgTemp = F("received data = ");
                mgTemp += LoRa.receivedData;
                debugPrintln(mgTemp);
                mglastRSSI = LoRa.RSSI;
                mglastSNR = LoRa.SNR;

                // test for received data from the hub (denoted by "+RCV")
                if(LoRa.receivedData.indexOf(F("+RCV")) >= 0) { // will be -1 of "+RCV" not in the string
                    
                    awaitingResponse = false; // we got a response
                    debugPrintln(F("response received"));

                    if (LoRa.receivedData.indexOf(F("TESTOK")) >= 0) {
                        debugPrintln(F("response is TESTOK"));
                        blinkLED(GRN_LED_PIN, 3, 150);
                    } else if (LoRa.receivedData.indexOf(F("NOPE")) >= 0) {
                        debugPrintln(F("response is NOPE"));
                        blinkLED(GRN_LED_PIN, 4, 250);
                    } else {
                        debugPrintln(F("response is unrecognized"));
                        blinkLED(RED_LED_PIN, 5, 250);
                    }
                } 
                needToSleep = true;
                break;

        } // end of switch(LoRa.receivedMessageState)

    } // end of if(awaitingResponse)

    // xxx this is where the power down code goes
    if (needToSleep) {
        LoRa.sleep(); // put the LoRa module to sleep
    }

} // end of loop()

