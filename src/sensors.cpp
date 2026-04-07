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
#include "Adafruit_BMP5xx.h"
#include <Arduino_JSON.h>

// ── Hardware objects ──────────────────────────────────────────────────────────
ScioSense_ENS160 ens160(ENS160_I2CADDR_1); 
Adafruit_AHTX0    aht; 
BH1750            lightMeter;
OneWire           oneWire(ONE_WIRE_BUS);
DallasTemperature waterSensors(&oneWire);
Adafruit_BMP5xx   bmp580;
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
CircularBuffer<float, 50>         pressure_buffer;
CircularBuffer<float, 50>         bmp580_temp_buffer;
CircularBuffer<unsigned long, 50> current_time_buffer;

// ── Hourly aggregation buffers ────────────────────────────────────────────────
CircularBuffer<float, 48>         hourly_temperature_buffer;
CircularBuffer<float, 48>         hourly_humidity_buffer;
CircularBuffer<float, 48>         hourly_water_temp_buffer;
CircularBuffer<float, 48>         hourly_light_buffer;
CircularBuffer<float, 48>         hourly_AQI_buffer;
CircularBuffer<float, 48>         hourly_VOC_buffer;
CircularBuffer<float, 48>         hourly_CO2_buffer;
CircularBuffer<float, 48>         hourly_pressure_buffer;
CircularBuffer<float, 48>         hourly_bmp580_temp_buffer;
CircularBuffer<unsigned long, 48> hourly_time_buffer;

// ── Current readings ──────────────────────────────────────────────────────────
float temperature     = -100.0;
float humidity   = -100.0;
float hic             = -100.0;
float water_temperature = -100.0;
float light_intensity   = -100.0;
float bmp580_pressure   = -100.0;
float temperature_bmp580 = -100.0;
int CO2              = -100;

uint8_t aqi = 0;
uint16_t tvoc = 0;
uint16_t eco2 = 0;

uint8_t sensor_enabled_mask = SENSOR_ALL_ENABLED;

static JSONVar readings;

// ── Per-sensor init status ────────────────────────────────────────────────────
static bool bh1750_ready = false;
static bool aht21_ready  = false;
static bool ens160_ready = false;
static bool bmp580_ready = false;

static bool initBH1750() {
  bh1750_ready = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  if (!bh1750_ready) Serial.println("BH1750 init failed");
  else               Serial.println("BH1750 OK");
  return bh1750_ready;
}
static bool initAHT21() {
  aht21_ready = aht.begin();
  if (!aht21_ready) Serial.println("AHT21 init failed");
  else              Serial.println("AHT21 OK");
  return aht21_ready;
}
static bool initENS160() {
  // Bypass begin() entirely — it calls Wire.begin() (bus reset), CLRGPR, and
  // getFirmware() which leaves GET_APPVER active in COMMAND.  Any of these can
  // block the prediction engine after repeated resets.  Minimal raw sequence:
  //   RESET → IDLE → NOP command → STD
  Wire.beginTransmission(ENS160_I2CADDR_1);
  Wire.write(ENS160_REG_OPMODE);
  Wire.write(ENS160_OPMODE_RESET);
  Wire.endTransmission();
  delay(10);

  Wire.beginTransmission(ENS160_I2CADDR_1);
  Wire.write(ENS160_REG_OPMODE);
  Wire.write(ENS160_OPMODE_IDLE);
  Wire.endTransmission();
  delay(10);

  Wire.beginTransmission(ENS160_I2CADDR_1);
  Wire.write(ENS160_REG_COMMAND);
  Wire.write(ENS160_COMMAND_NOP);
  Wire.endTransmission();
  delay(10);

  Wire.beginTransmission(ENS160_I2CADDR_1);
  Wire.write(ENS160_REG_OPMODE);
  Wire.write(ENS160_OPMODE_STD);
  Wire.endTransmission();
  delay(10);

  // Read back OPMODE to confirm
  Wire.beginTransmission(ENS160_I2CADDR_1);
  Wire.write(ENS160_REG_OPMODE);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)ENS160_I2CADDR_1, (uint8_t)1);
  uint8_t opmode = Wire.available() ? Wire.read() : 0xFF;
  ens160_ready = (opmode == ENS160_OPMODE_STD);
  Serial.print("ENS160 raw init OPMODE: 0x"); Serial.print(opmode, HEX);
  Serial.println(ens160_ready ? " OK" : " FAILED");

  if (ens160_ready) {
    AQI_buffer.clear();
    VOC_buffer.clear();
    CO2_buffer.clear();
  }
  return ens160_ready;
}
static bool initBMP580() {
  bmp580_ready = bmp580.begin(BMP5XX_ALTERNATIVE_ADDRESS, &Wire);
  if (!bmp580_ready) Serial.println("BMP580 init failed");
  else               Serial.println("BMP580 OK");
  return bmp580_ready;
}

