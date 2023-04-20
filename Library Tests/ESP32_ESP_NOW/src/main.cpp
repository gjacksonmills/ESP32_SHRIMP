/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-two-way-communication-esp32/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <esp_now.h>
#include <WiFi.h>
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

// REPLACE WITH THE MAC Address of your receiver 
uint8_t broadcastAddress[] = {0x60, 0x55, 0xF9, 0xF5, 0xA4, 0xC4};

// Define variables to store XYZ readings from Adafruit SeeSaw (SS) Controller
int ss_x;
int ss_y;

// Define variables to store incoming readings
float incomingTemp;
float incomingHum;
float incomingPres;
float battery;

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
  incomingTemp = incoming_Readings.temperature;
  incomingHum = incoming_Readings.humidity;
  incomingPres = incoming_Readings.pressure;
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  // Init Adafruit SeeSaw Joy 
  if(!ss.begin(0x49)){
    Serial.println("ERROR! seesaw not found");
    while(1) delay(1);
  } else {
    Serial.println("seesaw started");
    Serial.print("version: ");
    Serial.println(ss.getVersion(), HEX);
  }
  ss.pinModeBulk(button_mask, INPUT_PULLUP);
  ss.setGPIOInterrupts(button_mask, 1);

  pinMode(IRQ_PIN, INPUT); // Interrupt pin
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
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
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &ss_Readings, sizeof(ss_Readings));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }

}