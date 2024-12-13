/*
    tpp_LoRaGlobals.h

    Include this in all modules of the LoRa sensor and hub

    20241212 - works on Particle Photon 2. Verified on ATMega328
    
    (c) 2024 Bob Glicksmand and Jim Schrempp

*/

#ifndef tpp_LoRaGlobals_h 
#define tpp_LoRaGlobals_h


#define PARTICLEPHOTON 0

#if PARTICLEPHOTON
    #include "Particle.h"
    #define LORA_SERIAL Serial1
    #define DEBUG_SERIAL Serial
    // CONSTANTS 
    const int BUTTON_PIN = D0;   // the pushbutton is on digital pin 2 which is chip pin 4
    const int GRN_LED_PIN = D7;  // the Green LED is on digital pin 7 which is the onboard LED
    const int RED_LED_PIN = D8;  // the Red LED is on digital pin 8 
#else
    #include "arduino.h" 
    // ATMega328 has only one serial port, so no debug serial port
    #define LORA_SERIAL Serial
    // CONSTANTS  
    const int BUTTON_PIN = 2;   // Interrupt 0 is Arduino pin 2 is chip pin 4, external pullup with schmitt trigger is used.
    const int GRN_LED_PIN = 9;  // the Green LED is on digital pin 9 which is chip pin 15
    const int RED_LED_PIN = 8;  // the Red LED is on digital pin 8 which is chip pin 14
#endif

#endif