# Arduino-SignalK
## Introduction
We're working on building a SignalK based system for caravan or boating electronics based on Raspberry PI and Arduino. In this repository we'll provide the Arduino sketches to read in sensors and to display messages. [The central server will be installed on a Raspberry Pi](https://vehicle-hacks.com/signalk-2/use-a-raspberry-pi-as-signalk-server/). More Information about the project is on our ["Use a Raspberry PI as SignalK server"](https://vehicle-hacks.com/signalk-2/) page, [german version is here](https://vehicle-hacks.de/signalk/).

## Arduino-AVR-SignalK
This sketch is for AVR based Arduinos, e.g. the Uno or MEGA. It is sending SignalK messages using SignalK over UDP and a Wiznet W5100 Ethernet shield. Timestamps are acquired using NTP. Input examples are a HC-SR04 Ultrasonic, a Bosch BMP280 and a analog input. More details about the sketch are on our ["Arduino Uno as SignalK Source"](https://vehicle-hacks.com/signalk-2/arduino-uno-as-signalk-source/) page, german version is [here](https://vehicle-hacks.de/signalk/arduino-uno-als-signalk-quelle/).

## Arduino-AVR-Switching
This sketch is for AVR based Ardinos, e.g. the Uno or MEGA. It reads in a push button and on state change sends a PUT-Request to a SignalK-Server. On success, a LED is switched to show to requested state. On failure or while waiting for processing the request, the LED is blinking. More details about the sketch are [here](https://vehicle-hacks.com/2021/05/01/switching-using-signalk-part-2/), german version is [here](https://vehicle-hacks.de/2021/04/30/schalten-mit-signalk-teil-2/).

## Arduino-ESP8266-SignalK
This sketch is for ESP8266 based Arduinos, e.g. the WeMos D1 and the WeMos 0.66" OLED display. It subscribes to two SignalK paths using a WiFi/TCP connection and displays the values. Values can be from an NMEA recording or from and Arduino Uno using the Arduino-AVR-SignalK sketch. More details about the sketch are on our [Receiving SignalK using an ESP8266](https://vehicle-hacks.com/signalk-2/receiving-signalk-using-an-esp8266/) page, german version is [here](https://vehicle-hacks.de/signalk/empfangen-von-signalk-daten-mit-dem-esp8266/).
