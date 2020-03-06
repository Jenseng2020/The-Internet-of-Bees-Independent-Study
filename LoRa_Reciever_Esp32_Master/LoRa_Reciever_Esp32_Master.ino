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

//preferences library
#include <Preferences.h>

//Blynk Library
#include <BlynkSimpleEsp32.h>
#define BLYNK_PRINT Serial
#include <TimeLib.h>
#include <WidgetRTC.h>

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
const int port = 8080;

//Wifi Variables
//const char ssid[] = "Team Donut";
//const char pass[] = "EmmaandFred"; // set to "" for open networks
const char ssid[] = "Park School";
const char pass[] = "";

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

//Transmission interval
int interval = 1;

//Lock code and key
int lockCode = 5478;
int lockKey = 0;

//twitter variables
bool tweetEnable = true;
bool tweet = true;

//Blynk Terminal variables
WidgetTerminal terminal(V7);
String terminalString;

//create timer
#define weatherInterval 900000L
BlynkTimer timer;
int weatherTimer = 1;

//Initialize Preferences library object
Preferences preferences;

//Real Time Clock
WidgetRTC rtc;

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

void writePersistent()
{
  preferences.begin("persistent", false);
  preferences.putLong("scaleFactor", scaleFactor);
  preferences.putLong("offset", offset);
  preferences.putLong("tareOffset", tareOffset);
  preferences.end();
}

void readPersistent()
{
  preferences.begin("persistent", false);
  scaleFactor = preferences.getLong("scaleFactor", scaleFactor);
  offset = preferences.getLong("offset", offset);
  tareOffset = preferences.getLong("tareOffset", tareOffset);
  preferences.end();
}

void sendLoraPacket (String packet)
{
  Serial.println("In LoRa send Func");
  Serial.println("Sending packet: ");

  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();

  Serial.print("sent: ");
  Serial.println(packet);
}

void twitterFunc(int bypass)
{
  
  if (((hour() == 15) && (tweetEnable) && (tweet)) || bypass)
  {
    terminal.println("==> in twitterFunc");
    Serial.println("==> in twitterFunc");
    String strWeight = String(int(weightLbs));
    String strHumidity = String(int(hiveHumx));
    String strHiveTemp = String(int(hiveTempF));
    String strOutsideTemp = String(int(outsideTempF));
    terminal.println("==> Tweeting");
    terminal.flush();
    Serial.println("==> Tweeting");
    Serial.println(strWeight);
    Serial.println(strHumidity);
    Serial.println(strHiveTemp);
    Serial.println(strOutsideTemp);
    //Blynk.tweet("Hello World");
    Blynk.tweet("Today's Beehive report! \n Current hive weight (lbs) = " + strWeight + "\n Current hive humidity (%RH) = " + strHumidity + "\n Current hive temp (F) = " + strHiveTemp + "\n Current outside temp (F) = " + strOutsideTemp);
    tweet = false;
  }
  if (hour() != 15)
  {
    tweet = true;
  }
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

  rtc.begin();
  setSyncInterval(10 * 60);

  readPersistent();

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
  twitterFunc(0);
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
  if (String("tweet") == terminalString)
  {
    twitterFunc(1);
  }
  if (String("tweetEnable") == terminalString)
  {
    tweetEnable = true;
  }
  if (String("tweetFalse") == terminalString)
  {
    tweetEnable = false;
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
    terminal.println("==> Sets scale offset to val (lbs)");
    terminal.println(" ");
    terminal.println("3~val");
    terminal.println("==> Sets scale tareOffset to val (lbs)");
    terminal.println(" ");
    terminal.println("4~val");
    terminal.println("==> Sets transmission interval to val (sec)");
    terminal.println(" ");
    terminal.println("hive reset");
    terminal.println("==> Resets hive controller");
    terminal.println(" ");
    terminal.println("tweet");
    terminal.println("==> Forces a tweet");
    terminal.println(" ");
    terminal.println("tweetEnable/Disable");
    terminal.println("==> Enables and Disables tweeting");
    terminal.println(" ");
    terminal.println("Text or call Jensen at 410-390-1670 for more help");
    terminal.flush();
  }

  if (terminalString == String("hive reset"))
  {
    sendLoraPacket(terminalString);
    terminal.println(terminalString);
    terminal.println("==> Hive reset command sent");
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
      writePersistent();
      terminal.println("==> scaleFactor written");
      terminal.flush();
    }

    if (operation == 2)
    {
      offset = value;
      writePersistent();
      terminal.println("==> offset written");
      terminal.flush();
    }
    if (operation == 3)
    {
      tareOffset = value;
      writePersistent();
      terminal.println("==> tareOffset written");
      terminal.flush();
    }
    if (operation == 4)
    {
      interval = value;
      sendLoraPacket(terminalString);
      terminal.println(terminalString);
      terminal.println("==> interval update sent");
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

BLYNK_WRITE(V8)
{
  lockKey = param.asInt();
}

BLYNK_WRITE(V9)
{
  if (lockKey == lockCode)
  {
    tareOffset = noTareWeightLbs;
    terminal.println("==> Tare Written");
  }
  else
  {
    terminal.println("==> Incorrect lock key");
  }
}

BLYNK_WRITE(V10)
{
  if (lockKey == lockCode)
  {
    sendLoraPacket("hive reset");
    terminal.println("==> Going down for reset now!");
  }
  else
  {
    terminal.println("==> Incorrect lock key");
  }
}

BLYNK_WRITE(V11)
{
  if (lockKey == lockCode)
  {
    resetFunc();
    terminal.println("==> Reset Command Sent");
  }
  else
  {
    terminal.println("==> Incorrect lock key");
  }
}
