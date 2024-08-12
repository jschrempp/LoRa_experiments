/* Basic_ATmega328_LoRa_test.
 *  This is a test of LoRa communication to the "Range Testing Hub" of a bare bones ATmega328 communicating with
 *  the LoRa module via serial I/O.  It is essentally a duplicate of the test performed on 5/04/24 using as Photon
 *  driving the LoRa module.  The code powers up and down the LoRa module, but it does not power down the ATmega328.
 *  
 *  Connections:
 *    LoRa Vdd to +3 volts
 *    LoRa GND to GND
 *    LoRa Tx to ATmega328 RXD (chip pin 2)
 *    LoRa Rx to ATmega328 TXD (chip pin 3)
 *    
 *    A pushbutton signal is connected to ATmega328 pin 4 (Arduino DIO 2, Interrupt 0). The pushbutton is pulled
 *    up with a 1 Mohm resistor and grounds the signal line.  A 0.1 uF capacitor is placed across the pushbutton to reduce noise
 *    sensititvity.  The signal line is connected to a schmitt trigger inverter when is then connected to another inverter, the
 *    output of which goes to ATmega328 pin 4.
 *    
 *    An LED is connected to ATmega328 pin 15 (Arduino DIO 9) through a 330 ohm resistor.
 *    
 *    Interrupts are not used in this test. When the pushbutton is pressed, DIO 2 goes low and this triggers the 
 *    following sequence in code:
 *      the LoRa module is powered up ("AT+MODE=0"); wait for OK response
 *      a message is sent to the Hub ("AT+SEND ..."); wait for OK response
 *      the LoRa module is powered down ("AT+MODE=1"); wait for OK response
 *      the LED is flashed twice
 *      
 *    The LoRa chip is pre-programmed to be station number 5.  The Hub is station number 57248.
 *    The network ID is 18.  loRa parameters are: 9,7,1,24.  All are preprogrammed into the LoRa chip prior to this
 *    test being run.
 *    
 *    Version 1.0; 8/11/24, by Bob Glicksman
 *    (c) 2024 Bob Glicksman, Jim Schrempp, Team Practical projects; all rights reserved.
 *      
*/

#define VERSION 1.00
#define BUTTON_PIN 2  // the button is on Arduino pin 2, chip pin 4
#define LED_PIN 9 // the LED is on Arduino DIO 9, chip pin 15


void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  Serial.begin(38400); // the LoRa device baud rate
  Serial.setTimeout(10);  // a full string is received after 10 ms of no new data from the LoRa device

  blinkLed(5);  // blink the LED to show that setup is complete

} // end of setup()

void loop() {
  // test for the button to be pressed
  if(digitalRead(BUTTON_PIN) == LOW) {  // button press detected    
    // flush out the receive buffer
    Serial.readString();
    
    // wake up the LoRa module
    Serial.println("AT+MODE=0");
    if(waitForOK() == -1) { // only flash the LED if did not get +OK
      blinkLed(1);
    }

    // send out the button pressed message
    Serial.println("AT+SEND=57248,28,HELLO m: 2 rssi: -21 snr: 11");
    if(waitForOK() == -1) { // only flash the LED if did not get +OK
      blinkLed(2);
    }

    // put the LoRa module to sleep
    Serial.println("AT+MODE=1");
    if(waitForOK() == -1) { // only flash the LED if did not get +OK
      blinkLed(3);
    }
    
    // indicate that the message sending process is complete
    blinkLed(5); 
    
    // wait for button pin to go back HIGH    
    while(digitalRead(BUTTON_PIN) == LOW);  // wait for buttom to be released
  }
  delay(100); // wait a little while before sampling the button again

} // end of loop()

void blinkLed(int times) {
  for(int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(250);
    digitalWrite(LED_PIN, LOW);
    delay(250);
  }
  return;
} // end of blinkLed()

int waitForOK() {
  // wait for data in the serial buffer
  while(Serial.available() <=0) {}  // loop until data in the buffer

  // read out the whole string (note: setTimeout(10) ensures that the whole string is read
  String receivedData = Serial.readString();

  // test for "+OK"
  if(receivedData.indexOf("+OK") >= 0) { // got an +OK
    return 0;
  } else {
    return -1;
  }

} // end of waitForOK()
