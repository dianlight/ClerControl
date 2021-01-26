### Install Firmware instruction.

#### Configure

copy include/credentials.h.tpl to include/credentials.h
edit and change the parameters

#### Flash the modified Bootloader ( allow OTA )

pio run -t bootloader -e BootLoader

### Flasg the Main Firmware

pio run -t upload -e ATmega328P_MAIN

### Flash the Satellite Firmware

pio run -t upload -e ATmega328P_SAT


### OTA Instruction

#### Install dependencies

pip3 install paho-mqtt