#pragma once

#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;
extern AsyncEventSource events;
extern boolean boiler_state;
extern unsigned long boiler_turned_on_timestamp;

void initWebServer();
void send_new_readings_event_callback();
void readSamplingFreqParamer();
