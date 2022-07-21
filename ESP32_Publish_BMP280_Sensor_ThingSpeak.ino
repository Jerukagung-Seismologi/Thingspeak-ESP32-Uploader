//#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>
//#include <AsyncElegantOTA.h>
//#include "Adafruit_HTU21DF.h"
//#include "Adafruit_SHT31.h"
//#include <ESPmDNS.h>
//#include <WiFiUdp.h>
//#include <ArduinoOTA.h>
#include "ThingSpeak.h"

const char* ssid      = "Jerukagung Seismologi";   // your network SSID (name)
const char* password  = "seiscalogi";   // your network password

WiFiClient client;
//AsyncWebServer server(80);

unsigned int myChannelNumber = 1;
const char* myWriteAPIKey = "S1VN6AN4R1QT4QMD";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 59000; //MINIMAL 14 DETIK
const int ledStat    = 2;
//const int ledWifi    = 4;
//const int ledGood    = 16;
//const int ledFail    = 17;

// Variable to hold BME280 readings
float temperatureBmp;
//float humidityBme;
float pressureBmp;
/* Variable to hold HTU21 readings
  float temperatureHtu;
  float humidityHtu;
  // Variable to hold SHT31 readings
  float temperatureSht;
  float humiditySht;*/

// Create a sensor object
Adafruit_BMP280 bmp;
//Adafruit_BME280 bme;
//Adafruit_HTU21DF htu = Adafruit_HTU21DF();
//Adafruit_SHT31 sht31 = Adafruit_SHT31();

void initBMP() {
  if (!bmp.begin(0x76))
  {
    Serial.println("Gagal Menemukan BMP280, Mohon Cek Sambungan Kabel I2C");
    while (1);
  }
  /*bmp.setSampling(Adafruit_BME280::MODE_FORCED, 
                    Adafruit_BME280::SAMPLING_X1,     
                    Adafruit_BME280::SAMPLING_X1,   
                    Adafruit_BME280::FILTER_OFF);*/  
}
/*void OtaUpdate() {
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}*/
/*void OtaServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OTA Updater.");
  });
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server Diaktifkan");
}*/
/*void initHtu(){
  if (!htu.begin())
  {
    Serial.println("Gagal Menemukan HTU21, Mohon Cek Sambungan Kabel I2C");
    while (1);
  }
  }
  void initSht(){
   if (! sht31.begin(0x44)) { // Set to 0x45 for alternate i2c addr
    Serial.println("Gagal menemukan SHT31, Mohon Cek Sambungan Kabel I2C");
    while (1);
  }
  Serial.print("Heater Enabled State: ");
  if (sht31.isHeaterEnabled())
    Serial.println("ENABLED");
  else
    Serial.println("DISABLED");
  }*/
void setup() {
  Serial.begin(115200);  //Initialize serial
  initBMP();
  //OtaServer();
  //OtaUpdate();
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  pinMode(ledStat,OUTPUT);
  //pinMode(ledWifi,OUTPUT);
  //pinMode(ledGood,OUTPUT);
  //pinMode(ledFail,OUTPUT);
}

void loop() {
  //ArduinoOTA.handle();
  if ((millis() - lastTime) > timerDelay) {
    // Connect or reconnect to WiFi
    // Comment all Serial.print if use led indicator
    if (WiFi.status() != WL_CONNECTED) {
      //digitalWrite(ledWifi, LOW);
      Serial.print("Melakukan Penyambungan Internet...");
      while (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, password);
        delay(5000);
      }
      //digitalWrite(ledWifi, HIGH);
      Serial.println("\nTersambung.");
    }
    // Get a new Total reading
    digitalWrite(ledStat, HIGH);
    float temperatureTot = bmp.readTemperature();
    Serial.print("Temperature (ºC): ");
    Serial.println(temperatureTot);
    /*humidityTot = htu.readHumidity();
      Serial.print("Humidity (%): ");
      Serial.println(humidityTot);*/
    float pressureTot = bmp.readPressure() / 100.0F;
    Serial.print("Pressure (hPa): ");
    Serial.println(pressureTot);

    // set the fields with the values
    ThingSpeak.setField(1, temperatureTot);
    //ThingSpeak.setField(2, humidityTot);
    ThingSpeak.setField(3, pressureTot);

    //HTTP Kode untuk mengecek berhasil atau gagal
    int http = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
               ThingSpeak.setStatus(String(http));

    if (http == 200) {
      //digitalWrite(ledGood, HIGH);
      //digitalWrite(ledFail, LOW);
      Serial.println("Data Berhasil Update. Kode HTTP" + String(http));
    }
    else {
      //digitalWrite(ledFail, HIGH);
      //digitalWrite(ledGood, LOW);
      Serial.println("Data Gagal Update. Kode HTTP" + String(http));
    }
    lastTime = millis();
    digitalWrite(ledStat, LOW);
  }
}
