#include "HX711.h"

// HX711 circuit wiring
const int HX711_DOUT_PIN = 12;
const int HX711_SCK_PIN = 13;

HX711 scale;

void setup() {
  Serial.begin(57600);
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
}

void loop() {

  if (scale.is_ready()) {
    long reading = scale.read();
    Serial.print("HX711 reading: ");
    Serial.println(reading);
  } else {
    Serial.println("HX711 not found.");
  }

  delay(1000);
  
}
