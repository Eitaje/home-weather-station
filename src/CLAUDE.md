# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Flash

This is an Arduino sketch for an ESP8266 NodeMCU v2 board. Build and flash using the **Arduino IDE** — there is no CLI build system available in this environment.

- Board: `esp8266:esp8266:nodemcuv2`
- Filesystem upload (LittleFS `data/` folder): Arduino IDE → Tools → ESP8266 LittleFS Data Upload

Pre-built binary: `NodeMCU-weather-station-server.ino.nodemcu.bin`
Build artifacts: `build/esp8266.esp8266.nodemcuv2/`

## Architecture

The sketch runs a WiFi-connected weather station web server. Sensor sampling, NTP time sync, and SSE event pushing are all driven by a **TaskScheduler** (`runner`) — there is no blocking code in `loop()`, which just calls `runner.execute()`.

### File layout (post-refactor)

| File | Role |
|---|---|
| `NodeMCU-weather-station-server.ino` | Entry point: globals (`DEBUG`, credentials, `reporting_interval`), all `Task` objects, `setup()`, `loop()`, `task_initiator_callback()` |
| `config.h` | `#define` constants, pin assignments (`DHTPin D3`, `ONE_WIRE_BUS D4`, `SSRPin D5`) |
| `sensors.h` / `sensors.cpp` | All sensor driver objects, 50-sample circular buffers, current reading globals, `initSensors()`, five sampling callbacks, `getSensorReadings()` (JSON) |
| `wifi_manager.h` / `wifi_manager.cpp` | Static IP config, `initWiFi()`, reconnect task callback |
| `web_server.h` / `web_server.cpp` | `AsyncWebServer`/`AsyncEventSource` objects, boiler state, all HTTP route handlers, `initWebServer()`, `readSamplingFreqParamer()` (EEPROM) |
| `html_ui.h` / `html_ui.cpp` | Server-side HTML generation (`SendHTML`, `prepareReportHTML`, `prepareReportMultiLines`) |
| `data/` | LittleFS filesystem: `index.html`, `script.js`, `style.css`, `favicon.png` — served as static files |
| `pwd.h` | WiFi SSID and password (not committed to public repos) |

### Key design patterns

**Cross-file globals via `extern`:** Task objects (`task_sample_DHT`, etc.) are defined in the `.ino` and declared `extern` in `wifi_manager.cpp` and `web_server.cpp` where they need to be enabled/disabled. Same pattern for `DEBUG`, `reporting_interval`, `http_username/password`.

**Startup sequence:** `task_initiator` fires once after 1 s, reads `reporting_interval` from EEPROM, sets all task intervals, then enables all other tasks. This avoids sampling before WiFi is ready.

**Boiler/SSR control:** `SSRPin` (D5) drives a solid-state relay. `D0` is a status LED mirroring the relay. State is tracked in `boiler_state` and `boiler_turned_on_timestamp` (both in `web_server.cpp`).

**Sensor data flow:** Each sensor callback reads hardware → updates the current-value global → pushes to a `CircularBuffer<float, 50>`. `getSensorReadings()` snapshots the current values (not the buffers) into a JSON string sent via SSE and `/curr_readings`. The full buffer history is available at `/all_samples` as CSV.

**OTA updates:** Served at `/update` via `AsyncElegantOTA`, protected by `http_username`/`http_password`.

### Hardware sensors

| Sensor | Interface | Variable |
|---|---|---|
| DHT22 | D3 (1-wire) | `temperature`, `humidity`, `hic` |
| DS18B20 | D4 (OneWire) | `water_temperature` |
| BH1750 | I2C | `light_intensity` |
| BMP388 | I2C | `bmp388_temp_buffer`, `bmp388_pressure_buffer` |

### Network

Static IP: `192.168.3.44`, gateway `192.168.3.1`. Credentials in `pwd.h`. NTP: `pool.ntp.org` at GMT+2.