// ── Init ──────────────────────────────────────────────────────────────────────
void initSensors() {
  auto i2cReset = []() { Wire.begin(); delay(50); };

  if (sensor_enabled_mask & SENSOR_BH1750_BIT)  { if (!initBH1750())  i2cReset(); }
  if (sensor_enabled_mask & SENSOR_AHT21_BIT)   { if (!initAHT21())   i2cReset(); }
  if (sensor_enabled_mask & SENSOR_ENS160_BIT)  { if (!initENS160())  i2cReset(); }
  if (sensor_enabled_mask & SENSOR_DS18B20_BIT) waterSensors.begin();
  if (sensor_enabled_mask & SENSOR_BMP580_BIT)  initBMP580();
}

// ── JSON snapshot ─────────────────────────────────────────────────────────────
String getSensorReadings() {
  readings["water_temperature"] = String(water_temperature);
  readings["humidity"]          = String(humidity);
  readings["light"]             = String(light_intensity);
  readings["temperature"]       = String(temperature);
  readings["AQI"]               = AQI_buffer.isEmpty() ? String(0) : String(AQI_buffer.last());
  readings["VOC"]               = VOC_buffer.isEmpty() ? String(0) : String(VOC_buffer.last());
  readings["CO2"]               = CO2_buffer.isEmpty() ? String(0) : String(CO2_buffer.last());
  readings["pressure"]          = String(bmp580_pressure);
  readings["temperature_bmp580"] = String(temperature_bmp580);
  readings["timestamp"]         = current_time_buffer.isEmpty()
                                    ? String("0")
                                    : String(current_time_buffer.last());
  return JSON.stringify(readings);
}

// ── Sensor status ─────────────────────────────────────────────────────────────
// Returns JSON: {"aht21":"ok"|"fault"|"disabled", "ens160":..., ...}
String getSensorStatus() {
  static JSONVar s;
  auto st = [](bool enabled, bool ready) -> const char* {
    if (!enabled) return "disabled";
    return ready ? "ok" : "fault";
  };
  s["aht21"]   = st(sensor_enabled_mask & SENSOR_AHT21_BIT,   aht21_ready);
  s["ens160"]  = st(sensor_enabled_mask & SENSOR_ENS160_BIT,  ens160_ready);
  s["bh1750"]  = st(sensor_enabled_mask & SENSOR_BH1750_BIT,  bh1750_ready);
  s["bmp580"]  = st(sensor_enabled_mask & SENSOR_BMP580_BIT,  bmp580_ready);
  // DS18B20 has no init-time ready flag; use the current reading as proxy
  bool ds18b20_ok = (water_temperature > -50.0f);
  s["ds18b20"] = st(sensor_enabled_mask & SENSOR_DS18B20_BIT, ds18b20_ok);
  return JSON.stringify(s);
}

// ── Validation helpers ────────────────────────────────────────────────────────
// Returns true and accepts the reading; logs and discards on failure.
static bool validateFloat(const char* name, float val,
                           float minV, float maxV,
                           const CircularBuffer<float, 50>* buf = nullptr,
                           float maxDelta = -1) {
  if (isnan(val) || val < minV || val > maxV) {
    Serial.print("Rejected "); Serial.print(name);
    Serial.print(" (out of range): "); Serial.println(val);
    return false;
  }
  if (maxDelta >= 0 && buf && !buf->isEmpty()) {
    float delta = fabs(val - buf->last());
    if (delta > maxDelta) {
      Serial.print("Rejected "); Serial.print(name);
      Serial.print(" (spike +/-"); Serial.print(delta);
      Serial.print("): "); Serial.println(val);
      return false;
    }
  }
  return true;
}

