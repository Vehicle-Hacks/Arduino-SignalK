/*   Arduino SignalK
 *   Copyright (C) 2019 Ralph Grewe
 *  
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
/* Its based on: The Arduino UDPSendReceiveString example from Nov 3rd 2019: */ 
/* https://www.arduino.cc/en/Tutorial/UDPSendReceiveString */
/* The Arduino UdpNtpClient example from Nov. 25th 2019: */
/* https://www.arduino.cc/en/Tutorial/UdpNtpClient */
/* And a stackoverflow post about converting unix timestamp to a date: */
/* https://stackoverflow.com/questions/7136385/calculate-day-number-from-an-unix-timestamp-in-a-math-way */

/* ------------------------------- Configuration and includes ------------------------------- */
//#define SERIAL_OUTPUT   // Send SignalK messages on serial output.
#define USE_BMX280      // Read in temperature und pressure from BMx280.
#define USE_TANKLEVEL   // Measure tank level using an HC-SR04 Ultrasonic.

#include <Ethernet.h>
#include <EthernetUdp.h>
#include "ntp.h"

/* Basic Ethernet variables*/
byte mac[] = {0x90, 0xA2, 0xDA, 0x11, 0x05, 0xAA};
IPAddress signalkServer(192, 168, 178, 31); //SignalK Server Address
EthernetUDP udp;

/* For SignalK data*/
const unsigned int signalkPort = 8375;      // SignalK Port of the SignalK Server for UDP communication
// buffers for receiving and sending data

/* For NTP */
//const char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server
const char timeServer[] = "fritz.box"; // time.nist.gov NTP server

/* For the HC-SR04 as tank level sensor*/
#if defined (USE_TANKLEVEL)
#include <NewPing.h>
const int echoSendPin = 2;
const int echoReceivePin = 3;
const int maxDistance = 320;
float tankFull  = 0.05;
float tankEmpty = 0.75;
float distance;
float level;
NewPing sonar(echoSendPin, echoReceivePin, maxDistance); // NewPing Setup
#endif

/* For the BMx280 */
#if defined (USE_BMX280)
#include <Wire.h>
#include <BMx280I2C.h>
#define I2C_ADDRESS 0x76
BMx280I2C bmx280(I2C_ADDRESS);
#endif

/* Analog Input */
const int analogPin = A0;
int adcVal = 0;
float analogValue = 0.0;

/* ------------------- SignalK Messages sent by Arduino ------------------- 
 *  We store the messages in flash and load them on usage to save some SRAM
*/
const int maxStringLength = 128;
/* Signalk header */
const char string_0[] PROGMEM = "{"
                           "\"updates\": [{"
                           "\"source\": {"
                           "\"label\": \"Arduino SignalK\","
                           "\"type\": \"signalk\""
                           "},"
                           "\"timestamp\": \"";
/* Signalk values begin */
const char string_1[] PROGMEM = "\",\"values\": [";
/* Signalk message end */
const char string_2[] PROGMEM = "] }] }"; //values "]", updates "}]", final "}"

/* SignalK tank level message */
const char string_3[] PROGMEM = "{"
                              "\"path\": \"tanks.rainwater.currentLevel\","
                              "\"value\":";

/* Signalk environment temperature message */
const char string_4[] PROGMEM = "{"
                            "\"path\": \"environment.inside.galley.temperature\","
                            "\"value\":";
/* SignalK environment pressure message */
const char string_5[] PROGMEM = "{"
                            "\"path\": \"environment.inside.galley.pressure\","
                            "\"value\":";
/* SignalK environment temperature analog input */
const char string_6[] PROGMEM = "{"
                            "\"path\": \"environment.inside.engine.temperature\","
                            "\"value\":";

const char *const string_table[] PROGMEM = {string_0, string_1, string_2, string_3, string_4, string_5, string_6};

/* ----------------------------- Actual Code  -------------------------------- */
void setup() {
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  /*-------------------- Setup Ethernet --------------------------- */
  // use DHCP
  Ethernet.begin(mac);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println(F("No Ethernet Shield"));
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println(F("No Ethernet cable"));
  }

  // start UDP
  if (udp.begin(signalkPort))
  {
    Serial.print(F("UDP Port:"));
    Serial.println(signalkPort);
  } else {
    Serial.println(F("UDP error"));
  }

