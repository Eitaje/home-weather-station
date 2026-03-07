# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Flash

This is a **PlatformIO** project for an ESP8266 NodeMCU v2.

```bash
pio run                  # build firmware
pio run -t upload        # flash firmware (COM12)
pio run -t uploadfs      # upload LittleFS web UI files
```

Upload port is `COM12` in `platformio.ini`. Close any serial monitor before uploading — it holds the COM port.

LittleFS data directory: `src/data/` (set via `data_dir = src/data` in platformio.ini).

## Architecture

The sketch runs a WiFi-connected weather station web server. Sensor sampling, NTP time sync, and SSE event pushing are all driven by **TaskScheduler** (`runner`) — there is no blocking code in `loop()`, which just calls `runner.execute()`.

### File layout

| File | Role |
|---|---|
| `main.cpp` | Entry point: globals (`DEBUG`, credentials, `reporting_interval`), all `Task` objects, `setup()`, `loop()`, `task_initiator_callback()` |
| `config.h` | `#define` constants, pin assignments (`ONE_WIRE_BUS D4`, `SSRPin D5`) |
| `sensors.h` / `sensors.cpp` | All sensor driver objects, 50-sample circular buffers, current reading globals, `initSensors()`, sampling callbacks, `getSensorReadings()` (JSON) |
| `wifi_manager.h` / `wifi_manager.cpp` | Static IP config, `initWiFi()`, reconnect task callback |
| `web_server.h` / `web_server.cpp` | `AsyncWebServer`/`AsyncEventSource` objects, boiler state, all HTTP route handlers, `initWebServer()`, `readSamplingFreqParamer()` (EEPROM) |
| `html_ui.h` / `html_ui.cpp` | Server-side HTML generation (`SendHTML`, `prepareReportHTML`, `prepareReportMultiLines`) — legacy, main UI is LittleFS |
| `data/` | LittleFS filesystem: `index.html`, `script.js`, `style.css`, `favicon.png` — served as static files |
| `pwd.h` | WiFi SSID and password (not committed — gitignored) |

### Key design patterns

**Cross-file globals via `extern`:** Task objects are defined in `main.cpp` and declared `extern` where needed. Same pattern for `DEBUG`, `reporting_interval`, `http_username/password`.

**Startup sequence:** `task_initiator` fires once after 1 s, reads `reporting_interval` from EEPROM, sets all task intervals, then enables all other tasks. This avoids sampling before WiFi is ready.

**Boiler/SSR control:** `SSRPin` (D5) drives a solid-state relay. `D0` is a status LED mirroring the relay. State is tracked in `boiler_state` and `boiler_turned_on_timestamp` (both in `web_server.cpp`).

**Sensor data flow:** Each sensor callback reads hardware → updates current-value globals → pushes to a `CircularBuffer<T, 50>`. `getSensorReadings()` snapshots the current values into a JSON string sent via SSE (`/events`) and `/curr_readings`. Full buffer history available at `/all_samples` as CSV.

**OTA updates:** Served at `/update` via `AsyncElegantOTA`, protected by `http_username`/`http_password`.

### Hardware sensors

| Sensor | Interface | Globals |
|---|---|---|
| AHT21 | I2C | `temperature`, `humidity` |
| ENS160 | I2C | `eco2`, `tvoc`, `aqi` + `CO2_buffer`, `VOC_buffer`, `AQI_buffer` |
| BH1750 | I2C | `light_intensity`, `lightIntensity_buffer` |
| DS18B20 | D4 (OneWire) | `water_temperature`, `water_temp_buffer` |

### JSON keys from `getSensorReadings()`

`temperature`, `humidity`, `water_temperature`, `light`, `CO2`, `VOC`, `AQI`

### Network

Static IP: `192.168.3.44`, gateway `192.168.3.1`. Credentials in `pwd.h`. NTP: `pool.ntp.org` at GMT+2.
