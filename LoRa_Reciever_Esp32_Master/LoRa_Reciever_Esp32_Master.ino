/*********
  Jensen Gaither - Internet Of Bees
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

//eeprom library
#include <EEPROM.h>

//Blynk Library
#include <BlynkSimpleEsp32.h>
#define BLYNK_PRINT Serial

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
const char server[] = "45.55.96.146";
const int port = 443;

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
long scaleFactor = 1;
long offset = 20;
long tareOffset = 0;

//cycle counter
int counter;

//Blynk Terminal variables
WidgetTerminal terminal(V7);
String terminalString;

//create timer
#define weatherInterval 900000L
BlynkTimer timer;
int weatherTimer = 1;

/*****************************************************/
/**************** Hive Data Functions ****************/
/*****************************************************/

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

  tokenizeLoraString(loraData);
  processWeight();

  Serial.println();
}

void tokenizeLoraString(String loraString)
{
  char strBuffer[loraString.length() + 1] = "";
  loraString.toCharArray(strBuffer, loraString.length() + 1); // example: "\"45.3\""
  rawWht = atoi(strtok(strBuffer, "/"));
  hiveTempC = atof(strtok(NULL, "/"));
  hiveHumx = atof(strtok(NULL, "/"));
  counter = atoi(strtok(NULL, "/"));

  hiveTempF = (hiveTempC * (9 / 5)) + 32;

  Serial.print("Raw Weight: ");
  Serial.println(rawWht);
  Serial.print("Hive Temp F: ");
  Serial.println(hiveTempF);
  Serial.print("Hive Humidity: ");
  Serial.println(hiveHumx);
  Serial.print("Counter: ");
  Serial.println(counter);
}

void processWeight()
{
  rawWeightLbs = rawWht / scaleFactor;
  noTareWeightLbs = rawWeightLbs - offset;
  weightLbs = noTareWeightLbs - tareOffset;
}

/***********************************************************/
/**************** Weather Webhook Functions ****************/
/***********************************************************/

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
  const size_t capacity = JSON_ARRAY_SIZE(1) + 2 * JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(14) + 440;
  DynamicJsonBuffer jsonBuffer(capacity);

  const char* json = jsonString.c_str();

  JsonObject& root = jsonBuffer.parseObject(json);

  JsonObject& main = root["main"];
  outsideTempK = main["temp"];
  outsideTempC = outsideTempK - 273.15;
  outsideTempF = (outsideTempC * (9 / 5)) + 32;
  outsidePressure = main["pressure"];
  outsideHumx = main["humidity"];
  Serial.println(outsideTempF);
  Serial.println(outsidePressure);
  Serial.println(outsideHumx);
}

/***********************************************************/
/**************** Blynk Data Push Functions ****************/
/***********************************************************/

void pushBlynkData()
{
  Blynk.virtualWrite(V0, counter);
  Blynk.virtualWrite(V1, weightLbs);
  Blynk.virtualWrite(V2, hiveTempF);
  Blynk.virtualWrite(V3, hiveHumx);
  Blynk.virtualWrite(V4, outsideTempF);
  Blynk.virtualWrite(V5, outsideHumx);
  Blynk.virtualWrite(V6, outsidePressure);
}

/****************************************************************/
/**************** Miscellaneous System Functions ****************/
/****************************************************************/

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void writeEEPROM()
{
  int eeAddress = 0;
  EEPROM.put(eeAddress, scaleFactor);
  eeAddress += sizeof(scaleFactor);
  EEPROM.put(eeAddress, offset);
  eeAddress += sizeof(offset);
  EEPROM.put(eeAddress, tareOffset);
  eeAddress += sizeof(tareOffset);
}

void readEEPROM()
{
  int eeAddress = 0;
  EEPROM.get(eeAddress, scaleFactor);
  eeAddress += sizeof(scaleFactor);
  EEPROM.get(eeAddress, offset);
  eeAddress += sizeof(offset);
  EEPROM.get(eeAddress, tareOffset);
  eeAddress += sizeof(tareOffset);
}



/************************************************/
/**************** Setup Function ****************/
/************************************************/

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


  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");

  Blynk.config(auth, "blynk-cloud.com", 8080);

  if (!Blynk.connect())
  {
    Serial.println("Starting Blynk failed!");
    while (1);
  }
  Serial.println("Blynk Initializing OK!");


  getWeatherData();

  //Initialize Timers
  weatherTimer = timer.setInterval(weatherInterval, getWeatherData);
}

/***********************************************/
/**************** Loop Function ****************/
/***********************************************/

void loop()
{

  timer.run();
  Blynk.run();
  int packetSize = LoRa.parsePacket();
  if (packetSize)
  {
    if (!Blynk.connected()) {
      Serial.println("Blynk not connected");
    }
    getHiveData();
    pushBlynkData();
  }
}

/**********************************************************/
/**************** Blynk Terminal Functions ****************/
/**********************************************************/

