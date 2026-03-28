#pragma once

#define DHTTYPE DHT22
#define NTP_TIMEOUT 5000
#define report_time_interval_address 0
#define ota_reboot_delay_address     10
#define sensor_enable_address        20
#define DEFAULT_SAMPLE_RATE 2000
#define SEALEVELPRESSURE_HPA (1013.25)

#define DHTPin       D3
#define ONE_WIRE_BUS D4
#define SSRPin       D5

// Sensor enable bitmask bits
#define SENSOR_AHT21_BIT   (1 << 0)   // 0x01 — temperature & humidity
#define SENSOR_ENS160_BIT  (1 << 1)   // 0x02 — AQI, VOC, CO2
#define SENSOR_BH1750_BIT  (1 << 2)   // 0x04 — light
#define SENSOR_BMP580_BIT  (1 << 3)   // 0x08 — pressure & BMP temp
#define SENSOR_DS18B20_BIT (1 << 4)   // 0x10 — water temperature
#define SENSOR_ALL_ENABLED 0x1F

extern boolean DEBUG;
