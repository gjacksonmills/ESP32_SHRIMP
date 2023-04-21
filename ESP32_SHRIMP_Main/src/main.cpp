// SHRIMP Main
// Uses AsyncElegantOTA to upload over the air, and change a neopixel light depending on status. (but not yet actually)

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

// Adafruit Seesaw and sensor integration with ESP NOW
#include <esp_now.h>
#include <SPI.h>
#include <Adafruit_seesaw.h>

Adafruit_seesaw ss;

#define BUTTON_RIGHT 6
#define BUTTON_DOWN  7
#define BUTTON_LEFT  9
#define BUTTON_UP    10
#define BUTTON_SEL   14
uint32_t button_mask = (1 << BUTTON_RIGHT) | (1 << BUTTON_DOWN) | 
                (1 << BUTTON_LEFT) | (1 << BUTTON_UP) | (1 << BUTTON_SEL);

#define IRQ_PIN   5 // See the SeeSaw example for IRQ_Pin options for other boards
bool Seesaw; // Stores whether there is a Seesaw Joywing / Initialised or not.

// REPLACE WITH THE MAC Address of your receiver  -  we will use Gump as the controller and FruitOTS as the SHRIMP
uint8_t FruitOTS_Address[] = {0x60, 0x55, 0xF9, 0xF5, 0xA4, 0xC4}; // This is FruitOTS MAC- the SHRIMP
uint8_t Gump_Address[] =     {0x60, 0x55, 0xF9, 0xF5, 0xA3, 0x80}; // This is Gump MAC - the controller

int incoming_Serial; // Stores incoming Serial Data

// Define variables to store XYZ readings from Adafruit SeeSaw (SS) Controller
int ss_x;
int ss_y;

// Define variables to store incoming readings
int incoming_x;
int incoming_y;

float incoming_Temperature;
float incoming_Humidity;
float incoming_Pressure;
float incoming_Battery;

// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    int x;  // Seesaw analog stick x
    int y;  // Seesaw analog stick y
    bool a; // Seesaw button: A
    bool b; // Seesaw button: B
    bool c; // Seesaw button: X
    bool d; // Seesaw button: Y
    bool e; // Seesaw button: SELECT

    float temperature; // BME280 Reading
    float humidity;    // BME280 Reading
    float pressure;    // BME280 Reading

    float battery;     // LC709203F Battery Voltage
} struct_message;

// Create a struct_message called SeeSaw_Readings to hold SeeSaw Joypad readings
struct_message ss_Readings;

// Create a struct_message to hold incoming sensor readings
struct_message incoming_Readings;

esp_now_peer_info_t peerInfo;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incoming_Readings, incomingData, sizeof(incoming_Readings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incoming_Temperature = incoming_Readings.x;
  incoming_Humidity= incoming_Readings.y;
  //incoming_Pressure = incoming_Readings.pressure;
  //incoming_Battery = incoming_Readings.battery;
}


void setup() {
  Serial.begin(115200);

  set_SHRIMP_Name();

  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname.c_str());
  WiFi.begin(ssid, password);

 

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I'm an ESP32, I was programmed in Platformio!");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  
   while (!Serial){ // Stop until serial is connected
  delay(10);
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("HTTP server started, Hostname: ");
  Serial.println(hostname);

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Now setup ESP-NOW with the Seesaw controller

  // Init Adafruit SeeSaw Joy
  bool While_Seesaw = true; // Just a while loop condition for attaching the Seesaw
  do {
      if(!ss.begin(0x49)){
        Serial.println("ERROR! Seesaw Joywing Controller not found!");
        Serial.println("Check Seesaw Joywing Controller connection or type 'Y' to continue without Seesaw");
        if(Serial.available() > 0) {     // read the incoming byte:
          incoming_Serial = Serial.read(); 
        }
        if (incoming_Serial == 89){
          Serial.println("OK, starting without Seesaw!");
          Seesaw = false;
          While_Seesaw = false;
        }
      }
      else {
        Serial.println("Seesaw Joywing Controller started!");
        Serial.print("Version: ");
        Serial.println(ss.getVersion(), HEX);
        Seesaw = true;
        While_Seesaw = false;
      }
  } while (While_Seesaw); // Keep trying to initialise the Seesaw until user presses continue:  Y in ASCII = 89

  ss.pinModeBulk(button_mask, INPUT_PULLUP);
  ss.setGPIOInterrupts(button_mask, 1);

  pinMode(IRQ_PIN, INPUT); // Interrupt pin

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  if (hostname == "ESP32-Gump") { // If this is Gump
    memcpy(peerInfo.peer_addr, FruitOTS_Address, 6); // Then FruitOTS is the reciever
  } 
  else if (hostname == "ESP32-FruitOTS"){ // Else if this is FruitOTS 
    memcpy(peerInfo.peer_addr, Gump_Address, 6); // Then Gump is the reciever
  }

  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);


}

int last_x = 0;
int last_y = 0;

void Read_Seesaw(){
  ss_x = ss.analogRead(2);
  ss_y = ss.analogRead(3);
  
  if ( (abs(ss_x - last_x) > 3)  ||  (abs(ss_y - last_y) > 3)) {
    Serial.print(ss_x); Serial.print(", "); Serial.println(ss_y);
    last_x = ss_x;
    last_y = ss_y;
  }
  
  /* if(!digitalRead(IRQ_PIN)) {  // Uncomment to use IRQ (interrupts just for the buttons) */

    uint32_t buttons = ss.digitalReadBulk(button_mask);

    //Serial.println(buttons, BIN);

    if (! (buttons & (1 << BUTTON_RIGHT))) {
      Serial.println("Button A pressed");
    }
    if (! (buttons & (1 << BUTTON_DOWN))) {
      Serial.println("Button B pressed");
    }
    if (! (buttons & (1 << BUTTON_LEFT))) {
      Serial.println("Button Y pressed");
    }
    if (! (buttons & (1 << BUTTON_UP))) {
      Serial.println("Button X pressed");
    }
    if (! (buttons & (1 << BUTTON_SEL))) {
      Serial.println("Button SEL pressed");
    }
  /* } // Uncomment to use IRQ */
  //delay(10);
}



void loop() {
  
  Read_Seesaw();
 
  // Set values to send
  ss_Readings.x = ss_x;
  ss_Readings.y = ss_y;

  // Send message via ESP-NOW
  Serial.println(hostname);
  if (hostname == "ESP32-Gump") { // If this is Gump
    esp_err_t result = esp_now_send(FruitOTS_Address, (uint8_t *) &ss_Readings, sizeof(ss_Readings)); // Send a message to FruitOTS
    if (result == ESP_OK) {
      Serial.println("Sent from Gump to FruitOTS with success!");
    } else {
      Serial.println("Error sending the data to FruitOTS");
    }
  }
  else if (hostname == "ESP32-FruitOTS") { // Else if this is FruitOTS 
    esp_err_t result = esp_now_send(Gump_Address, (uint8_t *) &ss_Readings, sizeof(ss_Readings)); // Send a message to Gump
    if (result == ESP_OK) {
      Serial.println("Sent from FruitOTS to Gump with success!");
    } else {
      Serial.println("Error sending the data to Gump");
    }
  }
   
  incoming_x = incoming_Readings.x;
  incoming_y = incoming_Readings.y;

  Serial.println(incoming_x);
  Serial.println(incoming_y);

  delay(1000);
}