# Arduino-SignalK
## Introduction
We're working on building a SignalK based system for caravan or boating electronics based on Raspberry PI and Arduino. In this repository we'll provide the Arduino sketches to read in sensors and to display messages. [The central server will be installed on a Raspberry Pi](https://vehicle-hacks.com/signalk-2/use-a-raspberry-pi-as-signalk-server/). More Information about the project is on our ["Use a Raspberry PI as SignalK server"](https://vehicle-hacks.com/signalk-2/) page, [german version is here](https://vehicle-hacks.de/signalk/).

## Arduino-AVR-Signalk
This sketch is for AVR based Arduinos, e.g. the Uno or MEGA. It is sending SignalK messages using SignalK over UDP and a Wiznet W5100 Ethernet shield. Timestamps are accuired using UDP. Input examples are a HC-SR04 Ultrasonic, a Bosch BMP280 and a analog input. More details about the sketch are on our ["Arduino Uno as SignalK Source"](https://vehicle-hacks.com/signalk-2/arduino-uno-as-signalk-source/) page, german version is [here](https://vehicle-hacks.de/signalk/arduino-uno-als-signalk-quelle/).
