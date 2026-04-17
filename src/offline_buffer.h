#pragma once
#include <Arduino.h>

// One entry is written per AGG window while offline.
// 120 entries × 5 min = 10 hours of offline coverage.
#define OFFLINE_AGG_INTERVAL_MS  (5UL * 60UL * 1000UL)
#define OFFLINE_MAX_ENTRIES      120
#define OFFLINE_BUFFER_FILE      "/offlinebuf.csv"

// Call once at startup (after LittleFS.begin())
void offlineBuffer_init();

// Feed a sensor sample while offline. Accumulates internally; auto-flushes
// to LittleFS every OFFLINE_AGG_INTERVAL_MS.
void offlineBuffer_addSample(float water_temp, float temp, float temp_bmp580,
                              float humidity,  float light,
                              float co2,       float voc,
                              float aqi,       float pressure,
                              unsigned long    unix_ts);

// Force-flush the current RAM accumulator to LittleFS even if the window
// hasn't expired yet (call this just before attempting a server sync).
void offlineBuffer_flushWindow();

// POST all buffered entries to the server. Returns true and clears the file
// on success; returns false and leaves the file intact on failure.
bool offlineBuffer_syncToServer();

// True if there is a non-empty buffer file on LittleFS.
bool offlineBuffer_hasPending();
