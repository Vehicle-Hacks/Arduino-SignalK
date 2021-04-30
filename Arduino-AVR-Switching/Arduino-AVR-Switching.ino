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
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

/* Basic Ethernet variables*/
byte mac[] = {0x90, 0xA2, 0xDA, 0x11, 0x05, 0xAA};
IPAddress signalkServer(192, 168, 178, 53); //SignalK Server Address
EthernetClient ethernetClient;
WebSocketClient webSocketClient = WebSocketClient(ethernetClient, signalkServer, 3000);

/* Switch Input */
const int switchPin = 4;
bool switchOutput = false;
bool switchPressed = false;

/* LED Output */
const int ledPin = 5;
int targetState = 0;
bool ledBlinking = false;
bool ledState = 0;
unsigned long lastBlink = 0;

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

  pinMode(switchPin, INPUT);
  pinMode(ledPin, OUTPUT);

  lastBlink = millis();
}

void loop() {
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
        Serial.println(id);
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
      } else {
        switchStateChanged(0);
        targetState = 0;
        ledBlinking = true;
        switchOutput = false;
      }
    }
  } else {
    switchPressed = false;
  }

  if (ledBlinking) {
    if (millis() - lastBlink > 250) {
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      lastBlink = millis();
    }
  }
}
