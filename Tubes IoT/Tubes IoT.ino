#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>

// Wifi
const char* WIFI_SSID = "NAZONE";
const char* WIFI_PASSWORD = "kinanti20";

const char* SERVER_URL =
"http://10.55.228.11:5000/api/data";

// PIN
#define SDA_PIN 21
#define SCL_PIN 22

// Sensor
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;
BH1750 lightMeter;

// Status
bool ahtOK = false;
bool bmpOK = false;
bool bh1750OK = false;

// Timer
unsigned long lastRead = 0;
const unsigned long interval = 10000;

// Trend
float lastTemperature = NAN;
float lastHumidity = NAN;
float lastPressure = NAN;
float lastLux = NAN;

// 
void setup() {

  Serial.begin(115200);
  delay(2000);

  Serial.println(" SMART WEATHER STATION - ESP32");
 
  // I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(1000);

  Serial.println("[I2C] Initialized");

  // AHT20
  for (int i = 0; i < 5; i++) {

    ahtOK = aht.begin();

    if (ahtOK)
      break;

    Serial.println("[AHT20] Retry...");
    delay(1000);
  }

  if (ahtOK)
    Serial.println("[SENSOR] AHT20 OK");
  else
    Serial.println("[WARNING] AHT20 NOT DETECTED");

  // BMP280   for (int i = 0; i < 5; i++) {

    bmpOK = bmp.begin(0x77);

    if (!bmpOK)
      bmpOK = bmp.begin(0x76);

    if (bmpOK)
      break;

    Serial.println("[BMP280] Retry...");
    delay(1000);
  }

  if (bmpOK) {

    bmp.setSampling(
      Adafruit_BMP280::MODE_NORMAL,
      Adafruit_BMP280::SAMPLING_X2,
      Adafruit_BMP280::SAMPLING_X16,
      Adafruit_BMP280::FILTER_X16,
      Adafruit_BMP280::STANDBY_MS_500
    );

    Serial.println("[SENSOR] BMP280 OK");
  }
  else {
    Serial.println("[WARNING] BMP280 NOT DETECTED");
  }

  // BH1750   bh1750OK =
      lightMeter.begin(
        BH1750::CONTINUOUS_HIGH_RES_MODE
      );

  if (bh1750OK)
    Serial.println("[SENSOR] BH1750 OK");
  else
    Serial.println("[WARNING] BH1750 NOT DETECTED");

  // WIFI  
  WiFi.setSleep(false);

  connectWiFi();

    Serial.println("[SYSTEM] READY");
  }

// 
void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (millis() - lastRead >= interval) {

    lastRead = millis();

    readSensors();
  }
}

// 
void connectWiFi() {

  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.disconnect(true, true);
  delay(1000);

  Serial.print("[WiFi] Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(
      WIFI_SSID,
      WIFI_PASSWORD);

  int attempt = 0;

  while (
      WiFi.status() != WL_CONNECTED &&
      attempt < 20) {

    delay(500);

    Serial.print(".");
    Serial.print(" Status=");
    Serial.println(WiFi.status());

    attempt++;
  }

  
  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("[WiFi] Connected");
    Serial.print("[WiFi] IP : ");
    Serial.println(WiFi.localIP());

    Serial.print("[WiFi] RSSI : ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }
  else {

    Serial.println("[WiFi] Failed to connect");

    Serial.print("[WiFi] Final Status = ");
    Serial.println(WiFi.status());
  }
}

// 
void readSensors() {

  float temperature = NAN;
  float humidity = NAN;
  float pressure = NAN;
  float altitude = NAN;
  float lux = NAN;

  // AHT20   
    sensors_event_t hum;
    sensors_event_t temp;

    aht.getEvent(&hum, &temp);

    temperature =
        temp.temperature;

    humidity =
        hum.relative_humidity;
  }

  // BMP280  

    pressure =
        bmp.readPressure() / 100.0;

    altitude =
        bmp.readAltitude(1013.25);
  }

  // BH1750   

    lux =
        lightMeter.readLightLevel();
  }

  // TREND  
  float humTrend = NAN;
  float pressureTrend = NAN;
  float lightTrend = NAN;

  if (!isnan(lastTemperature))
    tempTrend =
      temperature - lastTemperature;

  if (!isnan(lastHumidity))
    humTrend =
      humidity - lastHumidity;

  if (!isnan(lastPressure))
    pressureTrend =
      pressure - lastPressure;

  if (!isnan(lastLux))
    lightTrend =
      lux - lastLux;

  // VALIDASI DATA 
    isnan(humidity) ||
    isnan(pressure) ||
    isnan(altitude) ||
    isnan(lux))
{
  Serial.println(
    "[WARNING] Invalid sensor data");

  return;
}

  // Simpan data sebelumnya
  lastTemperature = temperature;
  lastHumidity = humidity;
  lastPressure = pressure;
  lastLux = lux;

  // SERIAL 

  Serial.printf(
      "Temperature : %.2f C\n",
      temperature);

  Serial.printf(
      "Humidity    : %.2f %%\n",
      humidity);

  Serial.printf(
      "Pressure    : %.2f hPa\n",
      pressure);

  Serial.printf(
      "Altitude    : %.2f m\n",
      altitude);

  Serial.printf(
      "Light       : %.2f lux\n",
      lux);

  
  Serial.printf(
      "Temp Trend      : %.2f\n",
      tempTrend);

  Serial.printf(
      "Humidity Trend  : %.2f\n",
      humTrend);

  Serial.printf(
      "Pressure Trend  : %.2f\n",
      pressureTrend);

  Serial.printf(
      "Light Trend     : %.2f\n",
      lightTrend);

  sendToServer(
      temperature,
      humidity,
      pressure,
      altitude,
      lux,
      tempTrend,
      humTrend,
      pressureTrend,
      lightTrend);
}

// 
void sendToServer(
    float temperature,
    float humidity,
    float pressure,
    float altitude,
    float lux,
    float tempTrend,
    float humTrend,
    float pressureTrend,
    float lightTrend) {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println(
      "[HTTP] WiFi not connected");
    return;
  }

  StaticJsonDocument<512> doc;

  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["pressure"] = pressure;
  doc["altitude"] = altitude;
  doc["light_lux"] = lux;

  if (!isnan(tempTrend))
  doc["temp_trend"] = tempTrend;

  if (!isnan(humTrend))
    doc["humidity_trend"] = humTrend;

  if (!isnan(pressureTrend))
    doc["pressure_trend"] = pressureTrend;

  if (!isnan(lightTrend))
    doc["light_trend"] = lightTrend;

    doc["device_id"] =
        "ESP32_WEATHER_01";

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;

  http.begin(SERVER_URL);
  http.setTimeout(5000);

  http.addHeader(
      "Content-Type",
      "application/json");

  int httpCode =
      http.POST(payload);

  Serial.print(
      "[HTTP] Response : ");

  Serial.println(httpCode);

  if (httpCode > 0) {
    Serial.println(
      http.getString());
  }

  http.end();
}