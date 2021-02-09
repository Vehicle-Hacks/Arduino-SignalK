/* 20191103 Ralph Grewe */
/* This is a Arduino sketch which reads in analog values and sends out SignalK packages by UDP */
/* Its based on: The Arduino UDPSendReceiveString example from Nov 3rd 2019: */ 
/* https://www.arduino.cc/en/Tutorial/UDPSendReceiveString */
/* The Arduino UdpNtpClient example from Nov. 25th 2019: */
/* https://www.arduino.cc/en/Tutorial/UdpNtpClient */
/* And a stackoverflow post about converting unix timestamp to a date: */
/* https://stackoverflow.com/questions/7136385/calculate-day-number-from-an-unix-timestamp-in-a-math-way */


// For WiFi communication
#include <ESP8266WiFi.h>
#include <StreamUtils.h>

// for the OLED Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_RESET);

#include <ArduinoJson.h>

// Configure serial output
#define SERIAL_OUTPUT
//#undef SERIAL_OUTPUT

/* Basic Ethernet variabled*/
WiFiClient client;

/* For SignalK data*/
const unsigned int signalkPort = 8375;      // SignalK Port which is the same for TCP and UDP
IPAddress signalkServer(192, 168, 177, 1);

const char subscriptionString[] = "{\"context\":\"vessels.self\",\"subscribe\":[{\"path\":\"navigation.speedThroughWater\"},{\"path\":\"environment.depth.belowTransducer\"}]}";

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(57600);
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

  WiFi.begin("MarineNetwork", "Marine!Passwort");
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

  if (!client.connect(signalkServer, signalkPort)) {
    Serial.print("Connection to: ");
    Serial.print(signalkServer);
    Serial.println(" failed");
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Connection failed");
    display.println(signalkServer);
    display.display();
  } else {
    Serial.println("TCP Connected");
    StaticJsonDocument<512> doc;

    //wait for response from the server
    while (!client.available()) {
      delay(1);
    }

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
  if (client.available()) {
    DynamicJsonDocument doc(2048);
    
    deserializeJson(doc, client);
    JsonObject updates_0 = doc["updates"][0];

    const char* tmp = updates_0["values"][0]["path"];
    String path = String(tmp);

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
  } else {
    delay(100);
  }
}
