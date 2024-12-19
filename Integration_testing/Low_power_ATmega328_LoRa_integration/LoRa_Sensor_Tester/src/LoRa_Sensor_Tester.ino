/* 
 * Project LoRa Sensor Tester
 * Author: Bob Glicksman
 * Date: 12/18/24
 * 
 * This project is a Photon 1 based tester for a LoRa sensor.  It uses the TPP Wireless I/O Board
 *  with a relay installed and an LED connected to the LED terminal block.  The Tester simply loops 
 *  a preset number of times, closing the relay and  opening it at 10 second intervals.  
 *  The results of the sensor tests can be seen in the Google spreadsheet logs.
 * 
 * Version 1.00 - Initial release.
 * 
 *  (c) 2024, by Bob Glicksman, Jim Schrempp, Team Practical Projects
 */

// Include Particle Device OS APIs
#include "Particle.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

#define NUMBER_OF_SENSOR_TRIPS 1000
#define SENSOR_ON_TIME 2000
#define SENSOR_OFF_TIME 8000

#define RELAY_PIN D0
#define LED_PIN D5
#define INTERNAL_LED_PIN D7


void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(INTERNAL_LED_PIN, OUTPUT);

  // signal that the Photon is reset and getting ready to test
  digitalWrite(INTERNAL_LED_PIN, HIGH);
  delay(5000); 
  digitalWrite(INTERNAL_LED_PIN, LOW);

  // loop through NUMBER_OF_SENSOR_TRIPS times, closing and opening the relay
  for(int i = 0; i < NUMBER_OF_SENSOR_TRIPS; i++) {
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    delay(SENSOR_ON_TIME);
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    delay(SENSOR_OFF_TIME);
  }


}


void loop() {

  // Nothing here!

}
