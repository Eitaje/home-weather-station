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

- Static IP and gateway are configured in `src/pwd.h` (not committed — see `src/pwd.h.example`)
- NTP: `pool.ntp.org` at GMT+2

## Build & Flash

This is a **PlatformIO** project.

### Build

```bash
pio run                 # compile firmware only
pio run -t upload       # compile + flash firmware
pio run -t uploadfs     # compile + upload LittleFS web UI files (index.html, script.js, style.css)
```

### Upload method 1 — Serial (USB cable)

Default method. Port is set to `COM12` in `platformio.ini`; change it to match your system.

```bash
pio run -t upload
```

> Close any serial monitor before uploading — it holds the COM port.

### Upload method 2 — Browser OTA (no cable required)

`AsyncElegantOTA` serves an upload page at `/update` (auth required).

1. Build the binary: `pio run`
2. Open `http://<DEVICE_IP>/update` in a browser and enter credentials
3. Upload `.pio/build/nodemcuv2/firmware.bin`

> For LittleFS (web UI files) there is no browser OTA equivalent — use `pio run -t uploadfs` over serial.

### Version

The firmware version is defined in `platformio.ini`:

```ini
build_flags = -D FIRMWARE_VERSION='"1.01"'
```

It is exposed at `/version` and displayed in the dashboard header.

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
| `/update` | OTA firmware update via browser (auth required) |
| `/version` | Firmware version string (plain text) |

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
