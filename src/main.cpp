// Import required libraries
#include "WiFi.h"
#include "esp_wifi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "time.h"
#include "dht.h"
#include "ArduinoJson.h"
#include "DiOremote.h"

#define DHT11_PIN 4

#define SLEEP_TIME  2000L
#define WAKE_TIME   20000
#define CLIENT_TIME 20000

#define BUT_1_ON  101061255255255100LL
#define BUT_1_OFF 101061255255255000LL
#define BUT_2_ON  101071255255255100LL
#define BUT_2_OFF 101071255255255000LL
#define TX_PIN  2

// Temperature data
float avgLampTemp;
float actual_temp;
DHT dht(DHT11_PIN, DHT11);
RTC_DATA_ATTR int activeCount = 0;

// DiOremote
DiOremote myRempte = DiOremote(TX_PIN);

// JSON
struct Config{
  byte rising_hour = 8;
  byte sunset_hour = 19;
  byte rising_min = 0;
  byte sunset_min = 0;
};

const char *filename = "/config.txt";
Config config;

// Replace with your network credentials
const char* ssid = "WiFi-2.4-B4E5";
const char* password = "*********";

IPAddress local_ip(192,168,1,18);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 255);
bool recentConnection = false;

//Real time setting
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
struct tm timeInfo;
char timeHour[3];
char timeMinute[3];
uint64_t sleep_time = 5000L;
bool flag_sleep = false;

// Input param html
const char* PARAM_RISING = "rising";
const char* PARAM_SUNSET = "sunset";

// Set LED GPIO
const int ledPin = 26;
const int thermistancePin = 34;
const int fanPin = 25;
// Stores LED state
String ledState;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Replaces placeholder with LED state value
String processor(const String& var){
  if(var == "STATE"){
    if(digitalRead(ledPin)){
      ledState = "ON";
    }
    else{
      ledState = "OFF";
    }
    return ledState;
  }
  if(var == "TEMP"){
    return String(actual_temp); 
  }
  if(var == "LAMPTEMP"){
    return String(avgLampTemp);
  }
  if(var == "RISING_HOUR"){
    return String(config.rising_hour);
  }
  if(var == "SUNSET_HOUR"){
    return String(config.sunset_hour);
  }
  if(var == "RISING_MINUTE"){
    return String(config.rising_min);
  }
  if(var == "SUNSET_MINUTE"){
    return String(config.sunset_min);
  }
  return String();
}

void readTemp(){
  int Vo;
  float R1 = 10000;
  float logR2, R2, T, Tc, sum;
  float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
  for(int i = 0; i<500; i++){
    Vo = analogRead(thermistancePin);
    R2 = R1 * (4095.0 / (float)Vo - 1.0);
    logR2 = log(R2);
    T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
    Tc = T - 273.15;
    sum = sum + Tc;
  }
  avgLampTemp = sum/500;
  actual_temp = dht.readTemperature();
  Serial.print("Temperature: ");
  Serial.print(avgLampTemp);
  Serial.println(" C"); 
}

int timeStringToInt(String timeToInt, bool isHour){
  byte stop = 0;
  if(isHour){
    stop = 1;
  }
  else stop = 4;
  char* v[2];
  int timeInt;
  for (byte i = stop-1; i<stop; i++){
    v[i] = &timeToInt[i];
    timeInt += atoi(v[i]);
  }
  return timeInt;
}

void loadConfiguration(const char *filename, Config &config) {
  // Open file for reading
  File file = SPIFFS.open(filename);
  if (file.available())
  {
    Serial.println("File open");
  }
  

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  // Copy values from the JsonDocument to the Config
  config.rising_hour = doc["rising_hour"];
  config.rising_min = doc["rising_min"];
  config.sunset_hour = doc["sunset_hour"];
  config.sunset_min = doc["sunset_min"];
  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
}

void saveConfiguration(const char *filename, const Config &config) {
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(filename);

  // Open file for writing
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  // Set the values in the document
  doc["rising_hour"] = config.rising_hour; 
  doc["rising_min"] = config.rising_min;
  doc["sunset_hour"] = config.sunset_hour;
  doc["sunset_min"] = config.sunset_min;

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  Serial.println("Config save");
  file.close();
}

