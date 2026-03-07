#include <Arduino.h>
#include "wifi_manager.h"
#include "config.h"
#include "pwd.h"

#include <ESP8266WiFi.h>
#include <TaskSchedulerDeclarations.h>

// Task objects defined in the main .ino
extern Task task_sample_ENS160;
extern Task task_sample_water_temp;
extern Task task_sample_BH1750;
extern Task task_update_date_time;
extern Task task_send_new_readings_event;
extern Task task_check_wifi_connected;

static IPAddress local_IP(192, 168, 3, 44);
static IPAddress gateway(192, 168, 3, 1);
static IPAddress subnet(255, 255, 255, 0);

void initWiFi() {
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
}

void task_check_wifi_connected_callback() {
  if (WiFi.status() == WL_CONNECTED) return;

  task_sample_ENS160.disable();
  task_sample_water_temp.disable();
  task_sample_BH1750.disable();
  task_update_date_time.disable();
  task_send_new_readings_event.disable();
  task_check_wifi_connected.disable();

  while (WiFi.status() != WL_CONNECTED) {
    int waiting_periods = 5;
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    while (waiting_periods > 0 && WiFi.status() != WL_CONNECTED) {
      delay(3000);
      Serial.println(" not connected to wifi-yet ");
      waiting_periods--;
    }
  }

  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());

  task_sample_ENS160.enable();
  task_sample_water_temp.enableDelayed(1000);
  task_sample_BH1750.enableDelayed(2000);
  task_update_date_time.enableDelayed(4000);
  task_send_new_readings_event.enable();
  task_check_wifi_connected.enable();
}
