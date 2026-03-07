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
extern CircularBuffer<unsigned long, 50> current_time_buffer;

// Global variables for sensor data
extern float temperature, humidity;
extern uint16_t eco2, tvoc;
extern uint8_t aqi;
extern float water_temperature;
extern float light_intensity;

void   initSensors();
String getSensorReadings();

void sample_sensor_ENS160_callback();
void sample_sensor_water_temp_callback();
void sample_sensor_BH1750_callback();
void sample_sensor_BMP388_callback();
void update_date_time_callback();
