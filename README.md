# Eitaje Smart Home Weather Station

An ESP8266 NodeMCU v2-based WiFi weather station and boiler controller, serving a live dashboard with sensor gauges updated via Server-Sent Events (SSE).

## Hardware

| Sensor | Interface | Measures |
|---|---|---|
| AHT21 | I2C | Ambient temperature, humidity |
| ENS160 | I2C | eCO2 (ppm), TVOC (ppb), Air Quality Index (1–5) |
| BH1750 | I2C | Light intensity (lux) |
| DS18B20 | D4 (OneWire) | Water/tank temperature |
| SSR (solid-state relay) | D5 | Boiler on/off control |

## Network

- Static IP: `192.168.3.44`, gateway `192.168.3.1`
- NTP: `pool.ntp.org` at GMT+2
- Credentials stored in `src/pwd.h` (not committed)

## Build & Flash

This is a **PlatformIO** project.

```bash
# Build firmware
pio run

# Flash firmware
pio run -t upload

# Upload web UI files (LittleFS)
pio run -t uploadfs
```

The upload port is set to `COM12` in `platformio.ini` — change it to match your system.

Close any serial monitor before uploading (it holds the COM port).

## Web Endpoints

| Endpoint | Description |
|---|---|
| `/` | Live dashboard (gauges, boiler toggle) |
| `/curr_readings` | JSON snapshot of current sensor values |
| `/events` | SSE stream — pushes `new_readings` JSON on each sample |
| `/all_samples` | CSV of all buffered readings (last 50 per sensor) |
| `/button_state` | Boiler relay state + time on (plain text) |
| `/button_update?state=1\|0` | Toggle boiler relay |
| `/set_reporting_interval?sample_interval=N` | Set sampling interval in ms (auth required) |
| `/get_reporting_interval` | Read current sampling interval |
| `/update` | OTA firmware update (auth required) |

## Web UI

Static files served from LittleFS (`src/data/`):
- `index.html` — dashboard layout
- `script.js` — gauge init, SSE listener, polling
- `style.css` — styling
- `favicon.png` — icon

Gauges use the [canvas-gauges](https://canvas-gauges.com/) library (v2.1.7).

## Project Structure

```
src/
  main.cpp              # Entry point, Task objects, setup(), loop()
  config.h              # Pin assignments, constants
  sensors.h/cpp         # Sensor drivers, circular buffers, getSensorReadings()
  wifi_manager.h/cpp    # Static IP, initWiFi(), reconnect task
  web_server.h/cpp      # HTTP routes, SSE, boiler control, EEPROM
  html_ui.h/cpp         # Server-side HTML generation (legacy endpoints)
  pwd.h                 # WiFi credentials (not committed)
  data/                 # LittleFS web UI files
```

## Architecture Notes

- All periodic work (sampling, NTP, SSE push) runs via **TaskScheduler** — `loop()` only calls `runner.execute()`
- Sensor buffers hold the last 50 readings each (`CircularBuffer<T, 50>`)
- `getSensorReadings()` returns a JSON string with the latest values, keyed as: `temperature`, `humidity`, `water_temperature`, `light`, `CO2`, `VOC`, `AQI`
- Sampling interval is persisted to EEPROM and restored on boot
