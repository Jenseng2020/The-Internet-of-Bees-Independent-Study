/*********
  Jensen Gaither
*********/

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//Wifi Libraries
#include <WiFi.h>
#include <WiFiClient.h>

//webhook Libraries
#include <HTTPClient.h>
#include <ArduinoJson.h>

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
const char auth[] = "rC8MTOyvwXUT8EzPTRW5ZWQyRRsuYxNB";

//Wifi Variables
const char ssid[] = "Team Donut";
const char pass[] = "EmmaandFred"; // set to "" for open networks

//webhook Variables
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q=Pikesville,us&appid=";
const String key = "086031d405d348826a118a7d6a4484f9";
String jsonPayload;
float outsideTempK;
float outsideTempC;
float outsideTempF;
int outsidePressure;
int outsideHumx;

//lora Variables
String loraData;

//si7021 variables
float hiveTempC;
float hiveTempF;
float hiveHumx;

//HX711 Variables
long rawWht;
long rawWeightLbs;
long noTareWeightLbs;
long weightLbs;
long scaleFactor;
long offset;
long tareOffset;

//cycle counter
int counter;

<<<<<<< Updated upstream
//create timers
#define loraInterval 75
#define weatherInterval 60000
BlynkTimer loraTimer;
<<<<<<< HEAD
=======
//create timer
#define weatherInterval 900000L
BlynkTimer timer;
int weatherTimer = 1;
>>>>>>> Stashed changes
=======
BlynkTimer weatherTimer;
>>>>>>> 0e1bd378aa8626521261d23d50cf04bbe5b6f91a

void getHiveData()
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

<<<<<<< HEAD
<<<<<<< Updated upstream
    tokenizeString(loraData);
=======
    tokenizeLoraString(loraData);
>>>>>>> 0e1bd378aa8626521261d23d50cf04bbe5b6f91a

    Serial.println();
    delay(2000);

    Blynk.virtualWrite(V0, rawWht);
    Blynk.virtualWrite(V1, hiveTempF);
    Blynk.virtualWrite(V2, hiveHumx);
    Blynk.virtualWrite(V3, counter);
  }
=======
    tokenizeLoraString(loraData);
    processWeight();

    Serial.println();
>>>>>>> Stashed changes
}

void tokenizeLoraString(String loraString)
{
  char strBuffer[loraString.length()+1] = "";
  loraString.toCharArray(strBuffer, loraString.length()+1); // example: "\"45.3\""
  rawWht = atoi(strtok(strBuffer, "/"));
  hiveTempC = atof(strtok(NULL, "/"));
  hiveHumx = atof(strtok(NULL, "/"));
  counter = atoi(strtok(NULL, "/"));

  hiveTempF = (hiveTempC * (9/5)) + 32;
  
  Serial.print("Raw Weight: ");
  Serial.println(rawWht);
  Serial.print("Hive Temp F: ");
  Serial.println(hiveTempF);
  Serial.print("Hive Humidity: ");
  Serial.println(hiveHumx);
  Serial.print("Counter: ");
  Serial.println(counter);
}
<<<<<<< HEAD
<<<<<<< Updated upstream
=======

void processWeight()
{
  rawWeightLbs = rawWht / scaleFactor;
  noTareWeightLbs = rawWeightLbs - offset;
  weightLbs = noTareWeightLbs - tareOffset;
}
=======
>>>>>>> 0e1bd378aa8626521261d23d50cf04bbe5b6f91a

void getWeatherData()
{
  if ((WiFi.status() == WL_CONNECTED)) 
  { 
    HTTPClient http;
    http.begin(endpoint + key); //Specify the URL
    int httpCode = http.GET();  //Make the request
    if (httpCode > 0) 
    { //Check for the returning code
        jsonPayload = http.getString();
        Serial.println(httpCode);
        Serial.println(jsonPayload);
        parseWeatherData(jsonPayload);
    }
    else 
    {
      Serial.println("Error on HTTP request");
    }
    http.end(); //Free the resources
  }
  else
  {
    Serial.println("Wifi Error");
  }
}

void parseWeatherData(String jsonString)
{
  const size_t capacity = JSON_ARRAY_SIZE(1) + 2*JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(14) + 440;
  DynamicJsonBuffer jsonBuffer(capacity);
  
  const char* json = jsonString.c_str();
  
  JsonObject& root = jsonBuffer.parseObject(json);
  
  JsonObject& main = root["main"];
  outsideTempK = main["temp"]; 
  outsideTempC = outsideTempK - 273.15;
  outsideTempF = (outsideTempC * (9/5)) + 32;
  outsidePressure = main["pressure"]; 
  outsideHumx = main["humidity"]; 
  Serial.println(outsideTempF);
  Serial.println(outsidePressure);
  Serial.println(outsideHumx);
}

<<<<<<< HEAD
void pushBlynkData()
{
    Blynk.virtualWrite(V0, counter);
    Blynk.virtualWrite(V1, rawWht);
    Blynk.virtualWrite(V2, hiveTempF);
    Blynk.virtualWrite(V3, hiveHumx);
    Blynk.virtualWrite(V4, outsideTempF);
    Blynk.virtualWrite(V5, outsideHumx);
    Blynk.virtualWrite(V6, outsidePressure); 
}

=======
>>>>>>> 0e1bd378aa8626521261d23d50cf04bbe5b6f91a
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

<<<<<<< HEAD
  getWeatherData();
  
  //Initialize Timers
  weatherTimer = timer.setInterval(weatherInterval, getWeatherData); 
=======
  //Initialize Timers
  loraTimer.setInterval(loraInterval, getHiveData);
  weatherTimer.setInterval(weatherInterval, getWeatherData); 
>>>>>>> 0e1bd378aa8626521261d23d50cf04bbe5b6f91a
}

void loop() 
{
<<<<<<< HEAD
  
  timer.run();
  Blynk.run();
  int packetSize = LoRa.parsePacket();
  if (packetSize) 
  {
    getHiveData();
    pushBlynkData();
  }
}
>>>>>>> Stashed changes
=======
  loraTimer.run();
  weatherTimer.run();
  Blynk.run();
}
>>>>>>> 0e1bd378aa8626521261d23d50cf04bbe5b6f91a
