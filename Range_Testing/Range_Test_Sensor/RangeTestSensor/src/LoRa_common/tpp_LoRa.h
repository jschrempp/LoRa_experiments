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

#define LORA_SERIAL Serial1

// class for the LoRa module
class tpp_LoRa
{
private:
    /* data */
    void clearClassVariabels();

public:
    
    // Read current settings and print them to the serial monitor
    //  If error then return false
    bool readSettings(); 

    // check for a received message from the LoRa module. status in receivedMessageState
    // if successful, the received data is stored in the receivedData variable
    // and other class variables. If not, the class variables are set to default
    void checkForReceivedMessage();
    
    // function to send AT commands to the LoRa module
    // returns 0 if successful, -1 if not successful
    // prints message and result to the serial monitor
    int sendCommand(String command);

    int receivedMessageState = 0; // 0 = no message, 1 = message received, - 1 = error
    String parameters = "";
    String receivedData = "";
    String loraStatus = "";
    String deviceNum = "";
    String payload = "";
    String RSSI = "";
    String SNR = "";

};


#endif