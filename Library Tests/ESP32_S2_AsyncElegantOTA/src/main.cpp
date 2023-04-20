/*
  Rui Santos
  Complete project details
   - Arduino IDE: https://RandomNerdTutorials.com/esp32-ota-over-the-air-arduino/
   - VS Code: https://RandomNerdTutorials.com/esp32-ota-over-the-air-vs-code/
  
  This sketch shows a Basic example from the AsyncElegantOTA library: ESP32_Async_Demo
  https://github.com/ayushsharma82/AsyncElegantOTA
*/

// This code works for OTA on ESP32-S2-Feather, @ 20/04/2023
// There's an auto update feature to explore here in the GitHub Documentation: https://github.com/ayushsharma82/AsyncElegantOTA

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

const char* ssid = "Tracy Island";
const char* password = "Thunderbird4";
String hostname;

AsyncWebServer server(80);

void set_SHRIMP_Name() { // Sets device host name automatically when sent OTA update

 if      (WiFi.macAddress() == "0C:DC:7E:CA:FC:90"){hostname = "ESP32-Bubba";}    // ESP32 Feather 
 else if (WiFi.macAddress() == "60:55:F9:F5:A3:80"){hostname = "ESP32-Gump";}     // ESP32-S3 No PSRAM 
 else if (WiFi.macAddress() == "60:55:F9:F5:A4:C4"){hostname = "ESP32-FruitOTS";} // ESP32-S3 No PSRAM "Shrimp is the Fruit of the Sea" 
 else if (WiFi.macAddress() == "F4:12:FA:59:69:68"){hostname = "ESP32-Barbeque";} // ESP32-S3 2M PSRAM
 else if (WiFi.macAddress() == "84:F7:03:D7:36:42"){hostname = "ESP32-LTDan";}    // ESP32-S3 2M PSRAM
 else if (WiFi.macAddress() == "84:F7:03:D7:36:7A"){hostname = "ESP32-Boiled";}   // ESP32-S3 2M PSRAM

}

void setup(void) {
  Serial.begin(115200);

  set_SHRIMP_Name();

  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname.c_str());
  WiFi.begin(ssid, password);
  Serial.println(hostname);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I'm an ESP32, I was programmed in Platformio!");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {

}