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
#include <EthernetClient.h>
#include <EthernetUdp.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <hss-board-arduino.hpp>

/* Basic Ethernet variables*/
byte mac[] = {0x90, 0xA2, 0xDA, 0x11, 0x05, 0xAA};
IPAddress signalkServer(192, 168, 188, 28); //SignalK Server Address
EthernetClient ethernetClient;
WebSocketClient webSocketClient = WebSocketClient(ethernetClient, signalkServer, 3000);

EthernetUDP udp;
const int signalkPort = 8375;

/* Switch Input */
const int switchPin = 49;
bool switchOutput = false;
bool switchPressed = false;

/* LED Output */
const int ledPin = 48;
int targetState = 0;
bool ledBlinking = false;
bool ledState = 0;
unsigned long lastBlink = 0;

/* Power Output by SmartFET'S */
HssBoardIno HSS = HssBoardIno(&BTS7002);

/* ------------------- SignalK Messages sent by Arduino ------------------- 
 *  We store the messages in flash and load them on usage to save some SRAM
*/
const int maxStringLength = 512;
char stringBuffer[maxStringLength];

/* Signalk request messages */
const char string_0[] PROGMEM = "{"
                           "\"requestId\": \"";
const char string_1[] PROGMEM = "\",\"put\":{"
                                  "\"path\":\"electrical.inputs.testinput\","
                                  "\"value\":";

/* SignalK Switch State messages */
const char string_2[] PROGMEM = "{"
                           "\"updates\": [{"
                           "\"source\": {"
                           "\"label\": \"Arduino SignalK\","
                           "\"type\": \"signalk\""
                           "},"
                           "\"timestamp\": \"";
/* Signalk values begin */
const char string_3[] PROGMEM = "\",\"values\": [";
/* Signalk message end */
const char string_4[] PROGMEM = "] }] }"; //values "]", updates "}]", final "}"

/* SignalK tank level message */
const char string_5[] PROGMEM = "{"
                              "\"path\": \"electrical.load.1.";
const char string_6[] PROGMEM = "voltage\",";
const char string_7[] PROGMEM = "current\",";
const char string_8[] PROGMEM = "\"value\":";                                 

const char *const string_table[] PROGMEM = {string_0, string_1, string_2, string_3, string_4, string_5, string_6, string_7, string_8};


/* ----------------------------- Actual Code  -------------------------------- */
void switchStateChanged(int state) {
  if (webSocketClient.connected()) {
    // SignalK put request start
    webSocketClient.beginMessage(TYPE_TEXT);
    strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[0])));
    webSocketClient.print(stringBuffer);

    // SignalK put request ID
    webSocketClient.print("12345-12345-12345");

    strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[1])));
    webSocketClient.print(stringBuffer);
    webSocketClient.print(String(state).c_str());
    webSocketClient.print("}}");
    int result = webSocketClient.endMessage();
    if (result) {
      Serial.print(F("Error writing packet: "));
      Serial.println(result);
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
  Serial.println("Initializing...");

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
  webSocketClient.begin(F("/signalk/v1/stream?subscribe=none"));

  //initialize switch and LED
  pinMode(switchPin, INPUT);
  pinMode(ledPin, OUTPUT);

  lastBlink = millis();

  // start UDP
  if (udp.begin(signalkPort))
  {
    Serial.print(F("UDP Port:"));
    Serial.println(signalkPort);
  } else {
    Serial.println(F("UDP error"));
  }  

  //initialize SmartFET Board
  HSS.init();
}

void loop() {
  // Check if we received a message from the server and process it
  int messageSize = webSocketClient.parseMessage();
  if (messageSize > 0) {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, webSocketClient);
    if (error) {
      Serial.print(F("Can not parse JSON: "));
      Serial.println(error.f_str());
    } else {
      if (doc.containsKey("requestId")) {
        const char* id = doc["requestId"];
        int statusCode = doc["statusCode"];
        if (statusCode == 200) {
          ledBlinking = false;
          digitalWrite(ledPin, targetState);
        } else {
          Serial.print(F("Switching Error: "));
          Serial.println(String(statusCode));
        }
      } else if (doc.containsKey("name")) {
        Serial.print(F("Connected to: "));
        Serial.println(doc["name"].as<const char*>());
        Serial.print(F("Version: "));
        Serial.println(doc["version"].as<const char*>());
      }
    }
  }

  // Check if the switch is pressed and process it
  int switchState = digitalRead(switchPin);
  if (switchState == HIGH) {
    if (switchPressed == false) {
      /* Switch just changed state to high, so do something */
      switchPressed = true;
      if (switchOutput == false) {
        switchStateChanged(1);
        targetState = 1;
        ledBlinking = true;
        switchOutput = true;
        HSS.switchHxOn(1);
      } else {
        switchStateChanged(0);
        targetState = 0;
        ledBlinking = true;
        switchOutput = false;
        HSS.switchHxOff(1);
      }
    }
  } else {
    switchPressed = false;
  }

  // Make the LED blinking while waiting for a response from the server
  if (ledBlinking) {
    if (millis() - lastBlink > 250) {
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      lastBlink = millis();
    }
  }

  // Send switch data
  udp.beginPacket(signalkServer, signalkPort);
  strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[2])));
  udp.write(stringBuffer);
  udp.write("0");
  strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[3])));
  udp.write(stringBuffer);
  
  /* Voltage */
  strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[5])));
  udp.write(stringBuffer);
  strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[6])));
  udp.write(stringBuffer);
  strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[8])));
  udp.write(stringBuffer);

  float value = HSS.readVss();
  udp.write(String(value).c_str());
  udp.write("},");

  /* Current */
  strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[5])));
  udp.write(stringBuffer);
  strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[7])));
  udp.write(stringBuffer);
  strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[8])));
  udp.write(stringBuffer);
  
  value = HSS.readIsx(1);
  udp.write(String(value).c_str());
  udp.write("}");
  
  /* End of message */
  strcpy_P(stringBuffer, (char *)pgm_read_word(&(string_table[4])));
  udp.write(stringBuffer);

  udp.endPacket();  
}
