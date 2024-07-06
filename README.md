# LoRa_experiments
## Tests of LoRA module (RYLR998) ranging and low power.

The Reyax RYLR998 is an inexpensive LoRa "modem".  It can perform point to point wireless data communication with other such modules.

The RYLR998 contains both a LoRa chip and a low power microcontroller.  A host microcontroller communicates with an RLYR998 using
serial port "AT" commands, making the RYLR998 very easy to use.  In addition, the RYLR998 has a low power sleep mode that reduces current
draw to about 10 microamps.  The use of LoRa for long distance, low volume data communication along with the ability to sleep the module
when not transmitting suggests that these modules could be ideal for use as long range contact open/close sensors.

The purpose of the tests documented in this repository is to explore the RYLR998 module capabilities in two respects:

- Long range operation:  the ability of the module to successfully commmunicate from an outdoor location some distance from a house
into a receiver located inside of the house.

- Low power operation:  the ability of the module to be powered down when not needed to transmit a short message, in support
of battery powered operation.


There are two main folders in this repository:

- Range_Testing: this folder contains a report and source code for range testing experiments.

- Low_Power_Testing: this folder contains a report and source code for testing low power operation of the module. It also contains
a spreadsheet to calculate battery life, based upon using an Attiny85 as the host microcontroller with a reset pulse generator circuit 
to power it up for transmitting a short message and then powering the LoRa module down and then powering itself down until the next reset.

## LoRa set up ##
- LoRa AT command guide https://lemosint.com/wp-content/uploads/2021/11/Lora_AT_Command_RYLR998_RYLR498_EN.pdf

Note that for these tests you need to program each LoRa module by hand using a terminal emulator and an FTDI module. Connect four
pins from the FTDI to the LoRa module (leave the reset pin on the LoRa module unconnected):

```
FTDI       LoRa
3v3        VDD
TXD        RXD
RXD        TXD
GND        GND
```

Plug the FTDI device into your laptop.

You can use the Arduino IDE Serial Monitor. With the IDE running go to Tools / Port and select the port of your FTDI device (it might be 
something like `/dev/serial-1`). Then click the Serial Monitor button on the upper right of the Arduino IDE. This brings up a new window.
In the bottom menu select "Both NL & CR" and "115200 baud". In the command window at the top type AT and click Send. You should get the 
response +OK

Assuming you are now connected correctly to your LoRa module, perform the following commands:
```
AT+ADDRESS=0
AT+NETWORKID=3
```
Do the same for your second LoRa module.

  


