#include "offline_buffer.h"
#include "config.h"
#include "pwd.h"
#include "LittleFS.h"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// ── RAM accumulator ───────────────────────────────────────────────────────────
static struct {
  double water_temp, temp, temp_bmp580;
  double humidity, light, co2, voc, aqi, pressure;
  int    count;
  unsigned long window_start_ms;
  unsigned long window_start_ts;   // unix epoch seconds
} _acc;

static bool _initialised = false;

// ── Helpers ───────────────────────────────────────────────────────────────────
static int _countLines() {
  if (!LittleFS.exists(OFFLINE_BUFFER_FILE)) return 0;
  File f = LittleFS.open(OFFLINE_BUFFER_FILE, "r");
  if (!f) return 0;
  int n = 0;
  while (f.available()) if (f.read() == '\n') n++;
  f.close();
  return n;
}

static void _resetAcc() {
  memset(&_acc, 0, sizeof(_acc));
  _acc.window_start_ms = millis();
}

// ── Public API ────────────────────────────────────────────────────────────────
void offlineBuffer_init() {
  _resetAcc();
  _initialised = true;
}

void offlineBuffer_addSample(float water_temp, float temp, float temp_bmp580,
                              float humidity,  float light,
                              float co2,       float voc,
                              float aqi,       float pressure,
                              unsigned long    unix_ts) {
  if (!_initialised) offlineBuffer_init();

  if (_acc.count == 0) {
    _acc.window_start_ts = unix_ts;
    _acc.window_start_ms = millis();
  }
  _acc.water_temp   += water_temp;
  _acc.temp         += temp;
  _acc.temp_bmp580  += temp_bmp580;
  _acc.humidity     += humidity;
  _acc.light        += light;
  _acc.co2          += co2;
  _acc.voc          += voc;
  _acc.aqi          += aqi;
  _acc.pressure     += pressure;
  _acc.count++;

  if (millis() - _acc.window_start_ms >= OFFLINE_AGG_INTERVAL_MS) {
    offlineBuffer_flushWindow();
  }
}

void offlineBuffer_flushWindow() {
  if (!_initialised || _acc.count == 0) return;

  if (_countLines() >= OFFLINE_MAX_ENTRIES) {
    Serial.println("[offline] buffer full — oldest window dropped");
    _resetAcc();
    return;
  }

  File f = LittleFS.open(OFFLINE_BUFFER_FILE, "a");
  if (!f) { Serial.println("[offline] cannot open buffer file"); return; }

  float n = (float)_acc.count;
  // CSV: ts,water_temp,temp,temp_bmp580,humidity,light,co2,voc,aqi,pressure
  f.printf("%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
    _acc.window_start_ts,
    _acc.water_temp  / n, _acc.temp        / n, _acc.temp_bmp580 / n,
    _acc.humidity    / n, _acc.light       / n,
    _acc.co2         / n, _acc.voc         / n,
    _acc.aqi         / n, _acc.pressure    / n);
  f.close();

  Serial.printf("[offline] window flushed: %d samples, ts=%lu\n",
                _acc.count, _acc.window_start_ts);
  _resetAcc();
}

bool offlineBuffer_hasPending() {
  if (!LittleFS.exists(OFFLINE_BUFFER_FILE)) return false;
  return _countLines() > 0;
}

bool offlineBuffer_syncToServer() {
  if (!offlineBuffer_hasPending()) return true;

  File f = LittleFS.open(OFFLINE_BUFFER_FILE, "r");
  if (!f) return false;

  // Build JSON array — safe up to OFFLINE_MAX_ENTRIES × ~155 chars ≈ 18 KB
  String json;
  json.reserve(200 * _countLines() + 4);
  json = "[";
  bool first = true;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    // Parse 10 comma-separated fields
    float v[10];
    int   idx = 0, start = 0;
    for (int i = 0; i <= (int)line.length() && idx < 10; i++) {
      if (i == (int)line.length() || line[i] == ',') {
        v[idx++] = line.substring(start, i).toFloat();
        start = i + 1;
      }
    }
    if (idx < 10) continue;   // malformed line — skip

    if (!first) json += ',';
    first = false;
    char buf[160];
    snprintf(buf, sizeof(buf),
      "{\"timestamp\":%lu"
      ",\"water_temperature\":%.2f,\"temperature\":%.2f,\"temperature_bmp580\":%.2f"
      ",\"humidity\":%.2f,\"light\":%.2f"
      ",\"CO2\":%.0f,\"VOC\":%.0f,\"AQI\":%.0f"
      ",\"pressure\":%.2f}",
      (unsigned long)v[0],
      v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9]);
    json += buf;
  }
  f.close();
  json += ']';

  if (first) {   // file was empty
    LittleFS.remove(OFFLINE_BUFFER_FILE);
    return true;
  }

  // POST to server
  WiFiClient  client;
  HTTPClient  http;
  String url = String("http://") + SERVER_IP + ":" + SERVER_PORT +
               "/devices/nodemcu/bulk_readings";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000);

  Serial.printf("[offline] syncing %d entries to %s\n", _countLines(), url.c_str());
  int code = http.POST(json);
  http.end();

  Serial.printf("[offline] sync result: HTTP %d\n", code);

  if (code == 200 || code == 201) {
    LittleFS.remove(OFFLINE_BUFFER_FILE);
    Serial.println("[offline] buffer cleared");
    return true;
  }
  return false;
}