static bool validateUint16Ratio(const char* name, uint16_t val,
                                 uint16_t minV, uint16_t maxV,
                                 const CircularBuffer<uint16_t, 50>* buf,
                                 float maxRatio) {
  if (val < minV || val > maxV) {
    Serial.print("Rejected "); Serial.print(name);
    Serial.print(" (out of range): "); Serial.println(val);
    return false;
  }
  if (buf && !buf->isEmpty() && buf->last() > minV) {
    float ratio = (float)val / (float)buf->last();
    if (ratio > maxRatio || ratio < (1.0f / maxRatio)) {
      Serial.print("Rejected "); Serial.print(name);
      Serial.print(" (ratio "); Serial.print(ratio);
      Serial.print("): "); Serial.println(val);
      return false;
    }
  }
  return true;
}

// ── Sensor callbacks ──────────────────────────────────────────────────────────
void sample_sensor_ENS160_callback() {
  bool aht21_wanted  = !!(sensor_enabled_mask & SENSOR_AHT21_BIT);
  bool ens160_wanted = !!(sensor_enabled_mask & SENSOR_ENS160_BIT);
  if (!aht21_wanted && !ens160_wanted) return;

  // Lazy re-init: if a sensor failed to init at startup, try once per cycle.
  // If re-init just succeeded this tick, return — the sensor needs at least one
  // sampling interval to produce a valid first measurement before we read it.
  bool just_inited = false;
  if (!aht21_ready  && aht21_wanted)  { initAHT21();   just_inited = true; }
  if (!ens160_ready && ens160_wanted) { initENS160();  just_inited = true; }
  if (!aht21_ready && !ens160_ready) return; // nothing to read
  if (just_inited) return;                   // let the sensor settle one cycle

  sensors_event_t hum, temp;
  if (aht21_ready) aht.getEvent(&hum, &temp);

  if (ens160_ready) {
    // measure() MUST come first — any read of DATA_STATUS (register 0x20)
    // clears NEWDAT, so nothing else may touch the sensor before this call.
    ens160.measure(false);
    // Update compensation after reading so set_envdata() cannot clear NEWDAT.
    float comp_t = aht21_ready ? temp.temperature : temperature + 3;
    float comp_h = aht21_ready ? hum.relative_humidity : humidity;
    ens160.set_envdata(comp_t, comp_h);
  }
  uint8_t  new_aqi  = ens160_ready ? ens160.getAQI()  : 0;
  uint16_t new_tvoc = ens160_ready ? ens160.getTVOC() : 0;
  uint16_t new_eco2 = ens160_ready ? ens160.geteCO2() : 0;

  if (aht21_ready) {
    float t = temp.temperature;
    float h = hum.relative_humidity;
    if (validateFloat("ENS160 temp", t, -30.0f, 60.0f, &temperature_buffer, 15.0f)) {
      temperature = t - 3; // AHT21 reads ~3°C high due to self-heating
      temperature_buffer.push(t);
    }
    if (validateFloat("humidity", h, 0.0f, 100.0f, &humidity_buffer, 30.0f)) {
      humidity = h;
      humidity_buffer.push(h);
    }
    if (DEBUG) {
      Serial.print("Ext. temperature: "); Serial.println(t);
      Serial.print("Humidity: ");         Serial.println(h);
    }
  }
  if (ens160_ready) {
    if (DEBUG) {
      Serial.print("AQI: "); Serial.println(new_aqi);
      Serial.print("VOC: "); Serial.println(new_tvoc);
      Serial.print("CO2: "); Serial.println(new_eco2);
    }
    // AQI=0 means the sensor is in startup/warm-up — skip these samples.
    // AQI 1–5 is the valid operating range; only then commit all three values.
    if (new_aqi >= 1 && new_aqi <= 5) {
      aqi  = new_aqi;  AQI_buffer.push(new_aqi);
      tvoc = new_tvoc; VOC_buffer.push(new_tvoc);
      eco2 = new_eco2; CO2_buffer.push(new_eco2);
    }
  }
}

void sample_sensor_water_temp_callback() {
  if (!(sensor_enabled_mask & SENSOR_DS18B20_BIT)) return;
  waterSensors.requestTemperatures();
  float t = waterSensors.getTempCByIndex(0);

  // DS18B20 returns -127 on error; reject negatives and out-of-range values
  if (validateFloat("water temp", t, -5.0f, 100.0f, &water_temp_buffer, 20.0f)) {
    water_temperature = t;
    water_temp_buffer.push(t);
  }

  if (DEBUG) { Serial.print("Water temperature: "); Serial.println(String(t)); }
}

