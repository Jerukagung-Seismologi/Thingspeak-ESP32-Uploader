//=====================================================================================
//                                                                                   //
//                          ESP32 Weather Station Firmware                           //
//                                                                                   //
//                       Developed by Evan Alif Widhyatma, 2022                      //
//                        Program under Jerukagung Seismologi                        //
//                                                                                   //
//=====================================================================================
//Gantilah variabel sesuai data anda
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
//#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
//#include <BH1750.h>
#include "time.h"
#include "ThingSpeak.h"

// Timer variables
//Thingspeak
unsigned int ThingspeakPeriod = 60000; //1 Menit
unsigned int ThingspeakNext;
//TempHumiPress
unsigned int SensorPeriod = 30000; //30 Detik
unsigned int SensorNext;
//Weathercloud
unsigned long WeathercloudPeriod = 600000; //10 Menit
unsigned long WeathercloudNext;
//=====================================================================================
//setup
void setup() {
  Serial.begin(115200);
  initWiFi();
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getTime();
  bmp.begin(0x76);
  //Wire.begin();
  //lightMeter.begin();
  SensorNext      = millis() + SensorPeriod;
  ThingspeakNext  = millis() + ThingspeakPeriod;
  WeathercloudNext= millis() + WeathercloudNext;
}
//=====================================================================================
//loop
void loop() {
  if(SensorNext <= millis()) {
  getTime();
  getWeather();
  initTempHumiPress();
  //initSolarRadiation();
  //initUvRadiation();
  print_data();
  SensorNext = millis() + SensorPeriod;
  }

  if(ThingspeakNext <= millis()) {
  wunderground();
  thingspeak();
  ThingspeakNext = millis() + ThingspeakPeriod;
  }
  if(WeathercloudNext <= millis()) {
  weathercloud();
  WeathercloudNext = millis() + WeathercloudNext;
  }

  //sleep();
}

//============================================[WIFI SETUP]============================================
const char* ssid             = "XXXXXXXXXX"; //Isilah dengan Nama WiFi
const char* password         = "XXXXXXXXXX"; //Isilah dengan Password WiFi
WiFiClient client;

void initWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(5000);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");   Serial.print(WiFi.localIP());
  Serial.println(" ");
}
//===========================================[NTP CLIENT SETUP]==========================================
const long  gmtOffset_sec = 25200;    //Waktu lokal UTC dalam detik (contoh GMT+1 = +1 * 3600s = 3600s)
const int   daylightOffset_sec = 0;   //Menggunakan Day light saving time? Iya = 3600, Tidak = 0
const char* ntpServer = "time.google.com";
RTC_DATA_ATTR int lastminute, lasthour, lastday;

void getTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Gagal menyinkronkan data waktu");
    return;
  }
  Serial.print("Berhasil menyinkronkan data waktu dari server NTP. Waktu: ");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S\n");
}
//===========================================[DEEP SLEEP]==========================================
/*void sleep() {
  Serial.println("Weather station going to sleep...");
  esp_sleep_enable_timer_wakeup(600000000);
  delay(1000);
  Serial.flush();
  esp_deep_sleep_start();
  }*/
//========================================[WEATHER API]============================================
String jsonBuffer;

