/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/ttgo-lora32-sx1276-arduino-ide/
*********/

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//Wifi Libraries
#include <WiFi.h>
#include <WiFiClient.h>

//Blynk Library
#include <BlynkSimpleEsp32.h>

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//Lora Band
//915E6 for North America
#define BAND 915E6

//blynk Variables
char auth[] = "rC8MTOyvwXUT8EzPTRW5ZWQyRRsuYxNB";

//Wifi Variables
char ssid[] = "Team Donut";
char pass[] = "EmmaandFred"; // set to "" for open networks

//lora Variables
String loraData;

//si7021 variables
float rawTemp;
float rawHumx;

//HX711 Variables
long rawWht;

int counter;

//create timers
BlynkTimer loraTimer;

void setup() 
{ 
  //initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Receiver Push to Blynk Test");
  
  //Setup lora pins
  SPI.begin(SCK, MISO, MOSI, SS);//SPI LoRa pins
  LoRa.setPins(SS, RST, DIO0);//setup LoRa transceiver module

  //Initialize Lora
  if (!LoRa.begin(BAND)) 
  {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initializing OK!");

  //Initialize Blynk
  Blynk.begin(auth, ssid, pass);
  if (!Blynk.connected()) 
  {
    Serial.println("Starting Blynk failed!");
    while (1);
  }
  Serial.println("Blynk Initializing OK!");

  //Initialize Timers
  loraTimer.setInterval(75, getData); 
}

void loop() 
{
  loraTimer.run();
  Blynk.run();
}

void getData()
{
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

    Blynk.virtualWrite(V0, rawWht);
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
