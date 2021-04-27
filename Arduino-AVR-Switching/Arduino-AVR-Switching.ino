/*   Arduino SignalK
 *   Copyright (C) 2021 Ralph Grewe
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

/* ------------------------------- Configuration and includes ------------------------------- */

#include <Ethernet.h>
#include <EthernetUdp.h>
#include <EthernetClient.h>
#include <ArduinoHttpClient.h>
#include "ntp.h"

/* Basic Ethernet variables*/
byte mac[] = {0x90, 0xA2, 0xDA, 0x11, 0x05, 0xAA};
IPAddress signalkServer(192, 168, 178, 53); //SignalK Server Address
EthernetUDP udp;
EthernetClient ethernetClient;
WebSocketClient webSocketClient = WebSocketClient(ethernetClient, signalkServer, 3000);

/* For SignalK data*/
const unsigned int signalkPort = 8375;      // SignalK Port of the SignalK Server for UDP communication
// buffers for receiving and sending data

/* For NTP */
//const char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server
const char timeServer[] = "fritz.box"; // time.nist.gov NTP server

/* Analog Input */
const int analogPin = A0;
int adcVal = 0;
float analogValue = 0.0;

/* Switch Input */
const int switchPin = 4;
bool switchOutput = false;
bool switchPressed = false;

/* ------------------- SignalK Messages sent by Arduino ------------------- 
 *  We store the messages in flash and load them on usage to save some SRAM
*/
const int maxStringLength = 128;
char stringBuffer[maxStringLength];

/* Signalk request header */
const char string_0[] PROGMEM = "{"
                           "\"requestId\": \"";
const char string_1[] PROGMEM = "\",\"put\":{"
                                  "\"path\":\"electrical.testswitch\","
                                  "\"value\":";

const char *const string_table[] PROGMEM = {string_0, string_1};

/* ----------------------------- Actual Code  -------------------------------- */
void switchStateChanged(int state) {
  if (webSocketClient.connected()) {
    // SignalK put request start
    webSocketClient.beginMessage(TYPE_TEXT);
    strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[0])));
    webSocketClient.print(stringBuffer);
    Serial.print(stringBuffer);

    // SignalK put request ID
    webSocketClient.print("12345-12345-12345");
    Serial.print("12345-12345-12345");

    strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[1])));
    webSocketClient.print(stringBuffer);
    Serial.print(stringBuffer);
    webSocketClient.print(String(state).c_str());
    Serial.print(String(state).c_str());
    webSocketClient.print("}}");
    Serial.print("}}");
    int result = webSocketClient.endMessage();
    if (result) {
      Serial.print("Error writing packet: ");
      Serial.println(result);
    } else {
      Serial.println("Sent message");    
    }
  } else {
    Serial.println(F("Error: Websocket not connected"));
  }
  
}

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

  //start Websocket client
  webSocketClient.begin(F("/signalk/v1/stream"));

  // start UDP
  if (udp.begin(signalkPort))
  {
    Serial.print(F("UDP Port:"));
    Serial.println(signalkPort);
  } else {
    Serial.println(F("UDP error"));
  }

  pinMode(switchPin, INPUT);
}

void loop() {
  char stringBuffer[maxStringLength];

// send NTP request for current time - required for any message.
// after sending the request, we give the server/network some time to answer. That's why we do something in between.
  if (webSocketClient.connected()) {  
    sendNtpRequest(timeServer);
    readNtpTimestamp(stringBuffer);
  }

  int messageSize = webSocketClient.parseMessage();
  if (messageSize > 0) {
    Serial.println("Received a message:");
    Serial.println(webSocketClient.readString());
  }
  
  int switchState = digitalRead(switchPin);
  if (switchState == HIGH) {
    if (switchPressed == false) {
      /* Switch just changed state to high, so do something */
      switchPressed = true;
      if (switchOutput == false) {
        Serial.println("Switch on");
        switchStateChanged(1);
        switchOutput = true;
      } else {
        Serial.println("Switch off");
        switchStateChanged(0);
        switchOutput = false;
      }
    }
  } else {
    switchPressed = false;
  }
}
