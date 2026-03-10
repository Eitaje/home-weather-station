#pragma once

#define DHTTYPE DHT22
#define NTP_TIMEOUT 5000
#define report_time_interval_address 0
#define ota_reboot_delay_address     10
#define DEFAULT_SAMPLE_RATE 2000
#define SEALEVELPRESSURE_HPA (1013.25)

#define DHTPin       D3
#define ONE_WIRE_BUS D4
#define SSRPin       D5

extern boolean DEBUG;
