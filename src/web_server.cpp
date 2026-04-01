#include "web_server.h"
#include "config.h"
#include "sensors.h"
#include "html_ui.h"

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <EEPROM.h>
#include "LittleFS.h"
#include <TaskSchedulerDeclarations.h>

// Task objects defined in the main .ino
extern Task task_sample_water_temp;
extern Task task_sample_BH1750;
extern Task task_sample_BMP580;
extern Task task_sample_ENS160;
extern Task task_update_date_time;
extern Task task_send_new_readings_event;

// Globals defined in the main .ino
extern int          reporting_interval;
extern const char*  http_username;
extern const char*  http_password;

// Sensor enable mask (defined in sensors.cpp)
extern uint8_t sensor_enabled_mask;

// ── Server objects ────────────────────────────────────────────────────────────
AsyncWebServer  server(80);
AsyncEventSource events("/events");

// ── LittleFS file upload (used for OTA UI updates via curl) ──────────────────
static File uploadFile;

// ── Boiler state ──────────────────────────────────────────────────────────────
boolean       boiler_state              = true;
unsigned long boiler_turned_on_timestamp = 0;

// ── OTA reboot delay (seconds, persisted to EEPROM) ───────────────────────────
int ota_reboot_delay = OTA_REBOOT_DELAY;

// ── EEPROM helpers ────────────────────────────────────────────────────────────
static void readOtaDelayParam() {
  int     new_delay = 0;
  byte    value;
  int     index     = 0;

  while (index < 8) {
    value = EEPROM.read(ota_reboot_delay_address + index);
    if (value == 0) break;
    new_delay += (value - 48) * (int)pow(10, index);
    index++;
  }
  if (new_delay >= 5) ota_reboot_delay = new_delay;
}

void readSensorEnabledParam() {
  uint8_t val = EEPROM.read(sensor_enable_address);
  // 0xFF = uninitialised flash; 0x00 = all off (invalid) — default to all enabled
  if (val == 0xFF || val == 0x00) val = SENSOR_ALL_ENABLED;
  sensor_enabled_mask = val;
  Serial.print("Sensor enabled mask: 0x"); Serial.println(val, HEX);
}

void readSamplingFreqParamer() {
  int     new_reporting_interval = 0;
  byte    value;
  int     index                  = 0;
  boolean found_end_of_number    = false;

  while (!found_end_of_number && index < 512) {
    value = EEPROM.read(report_time_interval_address + index);
    if (value == 0) {
      found_end_of_number = true;
      if (new_reporting_interval > 0) {
        reporting_interval = new_reporting_interval;
        Serial.print(report_time_interval_address);
        Serial.print("\t");
        Serial.println(new_reporting_interval);
      }
    } else {
      new_reporting_interval += (value - 48) * pow(10, index);
      index++;
    }
  }

  if (new_reporting_interval == 0)
    reporting_interval = 3000;
}

// ── Event callback ────────────────────────────────────────────────────────────
void send_new_readings_event_callback() {
  events.send("ping", NULL, millis());
  events.send(getSensorReadings().c_str(), "new_readings", millis());
}

// ── HTTP handlers (file-local) ────────────────────────────────────────────────
static void handle_OnConnect(AsyncWebServerRequest *request) {
  // Check if the AHT21 is providing valid numbers
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from AHT21/ENS160");
    request->send(200, "text/html", "Error: Sensors not responding. Check I2C wiring.");
  } else {
    // This calls your UI function which should now include Lux, eCO2, and TVOC
    request->send(200, "text/html", prepareReportMultiLines());
  }
}

static void handle_hello(AsyncWebServerRequest *request) {
  Serial.println("GPIO7 Status: OFF | GPIO6 Status: OFF");
  request->send(200, "text/html", SendHTML("hello :-)"));
}