void sample_sensor_BH1750_callback() {
  if (!(sensor_enabled_mask & SENSOR_BH1750_BIT)) return;
  if (!bh1750_ready) { initBH1750(); return; } // settle one cycle after re-init

  float lux = lightMeter.readLightLevel();
  // BH1750 max range is 65535 lux; negative values indicate a read error
  if (validateFloat("light", lux, 0.0f, 65535.0f)) {
    light_intensity = lux;
    lightIntensity_buffer.push(lux);
  }
  if (DEBUG) { Serial.print("Light Meter: "); Serial.print(lux); Serial.println(" lx"); }
}

void sample_sensor_BMP580_callback() {
  if (!(sensor_enabled_mask & SENSOR_BMP580_BIT)) return;
  if (!bmp580_ready) { initBMP580(); return; } // settle one cycle after re-init

  if (!bmp580.performReading()) {
    Serial.println("BMP580 read failed");
    bmp580_ready = false; // force re-init next cycle
    return;
  }

  float p = bmp580.pressure;    // hPa
  float t = bmp580.temperature; // °C

  if (validateFloat("BMP580 pressure", p, 870.0f, 1084.0f, &pressure_buffer, 20.0f)) {
    bmp580_pressure = p;
    pressure_buffer.push(p);
  }
  if (validateFloat("BMP580 temp", t, -30.0f, 60.0f, &bmp580_temp_buffer, 15.0f)) {
    temperature_bmp580 = t;
    bmp580_temp_buffer.push(t);
  }

  if (DEBUG) {
    Serial.print("BMP580 pressure: ");    Serial.print(p); Serial.println(" hPa");
    Serial.print("BMP580 temperature: "); Serial.print(t); Serial.println(" °C");
  }
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

// ── Hourly aggregation ────────────────────────────────────────────────────────
static float avgFloat(const CircularBuffer<float, 50>& buf) {
  float s = 0;
  for (int i = 0; i < (int)buf.size(); i++) s += buf[i];
  return s / buf.size();
}
static float avgUint8(const CircularBuffer<uint8_t, 50>& buf) {
  float s = 0;
  for (int i = 0; i < (int)buf.size(); i++) s += buf[i];
  return s / buf.size();
}
static float avgUint16(const CircularBuffer<uint16_t, 50>& buf) {
  float s = 0;
  for (int i = 0; i < (int)buf.size(); i++) s += buf[i];
  return s / buf.size();
}

void aggregate_hourly_callback() {
  unsigned long t = current_time_buffer.isEmpty() ? 0 : current_time_buffer.last();
  hourly_time_buffer.push(t);
  hourly_temperature_buffer.push(temperature_buffer.isEmpty() ? 0.0f : avgFloat(temperature_buffer));
  hourly_humidity_buffer.push(humidity_buffer.isEmpty()       ? 0.0f : avgFloat(humidity_buffer));
  hourly_water_temp_buffer.push(water_temp_buffer.isEmpty()   ? 0.0f : avgFloat(water_temp_buffer));
  hourly_light_buffer.push(lightIntensity_buffer.isEmpty()    ? 0.0f : avgFloat(lightIntensity_buffer));
  hourly_AQI_buffer.push(AQI_buffer.isEmpty()                 ? 0.0f : avgUint8(AQI_buffer));
  hourly_VOC_buffer.push(VOC_buffer.isEmpty()                 ? 0.0f : avgUint16(VOC_buffer));
  hourly_CO2_buffer.push(CO2_buffer.isEmpty()                 ? 0.0f : avgUint16(CO2_buffer));
  hourly_pressure_buffer.push(pressure_buffer.isEmpty()       ? 0.0f : avgFloat(pressure_buffer));
  hourly_bmp580_temp_buffer.push(bmp580_temp_buffer.isEmpty() ? 0.0f : avgFloat(bmp580_temp_buffer));

  if (DEBUG) Serial.println("Hourly aggregation stored");
}

void update_date_time_callback() {
  unsigned long t = 0;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("NTP skipped: WiFi not connected");
  } else {
    t = ntpClient.getUnixTime();
    if (t == 0) Serial.println("NTP returned 0");
  }

  // Always push a timestamp so current_time_buffer stays the same size as the
  // sensor buffers.  On failure, repeat the last known good time (or 0 on
  // first boot) so every sensor row has a corresponding timestamp entry.
  if (t == 0 && !current_time_buffer.isEmpty()) t = current_time_buffer.last();
  current_time_buffer.push(t);

  if (DEBUG) {
    char buffer[40];
    sprintf(buffer, "%lu", t);
    Serial.print("time: ");
    Serial.println(buffer);
  }
}
