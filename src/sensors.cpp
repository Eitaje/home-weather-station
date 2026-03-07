#include "sensors.h"
#include "config.h"
#include <ESP8266WiFi.h>

#include <Adafruit_AHTX0.h>   // For AHT21
#include <ScioSense_ENS160.h> // For ENS160
#include <Wire.h>
#include <BH1750.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EasyNTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include <Arduino_JSON.h>

// ── Hardware objects ──────────────────────────────────────────────────────────
ScioSense_ENS160 ens160(ENS160_I2CADDR_1); 
Adafruit_AHTX0    aht; 
BH1750            lightMeter;
OneWire           oneWire(ONE_WIRE_BUS);
DallasTemperature waterSensors(&oneWire);
WiFiUDP           udp;
EasyNTPClient     ntpClient(udp, "pool.ntp.org", (2 * 60 * 60)); // GMT+2

// ── Circular buffers ──────────────────────────────────────────────────────────
CircularBuffer<float, 50>         temperature_buffer;
CircularBuffer<uint8_t, 50>       AQI_buffer;
CircularBuffer<uint16_t, 50>      VOC_buffer;
CircularBuffer<uint16_t, 50>      CO2_buffer;
CircularBuffer<float, 50>         humidity_buffer;
CircularBuffer<float, 50>         water_temp_buffer;
CircularBuffer<float, 50>         lightIntensity_buffer;
CircularBuffer<unsigned long, 50> current_time_buffer;

// ── Current readings ──────────────────────────────────────────────────────────
float temperature     = -100.0;
float humidity   = -100.0;
float hic             = -100.0;
float water_temperature = -100.0;
float light_intensity   = -100.0;
int CO2              = -100;

uint8_t aqi = 0;
uint16_t tvoc = 0;
uint16_t eco2 = 0;

static JSONVar readings;

// ── Init ──────────────────────────────────────────────────────────────────────
void initSensors() {
  
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("Could not find BH1750!");
  }
  
  if (!aht.begin()) {
    Serial.println("Could not find AHT21!");
  }

  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("Could not find BH1750!");
  }

  if (!ens160.begin()) {
    Serial.println("Could not find ENS160!");
  } else {
    ens160.setMode(ENS160_OPMODE_STD);
  }

  waterSensors.begin();
}

// ── JSON snapshot ─────────────────────────────────────────────────────────────
String getSensorReadings() {
  readings["water_temperature"] = String(water_temperature);
  readings["humidity"]          = String(humidity);
  readings["light"]             = String(light_intensity);
  readings["temperature"]       = String(temperature);
  readings["AQI"]               = String(AQI_buffer.last());
  readings["VOC"]               = String(VOC_buffer.last());
  readings["CO2"]               = String(CO2_buffer.last());
  // readings["HIC"]               = String(hic_buffer.last());
  //readings["pressure"]          = String(bmp388_pressure_buffer.last());
  return JSON.stringify(readings);
}

// ── Sensor callbacks ──────────────────────────────────────────────────────────
void sample_sensor_ENS160_callback() {
  // Single read per task execution — the task interval is already ≥ 2 s (DHT22 minimum).
  // Do NOT retry with delay() here: blocking > ~3 s starves the ESP8266 WiFi stack
  // and triggers the hardware watchdog. A failed read is recovered on the next cycle.
  sensors_event_t hum, temp;
  aht.getEvent(&hum, &temp);
  
  ens160.measure(true);
  aqi  = ens160.getAQI();
  tvoc = ens160.getTVOC();
  eco2 = ens160.geteCO2();

  float t = temp.temperature;
  float h = hum.relative_humidity;

  temperature = t;
  humidity = h;

  temperature_buffer.push(t);
  humidity_buffer.push(h);
  AQI_buffer.push(aqi);
  VOC_buffer.push(tvoc);
  CO2_buffer.push(eco2);

  if (DEBUG) {
    Serial.print("Ext. temperature: "); Serial.println(String(t));
    Serial.print("Humidity: ");         Serial.println(String(h));
    Serial.print("AQI: ");              Serial.println(String(aqi));
    Serial.print("VOC: ");              Serial.println(String(tvoc));
    Serial.print("CO2: ");              Serial.println(String(eco2));
    // Serial.print("Heat index: ");       Serial.println(String(hic));
  }
}

void sample_sensor_water_temp_callback() {
  waterSensors.requestTemperatures();
  float t = waterSensors.getTempCByIndex(0);
  if (isnan(t)) t = -1000;

  water_temperature = t;
  water_temp_buffer.push(water_temperature);

  if (DEBUG)
    Serial.print("Water temperature: ");
  Serial.println(String(water_temperature));
}

void sample_sensor_BH1750_callback() {
  float lux = lightMeter.readLightLevel();
  light_intensity = lux;
  lightIntensity_buffer.push(lux);
  Serial.print("Light Meter: ");
  Serial.print(lux);
  Serial.println(" lx");
}

/*
void sample_sensor_BMP388_callback() {
  if (!bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }

  bmp388_temp_buffer.push(bmp.temperature);
  bmp388_pressure_buffer.push(bmp.pressure / 100.0);

  if (DEBUG) {
    Serial.print("Temperature = ");
    Serial.print(bmp.temperature);
    Serial.println(" *C");

    Serial.print("Pressure = ");
    Serial.print(bmp.pressure / 100.0);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bmp.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");
  }
}
*/

void update_date_time_callback() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("NTP skipped: WiFi not connected");
    return;
  }

  unsigned long t = ntpClient.getUnixTime();
  if (t == 0) {
    Serial.println("NTP returned 0 — skipping");
    return;
  }

  current_time_buffer.push(t);

  if (DEBUG) {
    char buffer[40];
    sprintf(buffer, "%lu", t);
    Serial.print("time: ");
    Serial.println(buffer);
  }
}
