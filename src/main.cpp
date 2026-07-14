#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== I2C SCANNER ===");
  
  Wire.begin(4, 5);
  Wire.setClock(100000);
  
  byte error, address;
  int nDevices = 0;
  
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

void loop() { delay(10000); }
