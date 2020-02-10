/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/ttgo-lora32-sx1276-arduino-ide/
*********/

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//string tokenizer library
#include <StringTokenizer.h>

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 915E6

String loraData;

float rawTemp;
float rawHumx;
long rawWht;
int counter;

void setup() { 
  
  //initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Receiver Test");
  
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(BAND)) 
  {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initializing OK!");
}

void loop() {

  //try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) 
  {
    //received a packet
    Serial.print("Received packet ");

    //read packet
    while (LoRa.available()) 
    {
      loraData = LoRa.readString();
      Serial.println(loraData);
    }

    int rssi = LoRa.packetRssi();
    Serial.print("with RSSI ");    
    Serial.println(rssi);

    tokenizeString(loraData);

    Serial.println();
    delay(2000);
  }
}

void tokenizeString(String loraString)
{
  char strBuffer[loraString.length()+1] = "";
  loraString.toCharArray(strBuffer, loraString.length()+1); // example: "\"45.3\""
  rawWht = atoi(strtok(strBuffer, "/"));
  rawTemp = atof(strtok(NULL, "/"));
  rawHumx = atof(strtok(NULL, "/"));
  counter = atoi(strtok(NULL, "/"));
  
  Serial.print("Raw Weight: ");
  Serial.println(rawWht);
  Serial.print("Raw Temp: ");
  Serial.println(rawTemp);
  Serial.print("Raw Humidity: ");
  Serial.println(rawHumx);
  Serial.print("Counter: ");
  Serial.println(counter);
}