#if defined (USE_BMX280)
  /*-------------------- Initialize the I2C/TWI ---------------------------*/
  Wire.begin();

  /*-------------------- Setup the BMP280/BME280 --------------------------*/
  if (bmx280.begin())
  {
    bmx280.resetToDefaults();
    bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
    bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
    if (bmx280.isBME280()) {
      bmx280.writeOversamplingHumidity(BMx280MI::OSRS_H_x16);
    }

    if (!bmx280.measure())
    {
      Serial.println(F("BMx280: measure() failed"));
    }
  }
  else
  {
    Serial.println(F("BMx280: begin() failed."));
  }
#endif
}

void loop() {
  char stringBuffer[maxStringLength];

// send NTP request for current time - required for any message.
// after sending the request, we give the server/network some time to answer. That's why we do something in between.
  sendNtpRequest(timeServer);

// read SignalK Header from Flash
  udp.beginPacket(signalkServer, signalkPort);
  strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[0])));
  udp.write(stringBuffer);
#if defined(SERIAL_OUTPUT)
  Serial.print(stringBuffer);
#endif

// Now get and write the NTP timestamp
  readNtpTimestamp(stringBuffer);
  udp.write(stringBuffer);
#if defined(SERIAL_OUTPUT)
  Serial.print(stringBuffer);
#endif

// Get and write value array start
  strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[1])));
  udp.write(stringBuffer);
#if defined(SERIAL_OUTPUT)
  Serial.print(stringBuffer);
#endif

#if defined (USE_TANKLEVEL)
// Read distance from the sonar.
  float duration =  sonar.ping_median(5);
  distance = float(duration * (0.000343/2));       //Abstand in cm
  if (distance < tankFull) {
    level = 1;
  } else if (distance > tankEmpty) {
    level = 0;
  } else {
    level = 1 - (distance-tankFull)/tankEmpty;
  }
// Write the tank level
  strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[3])));
  udp.write(stringBuffer);
  udp.write(String(level).c_str());
  udp.write("},");
#if defined(SERIAL_OUTPUT)
  Serial.print(stringBuffer);
  Serial.print(String(level));
  Serial.print("},");
#endif
#endif

#if defined (USE_BMX280)
// Get and write temperature/pressure from the BMP280
  if (bmx280.hasValue()) 
  { 
    float value = bmx280.getTemperature();
    strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[4])));
    udp.write(stringBuffer);
    udp.write(String(value).c_str());
    udp.write(", \"units\": \"K\"},");
    #if defined (SERIAL_OUTPUT)
    Serial.print(stringBuffer);
    Serial.print(String(value));
    Serial.print(", \"units\": \"K\"},");
    #endif

    value = bmx280.getPressure();
    strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[5])));
    udp.write(stringBuffer);
    udp.write(String(value).c_str());
    udp.write(", \"units\": \"Pa\"},");
    #if defined (SERIAL_OUTPUT)
    Serial.print(stringBuffer);
    Serial.print(String(value));
    Serial.print(", \"units\": \"Pa\"},");
    #endif

    if (!bmx280.measure()) {
      Serial.print(F("bmx280: could not start measurement"));
    }
  }
#endif
  
  // read and convert ADC values;
  adcVal = analogRead(analogPin);
  analogValue = (adcVal / 1024.0) * 360.0;
  strcpy_P(stringBuffer, (char * )pgm_read_word(&(string_table[6])));
  udp.write(stringBuffer);
  udp.write(String(analogValue).c_str());
  udp.write("}");
#if defined (SERIAL_OUTPUT)
  Serial.print(stringBuffer);
  Serial.print(String(analogValue));
  Serial.print("}");
#endif

// Finalize message and send it.
  strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[2])));  // Necessary casts and dereferencing, just copy.  
  udp.write(stringBuffer);
  udp.endPacket();
#if defined(SERIAL_OUTPUT)
  Serial.println(stringBuffer);
#endif
}
