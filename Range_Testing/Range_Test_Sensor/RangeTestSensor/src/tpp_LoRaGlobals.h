/*
    tpp_LoRaGlobals.h

    Include this in all modules of the LoRa sensor and hub

    20241212 - works on Particle Photon 2
    
    (c) 2024 Bob Glicksmand and Jim Schrempp

*/

#ifndef tpp_LoRaGlobals_h 
#define tpp_LoRaGlobals_h


#define PARTICLEPHOTON 1

#if PARTICLEPHOTON
    #include "Particle.h"
    #define LORA_SERIAL Serial1
    #define DEBUG_SERIAL Serial
    // CONSTANTS 
    const int BUTTON_PIN = D10;   // the pushbutton is on Photon 2 pin D10/WKP
    const int GRN_LED_PIN = D2;  // the Green LED is on Photon 2 pin D2
    const int RED_LED_PIN = S4;  // the Red LED is on Photon 2 pin S4/D19
    // xxx additional pins for device address customization
    const int ADR1_PIN = D3;  // the device address = BASE_DEVICE_ADDRESS + (ADR4 + ADR2 + ADR1)
    const int ADR2_PIN = D4;  // the device address = BASE_DEVICE_ADDRESS + (ADR4 + ADR2 + ADR1)
    const int ADR4_PIN = D5;  // the device address = BASE_DEVICE_ADDRESS + (ADR4 + ADR2 + ADR1)
#else
    #include "arduino.h" 
    // ATMega328 has only one serial port, so no debug serial port
    #define LORA_SERIAL Serial
    // CONSTANTS  
    const int BUTTON_PIN = 2;   // Interrupt 0 is Arduino pin 2 is chip pin 4, external pullup with schmitt trigger is used.
    const int GRN_LED_PIN = 9;  // the Green LED is on digital pin 9 which is chip pin 15
    const int RED_LED_PIN = 8;  // the Red LED is on digital pin 8 which is chip pin 14
    // xxx additional pins for device address customization
    const int ADR1_PIN = 10;  // the device address = BASE_DEVICE_ADDRESS + (ADR4 + ADR2 + ADR1)
    const int ADR2_PIN = 11;  // the device address = BASE_DEVICE_ADDRESS + (ADR4 + ADR2 + ADR1)
    const int ADR4_PIN = 12;  // the device address = BASE_DEVICE_ADDRESS + (ADR4 + ADR2 + ADR1)

#endif

#endif