static void handle_get_all_samples(AsyncWebServerRequest *request) {
  char buffer[40];
  // Row count from the largest buffer so sensor data is never hidden.
  int n = 0;
  n = max(n, (int)current_time_buffer.size());
  n = max(n, (int)temperature_buffer.size());
  n = max(n, (int)pressure_buffer.size());
  n = max(n, (int)lightIntensity_buffer.size());
  n = max(n, (int)water_temp_buffer.size());
  n = max(n, (int)CO2_buffer.size());

  String ptr = "time, external temp, humidity, water temp, light intensity, AQI, VOC, CO2, pressure, temperature_bmp580\n";
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      // For rows where the NTP timestamp hasn't arrived yet (sensor callbacks
      // fire a few seconds before the NTP callback each cycle), fall back to
      // the last known timestamp rather than emitting "0".
      unsigned long ts = current_time_buffer.isEmpty() ? 0
                       : (i < (int)current_time_buffer.size())
                           ? current_time_buffer[i]
                           : current_time_buffer.last();
      sprintf(buffer, "%lu", ts);
      ptr += buffer;
      ptr += ",";
      ptr += (i < (int)temperature_buffer.size())    ? String(temperature_buffer[i])    : "0";
      ptr += ",";
      ptr += (i < (int)humidity_buffer.size())       ? String(humidity_buffer[i])       : "0";
      ptr += ",";
      ptr += (i < (int)water_temp_buffer.size())     ? String(water_temp_buffer[i])     : "0";
      ptr += ",";
      ptr += (i < (int)lightIntensity_buffer.size()) ? String(lightIntensity_buffer[i]) : "0";
      ptr += ",";
      ptr += (i < (int)AQI_buffer.size())            ? String(AQI_buffer[i])            : "0";
      ptr += ",";
      ptr += (i < (int)VOC_buffer.size())            ? String(VOC_buffer[i])            : "0";
      ptr += ",";
      ptr += (i < (int)CO2_buffer.size())            ? String(CO2_buffer[i])            : "0";
      ptr += ",";
      ptr += (i < (int)pressure_buffer.size())       ? String(pressure_buffer[i])       : "0";
      ptr += ",";
      ptr += (i < (int)bmp580_temp_buffer.size())    ? String(bmp580_temp_buffer[i])    : "0";
      ptr += "\n";
    }
    request->send(200, "text/plain", ptr);
  } else {
    request->send(200, "text/plain", "No samples yet\n");
  }
}

static void handle_get_hourly_samples(AsyncWebServerRequest *request) {
  char buf[20];
  String ptr = "time, external temp, humidity, water temp, light intensity, AQI, VOC, CO2, pressure, temperature_bmp580\n";
  if (!hourly_time_buffer.isEmpty()) {
    int n = (int)hourly_time_buffer.size();
    for (int i = 0; i < n; i++) {
      sprintf(buf, "%lu", hourly_time_buffer[i]);
      ptr += buf;
      ptr += ",";
      ptr += (i < (int)hourly_temperature_buffer.size()) ? String(hourly_temperature_buffer[i]) : "0";
      ptr += ",";
      ptr += (i < (int)hourly_humidity_buffer.size())    ? String(hourly_humidity_buffer[i])    : "0";
      ptr += ",";
      ptr += (i < (int)hourly_water_temp_buffer.size())  ? String(hourly_water_temp_buffer[i])  : "0";
      ptr += ",";
      ptr += (i < (int)hourly_light_buffer.size())       ? String(hourly_light_buffer[i])       : "0";
      ptr += ",";
      ptr += (i < (int)hourly_AQI_buffer.size())         ? String(hourly_AQI_buffer[i])         : "0";
      ptr += ",";
      ptr += (i < (int)hourly_VOC_buffer.size())         ? String(hourly_VOC_buffer[i])         : "0";
      ptr += ",";
      ptr += (i < (int)hourly_CO2_buffer.size())         ? String(hourly_CO2_buffer[i])         : "0";
      ptr += ",";
      ptr += (i < (int)hourly_pressure_buffer.size())    ? String(hourly_pressure_buffer[i])    : "0";
      ptr += ",";
      ptr += (i < (int)hourly_bmp580_temp_buffer.size()) ? String(hourly_bmp580_temp_buffer[i]) : "0";
      ptr += "\n";
    }
    request->send(200, "text/plain", ptr);
  } else {
    request->send(200, "text/plain", "No hourly data yet\n");
  }
}

static void handle_write_param(AsyncWebServerRequest *request) {
  if (request->hasParam("sample_interval", false)) {
    String dht_sample_interval_str = request->getParam("sample_interval", false, false)->value();
    int    dht_sampe_interval       = dht_sample_interval_str.toInt();

    if (isnan(dht_sampe_interval))            dht_sampe_interval = DEFAULT_SAMPLE_RATE;
    if (dht_sampe_interval <= 0)              dht_sampe_interval = 1000;
    else if (dht_sampe_interval > 86400000)   dht_sampe_interval = 86400000;

    reporting_interval = dht_sampe_interval;
    task_sample_ENS160.setInterval(dht_sampe_interval);
    task_sample_BH1750.setInterval(dht_sampe_interval);
    task_sample_BMP580.setInterval(dht_sampe_interval);
    task_sample_water_temp.setInterval(dht_sampe_interval);
    task_update_date_time.setInterval(dht_sampe_interval);
    task_send_new_readings_event.setInterval(dht_sampe_interval);

    int index = 0;
    for (int x = dht_sample_interval_str.length() - 1; x >= 0; x--)
      EEPROM.write(report_time_interval_address + index++, dht_sample_interval_str[x]);
    EEPROM.write(report_time_interval_address + dht_sample_interval_str.length(), (char)0);

    if (EEPROM.commit()) {
      request->send(200, "text/html", SendHTML("Set data reporting interval to: " + String(reporting_interval) + " seconds"));
      Serial.println("EEPROM successfully committed");
      Serial.println("Wrote reporting_interval value to flash: " + dht_sample_interval_str);
    } else {
      Serial.println("ERROR! EEPROM commit failed");
    }
    delay(100);
  } else {
    request->send(200, "text/plain", "reporting_interval was not specified or illigal");
  }
}

