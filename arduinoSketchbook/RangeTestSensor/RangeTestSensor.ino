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

    20241212 - version 2. works on Particle Photon 2 
    v 2.1 pulled all string searches out of if() clauses
    v 2.2 #define for continuous test mode
    v 2.3 works on ATmega328
    v 2.4 integrates ATmega power down code
    v 2.5 now calls setAddress for the LoRa module in setup
    v 2.6 processes address jumper pins to alter the sensor device address
    v 2.7 address lines work for P2
    v 2.8 #define to not wait for hub response
    v 2.9 messages are now shorter
 */

#include "tpp_LoRaGlobals.h"

#include "tpp_loRa.h" // include the LoRa class

#define CONTINUOUS_TEST_MODE 0 // set to 1 to enable continuous testing
#define WAIT_FOR_RESPONSE_FROM_HUB 1 // set ot 0 to disable waiting for a response from the hub

// The following system directives are to disregard WiFi for Particle devices.  Not needed for Arduino.
#if PARTICLEPHOTON
    SYSTEM_MODE(SEMI_AUTOMATIC);
    SYSTEM_THREAD(ENABLED);
    // Show system, cloud connectivity, and application logs over USB
    // View logs with CLI using 'particle serial monitor --follow'
    // SerialLogHandler logHandler(LOG_LEVEL_INFO);
#else
    // ATMega328
    #include <avr/sleep.h>  // the official avr sleep library
#endif

#define VERSION 2.9
#define STATION_NUM 0 // housekeeping; not used ini the code

#define LORA_TRIP_SENSOR_ADDRESS_BASE 5 // the base address of the trip sensor type

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
    digitalWrite(ledpin, LOW);
    delay(100);
    for(int i = 0; i < number; i++) {
        digitalWrite(ledpin, HIGH);
        delay(delayTimeMS);
        digitalWrite(ledpin, LOW);
        delay(delayTimeMS);
    }
    return;
} // end of blinkLED()

// blinkLEDsOnERROR() blinks red led fast Where times and then slow ErrNum times
//    then repeats. Never exits.
void blinkLEDsOnERROR(int whereTimes, int errNum){
    while(1) {
        blinkLED(RED_LED_PIN, 1, 1000);
        delay(250);
        blinkLED(RED_LED_PIN, whereTimes, 150);
        delay (500);
        blinkLED(RED_LED_PIN, errNum, 250);
        delay (500);
        blinkLED(GRN_LED_PIN,2,150);
        delay(250);
    }
}

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

void ISR_wakeAndSend() {
    #if (PARTICLEPHOTON)
        // nothing special to do
    #else
        // ATMega328
        sleep_disable();  // cancel sleep mode for now
        detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));  // preclude more interrupts due to bounce, or other
    #endif
    mgButtonPressed = true;
}

