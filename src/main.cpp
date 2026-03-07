#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <CircularBuffer.h>
#include <EEPROM.h>
#include "DHT.h"
#include <Wire.h>
#include <BH1750.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TaskScheduler.h>
#include <EasyNTPClient.h>
#include <WiFiUdp.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"

#include "config.h"
#include "sensors.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "html_ui.h"
// pwd.h included only in wifi_manager.cpp to avoid duplicate symbol definitions

// ── Globals ───────────────────────────────────────────────────────────────────
boolean     DEBUG             = true;
const char* http_username     = "WaterUser";
const char* http_password     = "WJJjCSHiXCFTiA;DR=2[";
int         reporting_interval = 3000;

// ── Scheduler and tasks ───────────────────────────────────────────────────────
Scheduler runner;

void task_initiator_callback(); // forward declaration

Task task_initiator              (1000,               TASK_ONCE,    task_initiator_callback);
Task task_sample_ENS160          (DEFAULT_SAMPLE_RATE, TASK_FOREVER, &sample_sensor_ENS160_callback);
Task task_sample_water_temp      (DEFAULT_SAMPLE_RATE, TASK_FOREVER, &sample_sensor_water_temp_callback);
Task task_sample_BH1750          (DEFAULT_SAMPLE_RATE, TASK_FOREVER, &sample_sensor_BH1750_callback);
Task task_update_date_time       (DEFAULT_SAMPLE_RATE, TASK_FOREVER, &update_date_time_callback);
Task task_send_new_readings_event(DEFAULT_SAMPLE_RATE, TASK_FOREVER, &send_new_readings_event_callback);
Task task_check_wifi_connected   (1000,               TASK_FOREVER, &task_check_wifi_connected_callback);

// ── LittleFS init ─────────────────────────────────────────────────────
static void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// ── Task initiator callback ───────────────────────────────────────────────────
void task_initiator_callback() {
  readSamplingFreqParamer();
  Serial.println("First iteration. Set sensors sampling time to: " + reporting_interval);

  task_sample_ENS160.setInterval(reporting_interval);
  task_sample_water_temp.setInterval(reporting_interval);
  task_sample_BH1750.setInterval(reporting_interval);
  task_update_date_time.setInterval(reporting_interval);

  task_sample_ENS160.enable();
  task_sample_water_temp.enableDelayed(1000);
  task_sample_BH1750.enableDelayed(2000);
  task_update_date_time.enableDelayed(4000);
  task_send_new_readings_event.enable();
  task_check_wifi_connected.enable();
  digitalWrite(SSRPin, HIGH);
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
  Wire.begin();
  Serial.begin(115200);
  pinMode(D0,     OUTPUT);
  pinMode(SSRPin, OUTPUT);
  pinMode(DHTPin, INPUT);

  EEPROM.begin(128);
  initFS();
  initSensors();

  runner.init();
  runner.addTask(task_initiator);
  runner.addTask(task_sample_ENS160);
  runner.addTask(task_sample_water_temp);
  runner.addTask(task_sample_BH1750);
  runner.addTask(task_update_date_time);
  runner.addTask(task_send_new_readings_event);
  runner.addTask(task_check_wifi_connected);

  initWiFi();
  task_initiator.enable();
  initWebServer();
}

// ── Main loop ─────────────────────────────────────────────────────────────────
void loop() {
  runner.execute();
}
