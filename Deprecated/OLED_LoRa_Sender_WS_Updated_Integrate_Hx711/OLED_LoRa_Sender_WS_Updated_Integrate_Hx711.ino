/********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/ttgo-lora32-sx1276-arduino-ide/
*********/

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Libraries for sensors
#include <HX711.h>
#include <Adafruit_Si7021.h>

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//Define LoRa Band
//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 915E6

//Define HX711 Pins
#define HX711_DOUT 12 
#define HX711_SCK 13

//packet counter
int counter = 0;

//Raw scale value
long scaleRaw;

//Initialize HX711
HX711 scale;

//Initialize si7021
Adafruit_Si7021 si7021 = Adafruit_Si7021();

void setup() 
{
  //initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println("LoRa Hx711/si7021 Sender Test");

  //begin Lora
  SPI.begin(SCK, MISO, MOSI, SS); //SPI LoRa pins
  LoRa.setPins(SS, RST, DIO0); //setup LoRa transceiver module
  if (!LoRa.begin(BAND)) //check if lora started 
  {
    Serial.println("Starting LoRa failed!");
  }
  else
  {
  Serial.println("LoRa Initializing OK!");//announce lora started ok
  }
  delay(1000);

  //begin hX711
  scale.begin(HX711_DOUT, HX711_SCK);
  if (!scale.is_ready())//check if hx711 started 
  {
    Serial.println("Starting Hx711 failed!");
  }
  else
  {
  Serial.println("HX711 Initializing OK!");
  }
  delay(1000);

  //begin si7021
  if (!si7021.begin())//check if si7021 started
  {
    Serial.println("Starting Si7021 failed!");
  }
  else
  {
  Serial.println("si7021 Initializing OK!");
  }
  delay(1000);
}

void loop() 
{
  Serial.print("Counter = ");
  Serial.println(counter);
  sendLoraPacket(getRawWht(),getTemp(),getHumx());
  counter++;
  delay(2000);
}

float getTemp()
{
  Serial.println("In temp func");
  float temp = si7021.readTemperature();
  return temp;
}

float getHumx()
{
  Serial.println("In humx func");
  float humx = si7021.readHumidity();
  return humx;
}

long getRawWht()
{
  Serial.println("In raw weight func");
  long scaleRaw = scale.read();
  return scaleRaw;
}

void sendLoraPacket (long rawWht, float temp, float humx)
{
  Serial.println("In LoRa Func");
  Serial.print("Sending packet: ");
  LoRa.beginPacket();
  LoRa.print(rawWht);
  LoRa.print(temp);
  LoRa.print(humx);
  LoRa.endPacket();
  Serial.println("sent");
  
}