void setup() {

    #if PARTICLEPHOTON
        pinMode(BUTTON_PIN, INPUT_PULLUP);
        attachInterrupt(BUTTON_PIN, ISR_wakeAndSend, FALLING);
    #else
        // ATmegs328
        pinMode(BUTTON_PIN, INPUT);
        // attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), ISR_wakeAndSend, FALLING);
    #endif

    pinMode(GRN_LED_PIN, OUTPUT); 
    pinMode(RED_LED_PIN, OUTPUT); 

    digitalWrite(GRN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, HIGH);

    mgpayload.reserve(75);
    mgTemp.reserve(75);

    #if PARTICLEPHOTON
        DEBUG_SERIAL.begin(115200); // the USB serial port
        waitFor(DEBUG_SERIAL.isConnected, 15000);
    #else
        // ATMega328 has only one serial port, so no debug serial port
    #endif

    int err = LoRa.begin();  // initialize the LoRa class
    if (err) {
        mgFatalError = true;
        blinkLEDsOnERROR(15,err);
    }

    // xxx read the address jumers and compute the device's address
    // set address line pin mode for pullup temporarily
    unsigned int deviceAddress = LORA_TRIP_SENSOR_ADDRESS_BASE;
  
    pinMode(ADR4_PIN, INPUT_PULLUP);
    pinMode(ADR2_PIN, INPUT_PULLUP);
    pinMode(ADR1_PIN, INPUT_PULLUP);

    // compute the device's address based upon jumpers
    if(digitalRead(ADR4_PIN) == HIGH) {
      deviceAddress += 4;
    }
    if(digitalRead(ADR2_PIN) == HIGH) {
      deviceAddress += 2;
    }
    if(digitalRead(ADR1_PIN) == HIGH) {
      deviceAddress += 1;
    }

    // change the address pins to eliminate pullups for lowest power consumption when sleeping
    pinMode(ADR4_PIN, INPUT);
    pinMode(ADR2_PIN, INPUT);
    pinMode(ADR1_PIN, INPUT);

    err = LoRa.setAddress(deviceAddress);
    if (err) {
        mgFatalError = true;
        blinkLEDsOnERROR(13,err);
    }

    // XXX send out a message for testing resets.  Comment this out after stree testing.

    //mgpayload = F("The ATMega328 has reset");
    //LoRa.transmitMessage(TPP_LORA_HUB_ADDRESS, mgpayload);

    // XXX end of reset test message
    
    if (!mgFatalError) {
        int errRtn = LoRa.sleep(); // put the LoRa module to sleep
        errRtn = errRtn; // to avoid a warning
        
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

    // if fatal error then 
    if (mgFatalError) {
        if (PARTICLEPHOTON) {
            delay(10); // loop forever
            return;
        } else { 
            //power down the ATmega328
        } 
    }
           
    if (CONTINUOUS_TEST_MODE) {
        if (!awaitingResponse) {
            delay(100);
            mgButtonPressed = true;
        }
    }

    #if (PARTICLEPHOTON)
        // nothing special to do
    #else
        // ATMega328

        // the first thing that we do is put the microcontroller to deep sleep; awake only on falling edge on interrupt 0.
        
        // disable the adc
        ADCSRA = 0;
        
        set_sleep_mode (SLEEP_MODE_PWR_DOWN); // the lowest possible low power mode
        sleep_enable(); // not sleeping yet!

        noInterrupts(); // disable interrupts until we actually go to sleep.
        attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), ISR_wakeAndSend, FALLING); // ready the wakeup interrupt
        EIFR = bit(INTF0);  // clear flag for interrupt 0

        // turn off brown-out enable in software
        // BODS must be set to one and and BODSE must be set to zero within 4 clock cycles
        MCUCR = bit(BODS) | bit (BODSE);
        // the BODS bit is automatically cleared after 3 clock cycles
        MCUCR = bit(BODS);

        interrupts(); // enable interrupts just before sleeping
        sleep_cpu();

        //  everything should now be in deep sleep.
        // Interrupt 0 wakes up the ATmega328 - send the message and go back to sleep
    #endif
 
     // test for button to be pressed and no transmission in progress
     if(mgButtonPressed && !awaitingResponse) { // button press detected 
        digitalWrite(GRN_LED_PIN, HIGH);
        debugPrintln(F("\n\r----- button press ----------"));
        int errRtn = LoRa.wake();
        if (errRtn) {
            blinkLEDsOnERROR(2,errRtn);
        }
        msgNum++;
        mgpayload = TPP_LORA_MSG_GATE_SENSOR;
        mgpayload += F(" m: ");
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

                break;
        }
        errRtn = LoRa.transmitMessage(TPP_LORA_HUB_ADDRESS, mgpayload); /// send the address as an int 
        if (errRtn != 0) {
            blinkLEDsOnERROR(7,errRtn);
        }
        mgButtonPressed = false;
        awaitingResponse = true;  
        startTime = millis();
        digitalWrite(GRN_LED_PIN, LOW);
    }

    if (WAIT_FOR_RESPONSE_FROM_HUB == 0) {
        awaitingResponse = false;
        needToSleep = true;
    }

    while(awaitingResponse) {

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
                int rcvIndex = LoRa.receivedData.indexOf(F("+RCV"));
                if(rcvIndex >= 0) { // will be -1 of "+RCV" not in the string
                    
                    awaitingResponse = false; // we got a response
                    debugPrintln(F("response received"));
                    int testokIndex = LoRa.receivedData.indexOf(F("TESTOK"));
                    if (testokIndex >= 0) {
                        debugPrintln(F("response is TESTOK"));
                        blinkLED(GRN_LED_PIN, 3, 150);
                    } else {
                        int nopeIndex = LoRa.receivedData.indexOf(F("NOPE"));
                        if (nopeIndex >= 0) {
                            debugPrintln(F("response is NOPE"));
                            blinkLED(GRN_LED_PIN, 4, 250);
                        } else {
                            debugPrintln(F("response is unrecognized"));
                            blinkLED(RED_LED_PIN, 5, 250);
                        }
                    }
                } 
                needToSleep = true;
                break;

        } // end of switch(LoRa.receivedMessageState)

    } // end of while(awaitingResponse)

    if (needToSleep) {
        int errRtn = LoRa.sleep(); // put the LoRa module to sleep
        if (errRtn) {
            blinkLEDsOnERROR(9, errRtn);
        }
        needToSleep = false;
    }

} // end of loop()
