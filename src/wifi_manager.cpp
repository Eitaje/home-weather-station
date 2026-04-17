#include <Arduino.h>
#include "wifi_manager.h"
#include "config.h"
#include "pwd.h"
#include "offline_buffer.h"

#include <ESP8266WiFi.h>
#include <TaskSchedulerDeclarations.h>

// Task objects defined in main.cpp
extern Task task_sample_ENS160;
extern Task task_sample_water_temp;
extern Task task_sample_BH1750;
extern Task task_update_date_time;
extern Task task_send_new_readings_event;
extern Task task_check_wifi_connected;

static IPAddress local_IP(DEVICE_IP);
static IPAddress gateway(GATEWAY_IP);
static IPAddress subnet(SUBNET_MASK);

// ── Reconnect state machine ───────────────────────────────────────────────────
// Keeps the main loop non-blocking so sensor tasks keep running while offline.

enum class WifiState { CONNECTED, TRYING_EXTENDER, TRYING_ANY };
static WifiState     _wifiState        = WifiState::CONNECTED;
static int           _extenderAttempt  = 0;     // 1..3 before falling back
static unsigned long _nextAttemptMs    = 0;     // millis() of next connect attempt
static unsigned long _offlineSinceMs   = 0;     // millis() when disconnect was first detected

// After this long offline, restart the chip to recover a stuck WiFi stack.
// ESP8266 repeated disconnect/begin cycles can exhaust the driver state machine;
// a restart is the only reliable recovery.
static const unsigned long RESTART_AFTER_OFFLINE_MS = 15UL * 60UL * 1000UL;  // 15 min

static const unsigned long CONNECT_WAIT_MS = 8000;   // wait between attempts
static const int           EXTENDER_TRIES  = 3;

// ── Helpers ───────────────────────────────────────────────────────────────────
static void _startConnect(bool useExtender, bool fullReset = false) {
  // Only call WiFi.disconnect() on the very first attempt of a new offline episode
  // (fullReset == true).  Calling it on every retry tears down internal driver
  // state faster than the SDK can rebuild it, eventually leaving it stuck.
  if (fullReset) {
    WiFi.disconnect(false);
    delay(100);
  }
  if (useExtender) {
    Serial.printf("[wifi] extender attempt %d/%d\n", _extenderAttempt, EXTENDER_TRIES);
    WiFi.begin(ssid, password, 0, extender_bssid);
  } else {
    Serial.println("[wifi] trying any AP");
    WiFi.begin(ssid, password);
  }
  _nextAttemptMs = millis() + CONNECT_WAIT_MS;
}

static void _onReconnected() {
  Serial.println("[wifi] connected!");
  Serial.print("[wifi] IP: "); Serial.println(WiFi.localIP());
  _wifiState       = WifiState::CONNECTED;
  _extenderAttempt = 0;

  // Flush any in-RAM offline samples to LittleFS, then sync to server
  offlineBuffer_flushWindow();
  offlineBuffer_syncToServer();

  // Only re-enable tasks that were disabled on disconnect.
  // Sensor sampling tasks (ENS160, water_temp, BH1750, BMP580) were
  // never disabled — they kept running to feed the offline buffer.
  task_update_date_time.enableDelayed(4000);
  task_send_new_readings_event.enable();
  task_check_wifi_connected.enable();
}

// ── Public API ────────────────────────────────────────────────────────────────
void initWiFi() {
  if (!WiFi.config(local_IP, gateway, subnet, gateway)) {
    Serial.println("[wifi] STA config failed");
  }
  offlineBuffer_init();

  // Try extender first
  _extenderAttempt = 1;
  _wifiState = WifiState::TRYING_EXTENDER;
  _startConnect(true);

  // Block only during initial boot until first connection is established
  unsigned long deadline = millis() + 30000UL;
  while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    _wifiState = WifiState::CONNECTED;
    Serial.println("[wifi] initial connect OK");
    Serial.print("[wifi] IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("[wifi] initial connect timed out — will retry in background");
  }
}

void task_check_wifi_connected_callback() {
  if (WiFi.status() == WL_CONNECTED) {
    if (_wifiState != WifiState::CONNECTED) {
      // Just reconnected
      _onReconnected();
    }
    return;
  }

  // ── Disconnected ─────────────────────────────────────────────────────────
  if (_wifiState == WifiState::CONNECTED) {
    // First detection of this disconnect — disable SSE/event pushing
    Serial.println("[wifi] lost connection");
    _wifiState       = WifiState::TRYING_EXTENDER;
    _extenderAttempt = 1;
    _offlineSinceMs  = millis();
    task_send_new_readings_event.disable();
    task_update_date_time.disable();
    _startConnect(true, /*fullReset=*/true);
    return;
    // Note: sensor sampling tasks stay ENABLED — data continues to accumulate
  }

  // Restart if the WiFi stack has been stuck for too long — this is the only
  // reliable recovery from an exhausted ESP8266 driver state machine.
  if (millis() - _offlineSinceMs >= RESTART_AFTER_OFFLINE_MS) {
    Serial.println("[wifi] offline too long — restarting to recover WiFi stack");
    delay(100);
    ESP.restart();
  }

  // Not yet time for next attempt
  if ((long)(millis() - _nextAttemptMs) < 0) return;

  if (_wifiState == WifiState::TRYING_EXTENDER) {
    if (_extenderAttempt < EXTENDER_TRIES) {
      _extenderAttempt++;
      _startConnect(true);
    } else {
      Serial.println("[wifi] extender failed, falling back to any AP");
      _wifiState = WifiState::TRYING_ANY;
      _startConnect(false);
    }
  } else {
    // TRYING_ANY — keep retrying indefinitely until restart threshold
    _startConnect(false);
  }
}
