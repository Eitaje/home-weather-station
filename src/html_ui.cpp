#include "html_ui.h"
#include "sensors.h"
#include "web_server.h"


String SendHTML(String str) {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>EITAJE-Weather-Station-Server</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #1abc9c;}\n";
  ptr += ".button-on:active {background-color: #16a085;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>ESP8266 Web Server</h1>\n";
  ptr += "<h3>Using Access Point(AP) Mode</h3>\n";
  ptr += "<h1>" + str + " </h1>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

String prepareReportHTML() {
  char Humiditystat_str[10];
  char Temperaturestat_str[10];
  char water_temp_stat_str[10];
  char light_intensity_stat_str[10];
  char co2_stat_str[10];

  dtostrf(humidity,         5, 1, Humiditystat_str);
  dtostrf(temperature,       5, 1, Temperaturestat_str);
  dtostrf(water_temperature, 5, 1, water_temp_stat_str);
  dtostrf(light_intensity,   5, 1, light_intensity_stat_str);
  dtostrf(eco2,             5, 0, co2_stat_str); // CO2 is usually a whole number

  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>ESP8266 Weather Report</title>\n";
  
  // Scripts for Gauges and Plotly
  ptr += "<script src=\"//cdn.rawgit.com/Mikhus/canvas-gauges/master/smart-gauges.min.js\"></script>\n";
  ptr += "<script src=\"https://cdn.plot.ly/plotly-latest.min.js\"></script>\n";
  
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px; background-color: #f7f7f7;} \n";
  ptr += ".container { display: flex; flex-wrap: wrap; justify-content: center; align-items: center; gap: 20px; padding: 20px; }\n";
  ptr += ".card { background: white; border-radius: 10px; padding: 20px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); min-width: 150px; }\n";
  ptr += "h1 {color: #444444;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  
  ptr += "<body>\n";
  ptr += "<h1>Eitaje Weather Report</h1>\n";

  // --- START OF GAUGE SECTION ---
  ptr += "<div class=\"container\">\n";
  
  // Tank Temperature Gauge
  ptr += "<div class=\"card\"><h3>Tank Temp</h3><canvas id=\"gauge-water-temperature\"></canvas></div>\n";
  
  // Outside Temperature Gauge
  ptr += "<div class=\"card\"><h3>Outside Temp</h3><canvas id=\"gauge-temperature\"></canvas></div>\n";
  
  // Humidity Gauge
  ptr += "<div class=\"card\"><h3>Humidity</h3><canvas id=\"gauge-humidity\"></canvas></div>\n";
  
  // Light Gauge (New)
  ptr += "<div class=\"card\"><h3>Light (Lux)</h3><canvas id=\"gauge-light\"></canvas></div>\n";
  
  // CO2 Gauge (New)
  ptr += "<div class=\"card\"><h3>Air Quality (CO2)</h3><canvas id=\"gauge-co2\"></canvas></div>\n";
  
  ptr += "</div>\n";
  // --- END OF GAUGE SECTION ---

  ptr += "<div id=\"myPlot\" style=\"width:100%;max-width:700px;margin:auto;\"></div>\n";

  // The actual sensor logic script (assuming this is kept in a separate .js file or further down)
  ptr += "<script>\n";
  ptr += "function toggleCheckbox(element) {\n";
  ptr += "var xhr = new XMLHttpRequest();\n";
  ptr += "if(element.checked){ xhr.open(\"GET\", \"/button_update?state=1\", true); }\n";
  ptr += "else { xhr.open(\"GET\", \"/button_update?state=0\", true); }\n";
  ptr += "xhr.send();}\n";
  ptr += "</script>\n"; // Fixed missing closing script tag

  // Static Plotly Logic
  ptr += "<script>\n";
  ptr += "var xArray = [50,60,70,80,90,100,110,120,130,140,150];\n";
  ptr += "var yArray = [7,8,8,9,9,9,10,11,14,14,15];\n";
  ptr += "var data = [{x: xArray, y: yArray, mode:\"lines\"}];\n";
  ptr += "var layout = {title: \"Measured:\"};\n";
  ptr += "Plotly.newPlot(\"myPlot\", data, layout);\n";
  ptr += "</script>\n";

  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

String prepareReportMultiLines() {
  char Humiditystat_str[10];
  char Temperaturestat_str[10];
  char water_temp_stat_str[10];
  char light_intensity_stat_str[10];
  char co2_stat_str[10];

  dtostrf(humidity,          5, 1, Humiditystat_str);
  dtostrf(temperature,       5, 1, Temperaturestat_str);
  dtostrf(water_temperature, 5, 1, water_temp_stat_str);
  dtostrf(light_intensity,   5, 1, light_intensity_stat_str);
  dtostrf(eco2,             5, 0, co2_stat_str);

  // --- Buffer to String conversions for Chart 1 (Environmental) ---
  String str_xValues = "[";
  for (int i = 0; i < temperature_buffer.size(); i++) {
    if (i > 0) str_xValues += ",";
    str_xValues += String(i);
  }
  str_xValues += "]";

  String str_data_temperature = "[";
  for (int i = 0; i < temperature_buffer.size(); i++) {
    if (i > 0) str_data_temperature += ",";
    str_data_temperature += String(temperature_buffer[i]);
  }
  str_data_temperature += "]";

  String str_data_humidity = "[";
  for (int i = 0; i < humidity_buffer.size(); i++) {
    if (i > 0) str_data_humidity += ",";
    str_data_humidity += String(humidity_buffer[i]);
  }
  str_data_humidity += "]";

  // --- Buffer to String conversions for Chart 2 (AQI/Light) ---
  // Note: Ensure you have added co2_buffer and lux_buffer to your sensors.cpp/h
  String str_data_eco2 = "[";
  for (int i = 0; i < CO2_buffer.size(); i++) {
    if (i > 0) str_data_eco2 += ",";
    str_data_eco2 += String(CO2_buffer[i]);
  }
  str_data_eco2 += "]";

  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head>\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>ESP8266 Weather Report</title>\n";
  
  // Gauges Library (Required for the gauges to draw)
  ptr += "<script src=\"https://cdn.rawgit.com/Mikhus/canvas-gauges/master/smart-gauges.min.js\"></script>\n";
  ptr += "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.5.0/Chart.min.js\"></script>\n";

  ptr += "<style>\n";
  ptr += "html {font-family: Arial; display: inline-block; text-align: center;}\n";
  ptr += "body {max-width: 800px; margin:0px auto; padding-bottom: 25px; background-color: #f4f4f4;}\n";
  ptr += ".gauge-container { display: flex; flex-wrap: wrap; justify-content: center; gap: 15px; }\n";
  ptr += ".card { background: white; padding: 15px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin: 10px; }\n";
  ptr += ".switch {position: relative; display: inline-block; width: 80px; height: 40px} \n";
  ptr += ".switch input {display: none}\n";
  ptr += ".slider {position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 20px}\n";
  ptr += ".slider:before {position: absolute; content: \"\"; height: 32px; width: 32px; left: 4px; bottom: 4px; background-color: #fff; transition: .4s; border-radius: 50%}\n";
  ptr += "input:checked+.slider {background-color: #2196F3}\n";
  ptr += "input:checked+.slider:before {transform: translateX(40px)}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";

  ptr += "<body>\n";
  ptr += "<h1>Eitaje Weather Report</h1>\n";

  // Boiler Control
  ptr += "<div class=\"card\">";
  ptr += "<h4>Boiler State: <span id=\"outputState\"></span></h4>\n";
  ptr += "<label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + String(boiler_state ? "checked" : "") + "><span class=\"slider\"></span></label>\n";
  ptr += "</div>\n";

  // Gauges Section
  ptr += "<div class=\"gauge-container\">\n";
  ptr += "<div class=\"card\"><h4>Tank</h4><canvas id=\"gauge-water-temperature\"></canvas></div>\n";
  ptr += "<div class=\"card\"><h4>Outside</h4><canvas id=\"gauge-temperature\"></canvas></div>\n";
  ptr += "<div class=\"card\"><h4>Humidity</h4><canvas id=\"gauge-humidity\"></canvas></div>\n";
  ptr += "<div class=\"card\"><h4>Light</h4><canvas id=\"gauge-light\"></canvas></div>\n";
  ptr += "<div class=\"card\"><h4>CO2</h4><canvas id=\"gauge-co2\"></canvas></div>\n";
  ptr += "</div>\n";

  // Text Report
  ptr += "<div class=\"card\">\n";
  ptr += "<p style=\"font-size:18px;\"><b>Ambient:</b> " + String(Temperaturestat_str) + "°C | " + String(Humiditystat_str) + "%</p>\n";
  ptr += "<p style=\"font-size:18px;\"><b>Air Quality:</b> " + String(co2_stat_str) + " ppm CO2</p>\n";
  ptr += "</div>\n";

  // Charts
  ptr += "<div class=\"card\"><canvas id=\"envChart\"></canvas></div>\n";
  ptr += "<div class=\"card\"><canvas id=\"airChart\"></canvas></div>\n";

  // JavaScript for Toggle and Gauges Logic
  ptr += "<script>\n";
  ptr += "function toggleCheckbox(element) { var xhr = new XMLHttpRequest(); xhr.open(\"GET\", \"/button_update?state=\" + (element.checked ? \"1\":\"0\"), true); xhr.send(); }\n";
  
  // Include your full Gauges Javascript file here (the one we fixed earlier)
  // ... [Insert the Gauge initialization code from previous step here] ...

  // Chart 1: Temperature/Humidity
  ptr += "new Chart(\"envChart\", {type: \"line\", data: {labels: " + str_xValues + ", datasets: [";
  ptr += "{label: 'Temp', data: " + str_data_temperature + ", borderColor: \"red\", fill: false},";
  ptr += "{label: 'Humidity', data: " + str_data_humidity + ", borderColor: \"blue\", fill: false}]}, options: {title: {display: true, text: 'Environment'}}});\n";

  // Chart 2: CO2
  ptr += "new Chart(\"airChart\", {type: \"line\", data: {labels: " + str_xValues + ", datasets: [";
  ptr += "{label: 'eCO2', data: " + str_data_eco2 + ", borderColor: \"green\", fill: false}]}, options: {title: {display: true, text: 'Air Quality (PPM)'}}});\n";
  
  ptr += "</script>\n";
  ptr += "</body></html>\n";
  return ptr;
}
