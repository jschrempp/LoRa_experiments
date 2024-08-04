/*
    tpp_LoRa.h - routines for communication with the LoRa module
    created by Bob Glicksman and Jim Schrempp 2024
    as part of Team Practical Projects (tpp)

*/
/*
    The block below was recommended by CoPilo. It has nothing to do with our libary.
    tpp_LoRa.h - Library for LoRa communication with the Things Plus Plus board.
    Created by Bennett Marsh, 2021.
    Released into the public domain.

*/
#ifndef tpp_LoRa_h 
#define tpp_LoRa_h

#define VERSION 1.00

#define TPP_LORA_HUB_ADDRESS 1

#define LORA_SERIAL Serial1

#define LoRaNETWORK_NUM 18
#define LoRaADDRESS_SENSOR 0
#define LoRaADDRESS_HUB 1
#define LoRaSPREADING_FACTOR 9  // 7 - 11  larger is better for range; default 9
                        // SF7to SF9 at 125kHz, SF7 to SF10 at 250kHz, and SF7 to SF11 at 500kHz
#define LoRaBANDWIDTH 7   // 7:125, 8:250, 9:500 kHz   lower is better for range; default 7
#define LoRaCODING_RATE 4  // 1 is faster; default 1 [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LoRaPREAMBLE 24  // 12 max unless network number is 18; 

// class for the LoRa module
class tpp_LoRa
{
private:
    /* data */
    void clearClassVariabels();

public:
    
    // Initialize the LoRa module with settings found in the tpp_LoRa.h file
    bool initDevice(int devAddress);

    // Read current settings and print them to the serial monitor
    //  If error then return false
    bool readSettings(); 

    // check for a received message from the LoRa module. status in receivedMessageState
    // if successful, the received data is stored in the receivedData variable
    // and other class variables. If not, the class variables are set to default
    void checkForReceivedMessage();
    
    // function to send AT commands to the LoRa module
    // returns 0 if successful, 1 if error, -1 if no response
    // prints message and result to the serial monitor
    int sendCommand(String command);

    int receivedMessageState = 0; // 0 = no message, 1 = message received, -1 = error
    String parameters = "";
    String receivedData = "";
    String loraStatus = "";
    String deviceNum = "";
    String payload = "";
    String RSSI = "";
    String SNR = "";

};


#endif