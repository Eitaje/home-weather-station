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

> When changing only web UI files (`src/data/`), only `uploadfs` is needed.
> When changing firmware (`.cpp`/`.h`/`platformio.ini`), run `upload`. Both are independent.

### Upload method 1 — Serial (USB cable)

Default method. Port is configured in `platformio.ini`; change it to match your system.

```bash
pio run -t upload
```

> Close any serial monitor before uploading — it holds the COM port.

### Upload method 2 — Browser OTA (no cable required)

A custom OTA page is served at `/update` (auth required).

1. Build the binary: `pio run`
2. Open `http://<DEVICE_IP>/update` in a browser and enter credentials
3. Select `.pio/build/nodemcuv2/firmware.bin` and click Upload
4. The page computes the file MD5 automatically, then shows upload progress
5. After upload completes, a countdown runs and the browser redirects to `/`

> For LittleFS (web UI files) there is no browser OTA equivalent — use `pio run -t uploadfs` over serial.

### Version

The firmware version is defined in `platformio.ini`:

```ini
build_flags =
    -D FIRMWARE_VERSION='"1.05"'
    -D OTA_REBOOT_DELAY=5
```

`FIRMWARE_VERSION` is exposed at `/version` and displayed in the dashboard header.

`OTA_REBOOT_DELAY` sets the default countdown (seconds) shown after a successful OTA upload before the browser redirects to `/`. It can also be changed at runtime from the **Config** tab in the dashboard UI and is persisted to EEPROM.

## Web Endpoints

| Endpoint | Description |
|---|---|
| `/` | Live dashboard (gauges, boiler toggle) |
| `/curr_readings` | JSON snapshot of current sensor values + `timestamp` |
| `/events` | SSE stream — pushes `new_readings` JSON on each sample |
| `/all_samples` | CSV of all buffered readings (last 50 per sensor) |
| `/button_state` | Boiler relay state + time on (plain text) |
| `/button_update?state=1\|0` | Toggle boiler relay |
| `/set_reporting_interval?sample_interval=N` | Set sampling interval in ms (persisted to EEPROM) |
| `/get_reporting_interval` | Read current sampling interval in ms |
| `/ota_delay` | Read current OTA reboot delay in seconds |
| `/set_ota_delay?delay=N` | Set OTA reboot delay in seconds, 5–300 (auth required, persisted to EEPROM) |
| `/update` | OTA firmware update page (auth required) |
| `/version` | Firmware version string (plain text) |

## Web UI

Three-tab dashboard served from LittleFS (`src/data/`):

- **Live** — real-time sensor gauges (temperature, humidity, light, CO₂, VOC, AQI) and boiler toggle
- **History** — Chart.js line charts for all sensors with optional moving-average overlay
- **Config** — sampling rate selector and OTA reboot delay setting

Static files:
- `index.html` — dashboard layout and tab structure
- `script.js` — gauge init, SSE listener, chart rendering, polling, config
- `style.css` — styling
- `favicon.png` — icon
- `update.html` — OTA firmware upload page

Gauges use the [canvas-gauges](https://canvas-gauges.com/) library (v2.1.7).
Charts use [Chart.js](https://www.chartjs.org/) v4.4.0.

## Project Structure

```
src/
  main.cpp              # Entry point, Task objects, setup(), loop()
  config.h              # Pin assignments, constants, EEPROM addresses
  sensors.h/cpp         # Sensor drivers, circular buffers, getSensorReadings()
  wifi_manager.h/cpp    # Static IP, initWiFi(), reconnect task
  web_server.h/cpp      # HTTP routes, SSE, boiler control, EEPROM persistence
  html_ui.h/cpp         # Server-side HTML generation (legacy endpoints)
  pwd.h                 # WiFi credentials + network config (not committed)
  pwd.h.example         # Template for pwd.h
  data/                 # LittleFS web UI files
```

## Architecture Notes

- All periodic work (sampling, NTP, SSE push) runs via **TaskScheduler** — `loop()` only calls `runner.execute()`
- Sensor buffers hold the last 50 readings each (`CircularBuffer<T, 50>`)
- `getSensorReadings()` returns a JSON string with the latest values, keyed as: `temperature`, `humidity`, `water_temperature`, `light`, `CO2`, `VOC`, `AQI`, `timestamp`
- Sampling interval is persisted to EEPROM (address 0) and restored on boot; changing it via the UI updates all sensor tasks and the SSE push task immediately
- OTA reboot delay is persisted to EEPROM (address 10) and restored on boot; default comes from the `OTA_REBOOT_DELAY` build flag
