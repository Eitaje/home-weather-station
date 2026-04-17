# Eitaje Smart Home Weather Station

An ESP8266 NodeMCU v2-based WiFi weather station and boiler controller, serving a live dashboard with sensor gauges updated via Server-Sent Events (SSE).

## Hardware

| Sensor | Interface | Measures |
|---|---|---|
| AHT21 | I2C | Ambient temperature, humidity |
| ENS160 | I2C | eCO2 (ppm), TVOC (ppb), Air Quality Index (1–5) |
| BH1750 | I2C | Light intensity (lux) |
| BMP580 | I2C | Barometric pressure, temperature |
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

### Upload method 3 — curl OTA for LittleFS web UI files (no cable required)

The `/upload` endpoint accepts individual files via `multipart/form-data` POST.
Upload each file you changed from `src/data/`:

**PowerShell (Windows)** — use `curl.exe` explicitly (`curl` alone is an alias for `Invoke-WebRequest`):

```powershell
curl.exe -u "WaterUser:<password>" -F "file=@src/data/index.html;filename=/index.html" http://<DEVICE_IP>/upload
curl.exe -u "WaterUser:<password>" -F "file=@src/data/script.js;filename=/script.js"   http://<DEVICE_IP>/upload
curl.exe -u "WaterUser:<password>" -F "file=@src/data/style.css;filename=/style.css"   http://<DEVICE_IP>/upload
```

**bash / Git Bash / WSL:**

```bash
for f in src/data/index.html src/data/script.js src/data/style.css src/data/favicon.png; do
  curl -u WaterUser:'<password>' \
       -F "file=@$f;filename=/$(basename $f)" \
       http://<DEVICE_IP>/upload \
  && echo "  → uploaded $f"
done
```

> No device reset is needed after a file upload; the next page load serves the new file.

### Version

The firmware version is defined in `platformio.ini`:

```ini
build_flags =
    -D FIRMWARE_VERSION='"1.07"'
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
| `/hourly_samples` | CSV of hourly-averaged readings (last 48 hours) |
| `/button_state` | Boiler relay state + time on (plain text) |
| `/button_update?state=1\|0` | Toggle boiler relay |
| `/set_reporting_interval?sample_interval=N` | Set sampling interval in ms (persisted to EEPROM) |
| `/get_reporting_interval` | Read current sampling interval in ms |
| `/ota_delay` | Read current OTA reboot delay in seconds |
| `/set_ota_delay?delay=N` | Set OTA reboot delay in seconds, 5–300 (auth required, persisted to EEPROM) |
| `/get_sensor_enabled` | Read sensor enable bitmask (plain text integer) |
| `/set_sensor_enabled?mask=N` | Set sensor enable bitmask (persisted to EEPROM; reset required) |
| `/upload` | Upload a file to LittleFS via multipart POST (auth required) |
| `/update` | OTA firmware update page (auth required) |
| `/reset` | Trigger a software reset |
| `/version` | Firmware version string (plain text) |

## Web UI

Three-tab dashboard served from LittleFS (`src/data/`):

- **Live** — real-time sensor gauges (temperature, humidity, light, CO₂, VOC, AQI) and boiler toggle
- **History** — Chart.js line charts for all sensors with optional moving-average overlay
- **Config** — per-sensor enable/disable toggles (persisted, reset required), sampling rate selector, and OTA reboot delay setting

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

## Tests

Automated firmware unit tests are not set up (PlatformIO's `test/` folder is the default empty placeholder). All pipeline testing for this device lives in the companion server project's test suite — see `tests/test_history_pipeline.py` in the home automation server repository.

### What is covered on the server side

The server tests exercise the full path from NodeMCU JSON output → Redis storage → history API → UI-parseable response, including edge cases that arise specifically after a firmware deploy:

- History endpoint returns the N **most recent** readings (not oldest)
- All 9 sensor fields (`temperature`, `humidity`, `water_temperature`, `light`, `CO2`, `VOC`, `AQI`, `pressure`, `temperature_bmp580`) survive the round-trip
- ENS160 warm-up: AQI/CO2/VOC `= "0"` are returned as numeric zero, not absent
- Old Redis entries missing `temperature_bmp580` don't crash the endpoint
- Offline buffer sync with past timestamps (device rebooted while online) does not return HTTP 500

### Manual smoke tests after flashing

After flashing new firmware, verify the following endpoints directly:

| Check | Command |
|---|---|
| Sensor values look sane | `curl http://<DEVICE_IP>/curr_readings` |
| ENS160 initialised | `curl http://<DEVICE_IP>/sensor_status` — `ens160` should be `"ok"`, not `"fault"` |
| WiFi reconnect log | Open serial monitor, cut power briefly, confirm `[wifi] connected!` appears within ~30 s |
| Offline buffer sync | Go offline for >1 min, reconnect, check server logs for `[offline] syncing` |

---

## Architecture Notes

- All periodic work (sampling, NTP, SSE push) runs via **TaskScheduler** — `loop()` only calls `runner.execute()`
- Sensor buffers hold the last 50 readings each (`CircularBuffer<T, 50>`)
- `getSensorReadings()` returns a JSON string with the latest values, keyed as: `temperature`, `humidity`, `water_temperature`, `light`, `CO2`, `VOC`, `AQI`, `timestamp`
- Sampling interval is persisted to EEPROM (address 0) and restored on boot; changing it via the UI updates all sensor tasks and the SSE push task immediately
- OTA reboot delay is persisted to EEPROM (address 10) and restored on boot; default comes from the `OTA_REBOOT_DELAY` build flag
- Sensor enable bitmask is persisted to EEPROM (address 20); bits 0–4 map to AHT21, ENS160, BH1750, BMP580, DS18B20; changes take effect on next boot
