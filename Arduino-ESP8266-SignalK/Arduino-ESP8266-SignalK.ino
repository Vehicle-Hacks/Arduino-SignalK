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

// For WiFi communication
#include <ESP8266WiFi.h>
#include <StreamUtils.h>

// for the OLED Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_RESET);

#include <ArduinoJson.h>


// Basic Ethernet variabled
WiFiClient client;

#define DATA_FROM_FILE
//#define DATA_FROM_ARDUINO

// For SignalK data
IPAddress signalkServer(192, 168, 4, 1);
const unsigned int signalkPort = 8375;      // SignalK Port which is the same for TCP and UDP

#if defined (DATA_FROM_FILE)
//Subscription string for NMEA from File
const char subscriptionString[] = "{\"context\":\"vessels.self\",\"subscribe\":[{\"path\":\"navigation.speedThroughWater\"},{\"path\":\"environment.depth.belowTransducer\"}]}";
#endif

#if defined (DATA_FROM_ARDUINO)
//Subscription string for data from Arduino Uno
const char subscriptionString[] = "{\"context\":\"vessels.self\",\"subscribe\":[{\"path\":\"tanks.rainwater.currentLevel\"},{\"path\":\"environment.inside.galley.temperature\"}]}";
#endif

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print("Connecting WiFi...");
  display.display();  

  //-------------------- Setup WiFi ---------------------------
  
  WiFi.mode(WIFI_STA);
  WiFi.begin("SignalkWlan", "SignalkWlan!Pwd");
  Serial.print("Connecting Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.print("IP:");
  display.println(WiFi.localIP());
  display.print("GW:");
  display.println(WiFi.gatewayIP());
  display.display();

  delay(500);

  int tries = 0;
  while (!client.connect(signalkServer, signalkPort)) {
    Serial.print("Connection to: ");
    Serial.print(signalkServer);
    Serial.println(" failed");
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wifi no connected");
    } else {
      Serial.println("WiFi still connected");
    }
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Connection failed");
    display.println(signalkServer);
    display.print(" retries: ");
    display.println(tries);
    display.display();
    tries++;
    delay(1000);
  } /*else {*/
    Serial.println("TCP Connected");
    StaticJsonDocument<512> doc;

    //wait for response from the server
    while (!client.available()) {
      delay(1);
    //}

    DeserializationError err = deserializeJson(doc, client);
    if (err) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(err.c_str());
      display.clearDisplay();
      display.setCursor(0,0);
      display.print("JSON error: ");
      display.print(err.c_str());
      display.display();
    } else {
      const char* tmp = doc["name"];

      display.clearDisplay();
      display.setCursor(0,0);
      display.print("Server: ");
      display.print(tmp);
      display.display();

      tmp = doc["self"];

      client.println(subscriptionString);
    }
  }
  delay(1000);
}

void loop() {
    static float depth;
    static float velocity;
    static float tanklevel;
    static float temperature;    
    if (client.available()) {
    DynamicJsonDocument doc(2048);
    
    deserializeJson(doc, client);
    JsonObject updates_0 = doc["updates"][0];

    const char* tmp = updates_0["values"][0]["path"];
    String path = String(tmp);

#if defined (DATA_FROM_FILE)
    if (path == String("environment.depth.belowTransducer")) {
      depth = doc["updates"][0]["values"][0]["value"];    
    } else if (path == String("navigation.speedThroughWater")) {
      velocity = doc["updates"][0]["values"][0]["value"];
    }

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Depth: ");
    display.println(depth);
    display.println("Velocity: ");
    display.println(velocity);
    display.display();
#endif

#if defined (DATA_FROM_ARDUINO)
    if (path == String("tanks.rainwater.currentLevel")) {
      tanklevel = doc["updates"][0]["values"][0]["value"];    
    } else if (path == String("environment.inside.galley.temperature")) {
      temperature = doc["updates"][0]["values"][0]["value"];
    }

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Tanklevel: ");
    display.println(tanklevel);
    display.println("Temperature: ");
    display.println(temperature);
    display.display();
#endif
  } else {
    delay(100);
  }
}
