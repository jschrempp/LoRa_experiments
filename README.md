# LoRa_experiments
 Tests of LoRA module (RYLR998) ranging and low power.

The Reyax RYLR998 is an inexpensive LoRa "modem".  It can perform point to point wireless data communication with other such modules.

The RYLR998 contains both a LoRa chip and a low power microcontroller.  A host microcontroller communicates with an RLYR998 using
serial port "AT" commands, making the RYLR998 very easy to use.  In addition, the RYLR998 has a low power sleep mode that reduces current
draw to about 10 microamps.  The use of LoRa for long distance, low volume data communication along with the ability to sleep the module
when not transmitting suggests that these modules could be ideal for use as long range contact open/close sensors.

The purpose of the tests documented in this repository is to explore the RYLR998 module capabilities in two respects:

- Long range operation:  the ability of the module to successfully commmunicate from an outdoor location some distance frorm a house
into a receiver located inside of the house.

- Low power operation:  the ability of the module to be powered down when not needed to transmit a short message, in support
of battery powered operation.


There are two main folders in this repository:

- Range_Testing: this folder contains a report and source code for range testing experiments.

- Low_Power_Testing: this folder contains a report and source code for testing low power operation of the module.