static void handle_read_param(AsyncWebServerRequest *request) {
  readSamplingFreqParamer();
  request->send(200, "text/plain", String(reporting_interval));
}

static void handle_NotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// ── Server init ───────────────────────────────────────────────────────────────
void initWebServer() {
  readOtaDelayParam();
  // sensor_enabled_mask is read in setup() before initSensors(); no re-read needed here
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (LittleFS.exists("/index.html"))
      request->send(LittleFS, "/index.html", "text/html");
    else
      request->send(500, "text/plain", "index.html not found — upload LittleFS filesystem");
  });

  server.on("/curr_readings", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  server.addHandler(&events);

  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request) {
    handle_hello(request);
  });

  server.on("/set_reporting_interval", HTTP_GET, [](AsyncWebServerRequest *request) {
    handle_write_param(request);
  });

  server.on("/get_reporting_interval", HTTP_GET, [](AsyncWebServerRequest *request) {
    handle_read_param(request);
  });

  server.on("/all_samples", HTTP_GET, [](AsyncWebServerRequest *request) {
    handle_get_all_samples(request);
  });

  server.on("/hourly_samples", HTTP_GET, [](AsyncWebServerRequest *request) {
    handle_get_hourly_samples(request);
  });

  server.serveStatic("/", LittleFS, "/");

  server.on("/button_update", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    if (request->hasParam("state")) {
      inputMessage  = request->getParam("state")->value();
      inputParam    = "state";
      int int_state = 1 - inputMessage.toInt();
      if (int_state == 0) {
        digitalWrite(D0,    HIGH);
        digitalWrite(SSRPin, LOW);
        boiler_state = true;
        Serial.println("Setting SSR to Low");
      } else {
        digitalWrite(D0,    LOW);
        digitalWrite(SSRPin, HIGH);
        boiler_state              = false;
        boiler_turned_on_timestamp = millis();
        Serial.println("Setting SSR to HIGH");
      }
    } else {
      inputMessage = "No message sent";
      inputParam   = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  server.on("/button_state", HTTP_GET, [](AsyncWebServerRequest *request) {
    bool   boiler_pin = digitalRead(D0);
    String ans        = String(boiler_pin).c_str();
    if (boiler_pin) {
      ans += ",";
      float time_in_minutes = (millis() - boiler_turned_on_timestamp) / 60000.0;
      ans += String(time_in_minutes);
    }
    request->send(200, "text/plain", ans);
  });

  server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", FIRMWARE_VERSION);
  });

  server.on("/ota_delay", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(ota_reboot_delay));
  });

  server.on("/set_ota_delay", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("delay")) {
      String val = request->getParam("delay")->value();
      int d = val.toInt();
      if (d >= 5 && d <= 300) {
        ota_reboot_delay = d;
        int index = 0;
        for (int x = val.length() - 1; x >= 0; x--)
          EEPROM.write(ota_reboot_delay_address + index++, val[x]);
        EEPROM.write(ota_reboot_delay_address + val.length(), (char)0);
        EEPROM.commit();
      }
      request->send(200, "text/plain", String(ota_reboot_delay));
    } else {
      request->send(400, "text/plain", "delay parameter missing");
    }
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send(LittleFS, "/update.html", "text/html");
  });

  server.on("/upload", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (!request->authenticate(http_username, http_password))
        return request->requestAuthentication();
      request->send(200, "text/plain", "OK");
    },
    [](AsyncWebServerRequest *request, const String& filename, size_t index,
       uint8_t *data, size_t len, bool final) {
      if (!index) {
        String path = filename.startsWith("/") ? filename : "/" + filename;
        Serial.println("Upload start: " + path);
        uploadFile = LittleFS.open(path, "w");
      }
      if (uploadFile) uploadFile.write(data, len);
      if (final && uploadFile) {
        uploadFile.close();
        Serial.println("Upload done: " + filename);
      }
    }
  );

  server.on("/sensor_status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getSensorStatus());
  });

  server.on("/get_sensor_enabled", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(sensor_enabled_mask));
  });

  server.on("/set_sensor_enabled", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("mask")) {
      int val = request->getParam("mask")->value().toInt();
      if (val < 1)    val = 1;    // prevent all-sensors-off
      if (val > 0x1F) val = 0x1F;
      sensor_enabled_mask = (uint8_t)val;
      EEPROM.write(sensor_enable_address, sensor_enabled_mask);
      EEPROM.commit();
      request->send(200, "text/plain", String(sensor_enabled_mask));
    } else {
      request->send(400, "text/plain", "mask parameter missing");
    }
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Resetting...");
    delay(200);
    ESP.restart();
  });

  server.onNotFound(handle_NotFound);
  AsyncElegantOTA.begin(&server, http_username, http_password);
  server.begin();
  Serial.println("HTTP server started");
}