void getWeather() {
  //URL OpenWeather API
  String serverPath = "contoh : http://api.openweathermap.org/data/2.5/weather?q={Kota},ID&APPID={ApiKey}&units=metric&lang=id";
      jsonBuffer = httpGETRequest(serverPath.c_str());
      Serial.println(jsonBuffer);
      JSONVar jsonVar = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(jsonVar) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
    
      Serial.print("JSON object = ");
      Serial.println(jsonVar);
      double tempapi  = double(jsonVar["main"]["temp"]);
      double heatapi  = double(jsonVar["main"]["feels_like"]
      int pressapi    = int(jsonVar["main"]["pressure"]);
      int humiapi     = int(jsonVar["main"]["humidity"]);
      double windspeed= double(jsonVar["wind"]["speed"]);
      int windeg      = int(jsonVar["wind"]["deg"]);
      double windgust = double(jsonVar["wind"]["gust"]);

      Serial.print("Temperature: ");
      Serial.println(tempapi);
      Serial.print("Heat Index: ");
      Serial.println(heatapi);
      Serial.print("Pressure: ");
      Serial.println(pressapi);
      Serial.print("Humidity: ");
      Serial.println(humiapi);
      Serial.print("Wind Speed: ");
      Serial.println(windspeed);
      Serial.print("Wind Deg: ");
      Serial.println(windeg);
      Serial.print("Wind Gust: ");
      Serial.println(windgust);
    
}
String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
//========================================[TEMP,HUMI,PRESS]========================================
Adafruit_BMP280 bmp;
//Konstanta
#define c1 -8.78469475556
#define c2 1.61139411
#define c3 2.33854883889
#define c4 -0.14611605
#define c5 -0.012308094
#define c6 -0.0164248277778
#define c7 0.002211732
#define c8 0.00072546
#define c9 -0.000003582

//Data Mentah
float temperature = 0.0;
float humidity = 0.0;
float pressure = 0.0;
float dewpoint = 0.0;
float heatIndex = 0.0;

//weathercloud data
int temp;
int bar;
int hum;
int heat;
int dew;
//wunderground data
float tempf;
float baromin;
float dewptf;

void initTempHumiPress() {
  temperature = bmp.readTemperature();
  //humidity = bme.readHumidity();
  humidity = humiapi;
  pressure = bmp.readPressure() / 100.0F;
  //weathercloud TempHumiPress
  temp = int(temperature * 10);
  bar  = int(pressure * 10);
  hum  = int(humidity);
  //wunderground TempHumiPress
  tempf     = float(1.8F * temperature + 32);
  baromin   = float(pressure / 33.86F);

  //weathercloud Heat Index
  heatIndex = c1 + (c2 * temperature) + (c3 * humidity) + (c4 * temperature * humidity) + (c5 * sq(temperature)) + (c6 * sq(humidity)) + (c7 * sq(temperature) * humidity) + (c8 * temperature * sq(humidity)) + (c9 * sq(temperature) * sq(humidity));
  heat = int(heatIndex * 10);

  //weathercloud Dew Point
  double gamma = log(humidity / 100.0F) + ((17.62F * temperature) / (243.5F + temperature));
  dewpoint    = (243.5F * gamma / (17.62F - gamma));
  dew = int(dewpoint * 10);
  //wunderground Dew Point
  dewptf = float(1.8F * dewpoint + 32);
  delay(2500);
}
//=======================================[SOLAR RADIATION]==============================================
/*float lux;
float solar;
int solarrad;
float solarradiation;
//BH1750 lightMeter;

void initSolarRadiation() {
  lux = lightMeter.readLightLevel();
  solarrad = lux * 0.79;
  }*/
//==========================================[UV RADIATION AND UV INDEX]===================================
//=====================================================================================
void print_data() {
  Serial.println("Thingspeak");
  Serial.print("Suhu Lingkungan [°C]: ");             Serial.println(temperature);
  Serial.print("Kelembapan Lingkungan [Rh%]: ");      Serial.println(humidity);
  Serial.print("Tekanan Udara [hPa]: ");              Serial.println(pressure);
  Serial.print("Titik Embun [°C]: ");                 Serial.println(dewpoint);
  Serial.print("Indeks Panas [°C]: ");                Serial.println(heatIndex);
  //Serial.print("Radiasi Matahari [W/m2]: ");        Serial.println(solarrad / 10);
  //Serial.print("Indeks UV [Index]: ");              Serial.println(uv);
  //Serial.print("Presiptasi (per hari) [mm/m2]: ");  Serial.println(rain * 3);
  //Serial.print("Presiptasi (per jam) [mm/m2]: ");   Serial.println(rainrate * 3);
  //Serial.print("Laju Penguapan [mm/jam]: ");        Serial.println(et);
  //Serial.print("Suhu Ruangan [°C]: ");              Serial.println(tempin / 10);
  //Serial.print("Titik Embun Ruangan [°C]: ");       Serial.println(dewin /10);
  //Serial.print("Indeks Panas Ruangan [°C]: ");      Serial.println(heatin/10);
  //Serial.print("Kelembapan Ruangan [Rh%]: ");       Serial.println(humin);
  //Serial.print("Suhu Angin [°C]: ");                Serial.println(chill / 10);
  Serial.print("Wind speed [m/s]: ");               Serial.println(windspeed);
  Serial.print("Wind direction [°]: ");             Serial.println(windir);
  Serial.println("======================================================================");
  delay(1000);
  //***************************************************************************************
  Serial.println("Weathercloud");
  Serial.print("Suhu Lingkungan [°C]: ");             Serial.println(temp);
  Serial.print("Kelembapan Lingkungan [Rh%]: ");      Serial.println(hum);
  Serial.print("Tekanan Udara [hPa]: ");              Serial.println(bar);
  Serial.print("Titik Embun [°C]: ");                 Serial.println(dew);
  Serial.print("Indeks Panas [°C]: ");                Serial.println(heat);
  //Serial.print("Suhu Angin [°C]: ");                Serial.println(chill / 10);
  //Serial.print("Presiptasi (per hari) [mm/m2]: ");  Serial.println(rain * 3);
  //Serial.print("Presiptasi (per jam) [mm/m2]: ");   Serial.println(rainrate * 3);
  //Serial.print("Radiasi Matahari [W/m2]: ");        Serial.println(solarrad / 10);
  //Serial.print("Laju Penguapan [mm/jam]: ");        Serial.println(et);
  //Serial.print("Indeks UV [Index]: ");              Serial.println(uv);
  Serial.print("Wind speed [m/s]: ");               Serial.println(wspdhi / 10);
  Serial.print("Wind direction [°]: ");             Serial.println(wdiravg);
  Serial.println("======================================================================");
  delay(1000);
  //***************************************************************************************
  Serial.println("Wunderground");
  Serial.print("Suhu Lingkungan [°F]: ");             Serial.println(tempf);
  Serial.print("Kelembapan Lingkungan [Rh%]: ");      Serial.println(humidity);
  Serial.print("Tekanan Udara [inHg]: ");              Serial.println(baromin);
  Serial.print("Titik Embun [°F]: ");                 Serial.println(dewptf);
  //Serial.print("Radiasi Matahari [W/m2]: ");        Serial.println(solarrad);
  //Serial.print("Indeks UV [Index]: ");              Serial.println(uv);
  //Serial.print("Presiptasi (per hari) [mm/m2]: ");  Serial.println(rain * 3);
  //Serial.print("Presiptasi (per jam) [mm/m2]: ");   Serial.println(rainrate * 3);
  Serial.print("Wind speed [mph]: ");               Serial.println(wspdhi / 10);
  Serial.print("Wind direction [°]: ");             Serial.println(wdiravg);
  Serial.println("======================================================================");
}
//=====================================================================================
//Weathercloud
const char* WeathercloudID  = "XXXXXXXXXX";    //Copy and paste your Weathercloud ID here
const char* WeathercloudKEY = "XXXXXXXXXX";    //Copy and paste your Weathercloud KEY here
const char* Weathercloud    = "http://api.weathercloud.net";
//const char* streamId        = "....................";
//const char* privateKey      = "....................";

void weathercloud() {
  Serial.println("Menginisiasi data ke Weathercloud.");
  WiFiClient client;
  if (client.connect(Weathercloud, 80)) {
    Serial.print("Tersambung ke ");
    Serial.println(client.remoteIP());
  } else {
    Serial.println("Gagal tersambung ke Weathercloud.\n");
    return;
  }
  client.print("GET /set");
  client.print("/wid/");      client.print(WeathercloudID);
  client.print("/key/");      client.print(WeathercloudKEY);
  client.print("/temp/");     client.print(temp);
  client.print("/dew/");      client.print(dew);
  client.print("/heat/");     client.print(heat);
  client.print("/hum/");      client.print(hum);
  client.print("/bar/");      client.print(bar);
  //client.print("/tempin/");   client.print(tempin);
  //client.print("/chill/");    client.print(chill);
  client.print("/wspd/");     client.print(wspd);
  client.print("/wdir/");     client.print(wdir);
  //client.print("/rain/");     client.print(rainfall * 3);
  //client.print("/rainrate/"); client.print(rainrate * 3);
  //client.print("/uvi/");      client.print(uvi);
  //client.print("/solarrad/"); client.print(solarrad);

  client.println("/ HTTP/1.1");
  client.println("Host: api.weathercloud.net");
  client.println("Connection: close");
  client.println();
  Serial.println("Data berhasil terkirim ke Weathercloud.\n");
}
//=====================================================================================
//Wunderground
const char* ID      = "XXXXXX";  //Copy and paste your Weathercloud ID here
const char* KEY     = "XXXXXX"; //Copy and paste your Weathercloud KEY here
const char* server  = "rtupdate.wunderground.com";
const char* WebPage = "GET /weatherstation/updateweatherstation.php?";

void wunderground() {
  Serial.println("Menginisiasi sambungan ke Wunderground.");
  WiFiClient client;
  if (client.connect(server, 80)) {
      Serial.print("Tersambung ke ");
      Serial.println(client.remoteIP());
      delay(100);
    } else {
      Serial.println("Gagal Tersambung ke Wunderground.\n");
      return;
     }
  client.print(WebPage);
  client.print("ID=");
  client.print(ID);
  client.print("&PASSWORD=");
  client.print(KEY);
  client.print("&dateutc=now&winddir=");
  client.print(0);
  client.print("&tempf=");
  client.print(tempf);
  client.print("&humidity=");
  client.print(humidity);
  client.print("&baromin=");
  client.print(baromin);
  client.print("&dewptf=");
  client.print(dewptf);
  //client.print("&solarradiation=");
  //client.print(solarrad);
  //client.print("&uv=");
  //client.print(uv);
  client.print("&windspeedmph=");
  client.print(0); //0
  client.print("&windgustmph=");
  client.print(0);  //0
  client.print("&rainin=");
  client.print(0); //0
  client.print("&dailyrainin=");
  client.print(0); //0
  client.print("&softwaretype=ESP32&action=updateraw&realtime=1&rtfreq=30");
  client.print("/ HTTP/1.1\r\nHost: rtupdate.wunderground.com:80\r\nConnection: close\r\n\r\n");
  Serial.println("Data berhasil terkirim ke Wunderground.\n");
}
//=====================================================================================
//ThingSpeak
unsigned long myChannelNumber = 1;
const char* myWriteAPIKey = "XXXXXXX"; //Write API Key

// Timer variables
unsigned long lastTime   = 0;
unsigned long timerDelay = 60000; //MINIMAL 14 DETIK SARAN 60 DETIK

void thingspeak() {
    ThingSpeak.setField(1, temperature);
    ThingSpeak.setField(2, humidity);
    ThingSpeak.setField(3, pressure);
    ThingSpeak.setField(4, dewpoint);
    ThingSpeak.setField(5, heatIndex);
    //ThingSpeak.setField(6, solar);
    //ThingSpeak.setField(7, uvi);

    int stat = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (stat == 200) {
      Serial.println("Data Berhasil Update. Kode HTTP" + String(stat));
    }
    else {
      Serial.println("Data Gagal Update. Kode HTTP" + String(stat));
    }
}

//=====================================END OF CODE=====================================