void AsynchronServerSetup(){
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    recentConnection = true;
    readTemp();
    Serial.println("New Client");
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", String());
  });

  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/setting.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/setting.html", String(), false, processor);
  });

  // Route to set GPIO to HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, HIGH);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, LOW);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/sleep", HTTP_GET, [](AsyncWebServerRequest *request){ 
    flag_sleep = true;
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/download.html", HTTP_GET, [](AsyncWebServerRequest * request){
    request->send(SPIFFS, "/data.txt", String(), true);
    Serial.print("download");
  });
  // Getting input form sereur
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // GET input1 value on <ESP_IP>/get?rising=<inputMessage>
    if (request->hasParam(PARAM_RISING)) {
      String timeRising = request->getParam(PARAM_RISING)->value();
      config.rising_hour = timeStringToInt(timeRising, true);
      config.rising_min = timeStringToInt(timeRising, false);
    }
    if (request->hasParam(PARAM_SUNSET)) {
      String timeSunset = request->getParam(PARAM_SUNSET)->value();
      config.sunset_hour = timeStringToInt(timeSunset, true);
      config.sunset_min = timeStringToInt(timeSunset, false);
      request->send(SPIFFS, "/index.html", String(), false, processor);
      saveConfiguration(filename, config);
    }
    if(request->hasParam("slider")){

    }
    request->send(SPIFFS, "/index.html", String(), false, processor);
    saveConfiguration(filename, config);
    });
  // Start server
  loadConfiguration(filename, config);
  server.begin();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.print("Enter Setup");
  pinMode(ledPin, OUTPUT);

  // Connect to Wi-Fi
  //WiFi.config(local_ip, gateway,subnet);
  WiFi.begin(ssid, password);
  Serial.print("Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.println("Connecting to WiFi..");
  }

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Config time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  AsynchronServerSetup();
}

void getTime(){
  getLocalTime(&timeInfo);
  strftime(timeHour, 3, "%H", &timeInfo);
  strftime(timeMinute, 3, "%M", &timeInfo);
}
 
bool compareTime(byte fixRisingHour, byte fixSunsetHour, char* cHour, byte fixRisingMin, byte fixSunsetMin, char* cMin){
  byte HourInt;
  byte MinInt;
  for (byte i = 0; i < 1; i++){
    HourInt += atoi(cHour);
    MinInt += atoi(cMin);
  }
  if(fixRisingHour <= HourInt && fixSunsetHour >= HourInt){
    if(fixRisingHour == HourInt){
      if(MinInt <= fixRisingMin){
        return 0;
      }
    }
    if(fixSunsetHour == HourInt){
      if(MinInt >= fixSunsetMin){
        return 0;
      }
    }
    return 1;
  }
  return 0;
}

void sleep_option(){ 
  if(!recentConnection){
    server.begin();
  }
    if(recentConnection){
    activeCount = CLIENT_TIME;
    recentConnection = false;
    Serial.println("New Connection");
  }
  if(activeCount > 0){
    activeCount -= 1;
    Serial.println(activeCount);
  }
  if(activeCount <= 0){
    activeCount = WAKE_TIME;
    flag_sleep = true;
  }
  
  if(flag_sleep){
    flag_sleep = false;
    Serial.println("Going to sleep");
    Serial.flush();
    server.end();
    esp_sleep_enable_timer_wakeup(sleep_time);
    //esp_light_sleep_start();
  }
}

void loop(){
  getTime();
  readTemp();
  if(compareTime(config.rising_hour, config.sunset_hour, timeHour, config.rising_min, config.sunset_min, timeMinute)){
    digitalWrite(ledPin, HIGH);
  }
  else digitalWrite(ledPin, LOW);

  if(flag_sleep){
    flag_sleep = false;
    esp_sleep_enable_timer_wakeup(sleep_time);
    //esp_deep_sleep_start();
  }
}