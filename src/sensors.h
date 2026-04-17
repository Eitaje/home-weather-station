#pragma once

#include <CircularBuffer.h>
#include <Arduino.h>

extern CircularBuffer<float, 50>         temperature_buffer;
extern CircularBuffer<uint8_t, 50>       AQI_buffer;
extern CircularBuffer<uint16_t, 50>      VOC_buffer;
extern CircularBuffer<uint16_t, 50>      CO2_buffer;
extern CircularBuffer<float, 50>         humidity_buffer;
extern CircularBuffer<float, 50>         water_temp_buffer;
extern CircularBuffer<float, 50>         lightIntensity_buffer;
extern CircularBuffer<float, 50>         pressure_buffer;
extern CircularBuffer<float, 50>         bmp580_temp_buffer;
extern CircularBuffer<unsigned long, 50> current_time_buffer;

// Hourly aggregation buffers (48 hours)
extern CircularBuffer<float, 48>         hourly_temperature_buffer;
extern CircularBuffer<float, 48>         hourly_humidity_buffer;
extern CircularBuffer<float, 48>         hourly_water_temp_buffer;
extern CircularBuffer<float, 48>         hourly_light_buffer;
extern CircularBuffer<float, 48>         hourly_AQI_buffer;
extern CircularBuffer<float, 48>         hourly_VOC_buffer;
extern CircularBuffer<float, 48>         hourly_CO2_buffer;
extern CircularBuffer<float, 48>         hourly_pressure_buffer;
extern CircularBuffer<float, 48>         hourly_bmp580_temp_buffer;
extern CircularBuffer<unsigned long, 48> hourly_time_buffer;

// Global variables for sensor data
extern float temperature, humidity;
extern uint16_t eco2, tvoc;
extern uint8_t aqi;
extern float water_temperature;
extern float light_intensity;
extern float bmp580_pressure;
extern float temperature_bmp580;

extern uint8_t sensor_enabled_mask;

void          initSensors();
String        getSensorReadings();
String        getSensorStatus();
unsigned long getApproxUnixTime();   // last NTP time + elapsed millis

void sample_sensor_ENS160_callback();
void sample_sensor_water_temp_callback();
void sample_sensor_BH1750_callback();
void sample_sensor_BMP580_callback();
void update_date_time_callback();
void aggregate_hourly_callback();