BLYNK_WRITE(V7)
{
  terminalString = param.asStr();
  if (String("reset") == terminalString)
  {
    terminal.println(" ");
    terminal.println("Going down for reboot now!");
    terminal.flush();
    resetFunc();
  }
  if (String("debug hx711") == terminalString)
  {
    debugLoop(1);
  }
  if (String("debug si7020") == terminalString)
  {
    debugLoop(2);
  }
  if (String("debug weather") == terminalString)
  {
    debugLoop(3);
  }
  if (String("debug wifi") == terminalString)
  {
    debugLoop(4);
  }
  if (String("clear") == terminalString)
  {
    clearTerminal();
  }
  if (String("help") == terminalString || String("Help") == terminalString || String("HELP") == terminalString)
  {
    clearTerminal();
    terminal.println("Help page 1 of 1");
    terminal.println("COMMANDS:");
    terminal.println(" ");
    terminal.println("reset");
    terminal.println("==> Resets device");
    terminal.println(" ");
    terminal.println("read");
    terminal.println("==> Displays current hive info");
    terminal.println(" ");
    terminal.println("debug hx711");
    terminal.println("==> Displays HX711 related debug info");
    terminal.println(" ");
    terminal.println("debug si7020");
    terminal.println("==> Displays si7020 related debug info");
    terminal.println(" ");
    terminal.println("debug weather");
    terminal.println("==> Displays weather related debug info");
    terminal.println(" ");
    terminal.println("1~val");
    terminal.println("==> Sets scale factor to val");
    terminal.println(" ");
    terminal.println("2~val");
    terminal.println("==> Sets scale offset to val");
    terminal.println(" ");
    terminal.println("3~val");
    terminal.println("==> Sets scale tareOffset to val");
    terminal.println(" ");
    terminal.println("Text or call Jensen at 410-390-1670 for more help");
    terminal.flush();
  }
  if (terminalString.indexOf('~') != -1)
  {
    terminal.println(" ");
    terminal.println("In set function");
    terminal.flush();
    int stringLength = terminalString.length() + 1;
    char strBuffer[stringLength] = "";
    terminalString.toCharArray(strBuffer, stringLength);
    int operation = atoi(strtok(strBuffer, "\"~"));
    float value = atof(strtok(NULL, "~"));

    if (operation == 1)
    {
      scaleFactor = value;
      writeEEPROM();
      terminal.println("==> scaleFactor written");
      terminal.flush();
    }

    if (operation == 2)
    {
      offset = value;
      writeEEPROM();
      terminal.println("==> offset written");
      terminal.flush();
    }
    if (operation == 3)
    {
      tareOffset = value;
      writeEEPROM();
      terminal.println("==> tareOffset written");
      terminal.flush();
    }
  }
}

void debugLoop(int x)
{
  if (x == 1)
  {
    clearTerminal();
    terminal.println("Current states of HX711 related variables:");
    terminal.print("rawWeight = ");
    terminal.println(rawWht);
    terminal.print("rawWeightLbs = ");
    terminal.println(rawWeightLbs);
    terminal.print("noTareWeightLbs = ");
    terminal.println(noTareWeightLbs);
    terminal.print("weight = ");
    terminal.println(weightLbs);
    terminal.print("scaleFactor = ");
    terminal.println(scaleFactor);
    terminal.print("offset = ");
    terminal.println(offset);
    terminal.print("tareOffset = ");
    terminal.println(tareOffset);
    terminal.flush();
  }

  if (x == 2)
  {
    clearTerminal();
    terminal.println("Current states of si7020 related variables:");
    terminal.print("hiveTempC = ");
    terminal.println(hiveTempC);
    terminal.print("hiveTempF = ");
    terminal.println(hiveTempF);
    terminal.print("hiveHumx = ");
    terminal.println(hiveHumx);
    terminal.flush();
  }

  if (x == 3)
  {
    clearTerminal();
    terminal.println("Current states of weather related variables:");
    terminal.print("outsideTempK = ");
    terminal.println(outsideTempK);
    terminal.print("outsideTempC = ");
    terminal.println(outsideTempC);
    terminal.print("outsideTempF = ");
    terminal.println(outsideTempF);
    terminal.print("outsideHumx = ");
    terminal.println(outsideHumx);
    terminal.print("outsidePressure = ");
    terminal.println(outsidePressure);
    terminal.flush();
  }
  if (x == 4)
  {
    clearTerminal();
    terminal.println("Current states of Wifi related variables:");
    terminal.print("SSID of network = ");
    terminal.println(WiFi.SSID());
    terminal.print("RSSI of network = ");
    terminal.println(WiFi.RSSI());
    terminal.flush();
  }
}

void clearTerminal()
{
  terminal.println(" ");
  terminal.println(" ");
  terminal.println(" ");
  terminal.println(" ");
  terminal.println(" ");
  terminal.println(" ");
  terminal.println(" ");
  terminal.println(" ");
  terminal.println(" ");
  terminal.println(" ");
  terminal.println(" ");
  terminal.println(" ");
  terminal.flush();
}

/*******************************************************/
/**************** BLYNK_WRITE Functions ****************/
/*******************************************************/
