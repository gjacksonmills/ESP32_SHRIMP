#include "SPI.h"
#include "Adafruit_LC709203F.h"

Adafruit_LC709203F Battery_Monitor_LC7;


void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("\nAdafruit LC709203F demo");

  if (!Battery_Monitor_LC7.begin()) {
    Serial.println(F("Couldn't find Adafruit LC709203F?\nMake sure a battery is plugged in!"));
    while (1) delay(10);
  }
  Serial.println(F("Found LC709203F:"));
  Serial.print(" Version: 0x"); Serial.println(Battery_Monitor_LC7.getICversion(), HEX);
  Battery_Monitor_LC7.setThermistorB(3950);
  Serial.print(" Thermistor B = "); Serial.println(Battery_Monitor_LC7.getThermistorB());
  Battery_Monitor_LC7.setPackSize(LC709203F_APA_500MAH);
  Battery_Monitor_LC7.setAlarmVoltage(3.5);
}

void loop() {
  Serial.print("Batt_Voltage:");
  Serial.print(Battery_Monitor_LC7.cellVoltage(), 3);
  Serial.print("\t");
  Serial.print("Batt_Percent:");
  Serial.print(Battery_Monitor_LC7.cellPercent(), 1);
  Serial.print("\t");
  Serial.print("Batt_Temp:");
  Serial.println(Battery_Monitor_LC7.getCellTemperature(), 1);

  delay(2000);  // dont query too often!
}