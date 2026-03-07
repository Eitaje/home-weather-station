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
extern Task task_sample_ENS160;

// Globals defined in the main .ino
extern int          reporting_interval;
extern const char*  http_username;
extern const char*  http_password;

// ── Server objects ────────────────────────────────────────────────────────────
AsyncWebServer  server(80);
AsyncEventSource events("/events");

// ── Boiler state ──────────────────────────────────────────────────────────────
boolean       boiler_state              = true;
unsigned long boiler_turned_on_timestamp = 0;

// ── EEPROM helpers ────────────────────────────────────────────────────────────
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
  String ptr = "time, external temp, humidity, water temp, light intensity, AQI, VOC, CO2\n";
  if (!temperature_buffer.isEmpty()) {
    for (int i = 0; i < temperature_buffer.size(); i++) {
      if (i < (int)current_time_buffer.size()) {
        sprintf(buffer, "%lu", current_time_buffer[i]);
        ptr += buffer;
      } else {
        ptr += "0";
      }
      ptr += ",";
      ptr += temperature_buffer[i];
      ptr += ",";
      ptr += humidity_buffer[i];
      ptr += ",";
      ptr += water_temp_buffer[i];
      ptr += ",";
      ptr += lightIntensity_buffer[i];
      ptr += ",";
      ptr += (i < (int)AQI_buffer.size()) ? String(AQI_buffer[i]) : "0";
      ptr += ",";
      ptr += (i < (int)VOC_buffer.size()) ? String(VOC_buffer[i]) : "0";
      ptr += ",";
      ptr += (i < (int)CO2_buffer.size()) ? String(CO2_buffer[i]) : "0";
      ptr += "\n";
    }
    request->send(200, "text/plain", ptr);
  } else {
    request->send(200, "text/plain", "No samples yet\n");
  }
}

static void handle_write_param(AsyncWebServerRequest *request) {
  if (request->hasParam("sample_interval", false)) {
    String dht_sample_interval_str = request->getParam("sample_interval", false, false)->value();
    int    dht_sampe_interval       = dht_sample_interval_str.toInt();

    if (isnan(dht_sampe_interval))            dht_sampe_interval = DEFAULT_SAMPLE_RATE;
    if (dht_sampe_interval <= 0)              dht_sampe_interval = 1000;
    else if (dht_sampe_interval > 86400000)   dht_sampe_interval = 86400000;

    task_sample_ENS160.setInterval(dht_sampe_interval);
    task_sample_BH1750.setInterval(dht_sampe_interval);
    task_sample_water_temp.setInterval(dht_sampe_interval);

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
  request->send(200, "text/html", SendHTML("Data reporting interval is " + String(reporting_interval) + " seconds"));
}

static void handle_NotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// ── Server init ───────────────────────────────────────────────────────────────
void initWebServer() {
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
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    handle_write_param(request);
  });

  server.on("/get_reporting_interval", HTTP_GET, [](AsyncWebServerRequest *request) {
    handle_read_param(request);
  });

  server.on("/all_samples", HTTP_GET, [](AsyncWebServerRequest *request) {
    handle_get_all_samples(request);
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

  server.onNotFound(handle_NotFound);
  AsyncElegantOTA.begin(&server, http_username, http_password);
  server.begin();
  Serial.println("HTTP server started");
}
