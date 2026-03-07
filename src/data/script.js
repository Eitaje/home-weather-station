// Get current sensor readings when the page loads  
window.addEventListener('load', function () {
    // Increased interval slightly to allow animations to complete smoothly
    setInterval(getReadings, 2000); 
}, false);

// Create Water Temperature Gauge
var gaugeWaterTemp = new LinearGauge({
    renderTo: 'gauge-water-temperature',
    width: 120,
    height: 400,
    units: "Temperature C",
    minValue: 0,
    maxValue: 80,
    valueDec: 2,
    valueInt: 2,
    majorTicks: ["0", "10", "20", "30", "40", "50", "60", "70", "80"],
    minorTicks: 4,
    strokeTicks: true,
    highlights: [{ "from": 40, "to": 80, "color": "rgba(200, 20, 20, .75)" }],
    colorPlate: "#FFD",
    colorBarProgress: "#CC2936",
    colorBarProgressEnd: "#049faa",
    borderShadowWidth: 0,
    borders: true,
    needleType: "arrow",
    animationDuration: 1500,
    animationRule: "linear",
    barWidth: 10,
}).draw();

// Create Temperature Gauge - FIXED majorTicks and maxValue alignment
var gaugeTemp = new LinearGauge({
    renderTo: 'gauge-temperature',
    width: 120,
    height: 400,
    units: "Temperature C",
    minValue: 0,
    maxValue: 50,
    valueDec: 2,
    valueInt: 2,
    // Added "45" to make the scale mathematically linear for the needle
    majorTicks: ["0", "5", "10", "15", "20", "25", "30", "35", "40", "45", "50"], 
    minorTicks: 4,
    strokeTicks: true,
    highlights: [{ "from": 33, "to": 50, "color": "rgba(200, 50, 50, .75)" }],
    colorPlate: "#EEE",
    colorBarProgress: "#CC2936",
    animationDuration: 1500,
    animationRule: "linear",
    barWidth: 10,
}).draw();

// Create Humidity Gauge
var gaugeHum = new RadialGauge({
    renderTo: 'gauge-humidity',
    width: 250,
    height: 250,
    units: "Humidity (%)",
    minValue: 0,
    maxValue: 100,
    valueDec: 0,
    majorTicks: ["0", "20", "40", "60", "80", "100"],
    minorTicks: 4,
    strokeTicks: true,
    highlights: [{ "from": 60, "to": 100, "color": "#c0c0ff" }],
    colorPlate: "#fff",
    needleType: "line",
    colorNeedle: "#007F80",
    animationDuration: 1500,
    animationRule: "linear"
}).draw();

// Create Light Gauge
var gaugeLight = new RadialGauge({
    renderTo: 'gauge-light',
    width: 250,
    height: 250,
    units: "Lux",
    minValue: 0,
    maxValue: 1000,
    valueDec: 0,
    majorTicks: ["0", "200", "400", "600", "800", "1000"],
    minorTicks: 4,
    strokeTicks: true,
    highlights: [{ "from": 200, "to": 1000, "color": "#FFFFC0" }],
    colorPlate: "#fff",
    needleType: "line",
    colorNeedle: "#007F80",
    animationDuration: 1500,
    animationRule: "linear"
}).draw();

// Create CO2 Gauge for ENS160
var gaugeCO2 = new RadialGauge({
    renderTo: 'gauge-co2',
    width: 250,
    height: 250,
    units: "eCO2 (ppm)",
    minValue: 400,
    maxValue: 2000,
    colorValueBoxRect: "#049faa",
    colorValueBoxRectEnd: "#049faa",
    colorValueBoxBackground: "#f1fbfc",
    valueInt: 1,
    valueDec: 0,
    majorTicks: ["400", "800", "1200", "1600", "2000"],
    minorTicks: 4,
    strokeTicks: true,
    highlights: [
        { "from": 400,  "to": 1000, "color": "rgba(100, 255, 100, .3)" },
        { "from": 1000, "to": 1500, "color": "rgba(255, 255, 100, .3)" },
        { "from": 1500, "to": 2000, "color": "rgba(255, 100, 100, .3)" }
    ],
    colorPlate: "#fff",
    borderShadowWidth: 0,
    borders: false,
    needleType: "line",
    colorNeedle: "#007F80",
    animationDuration: 1500,
    animationRule: "linear"
}).draw();

// Create VOC Gauge for ENS160
var gaugeVOC = new RadialGauge({
    renderTo: 'gauge-voc',
    width: 250,
    height: 250,
    units: "TVOC (ppb)",
    minValue: 0,
    maxValue: 500,
    valueDec: 0,
    majorTicks: ["0", "100", "200", "300", "400", "500"],
    minorTicks: 4,
    strokeTicks: true,
    highlights: [
        { "from": 0,   "to": 150, "color": "rgba(100, 255, 100, .3)" },
        { "from": 150, "to": 350, "color": "rgba(255, 255, 100, .3)" },
        { "from": 350, "to": 500, "color": "rgba(255, 100, 100, .3)" }
    ],
    colorPlate: "#fff",
    borderShadowWidth: 0,
    borders: false,
    needleType: "line",
    colorNeedle: "#007F80",
    animationDuration: 1500,
    animationRule: "linear"
}).draw();

// Create AQI Gauge for ENS160 (1=Excellent, 2=Good, 3=Moderate, 4=Poor, 5=Unhealthy)
var gaugeAQI = new RadialGauge({
    renderTo: 'gauge-aqi',
    width: 250,
    height: 250,
    units: "AQI (1-5)",
    minValue: 0,
    maxValue: 5,
    valueDec: 0,
    majorTicks: ["0", "1", "2", "3", "4", "5"],
    minorTicks: 0,
    strokeTicks: true,
    highlights: [
        { "from": 1, "to": 2, "color": "rgba(100, 255, 100, .5)" },
        { "from": 2, "to": 3, "color": "rgba(180, 255, 100, .5)" },
        { "from": 3, "to": 4, "color": "rgba(255, 255, 100, .5)" },
        { "from": 4, "to": 5, "color": "rgba(255, 100, 100, .5)" }
    ],
    colorPlate: "#fff",
    borderShadowWidth: 0,
    borders: false,
    needleType: "line",
    colorNeedle: "#007F80",
    animationDuration: 1500,
    animationRule: "linear"
}).draw();

// Update function for initial load or polling
function getReadings(){
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var myObj = JSON.parse(this.responseText);
            console.log("Initial Load:", myObj);
            
            // Setting the .value property triggers the draw/animation automatically
            if(myObj.water_temperature !== undefined) gaugeWaterTemp.value = Number(myObj.water_temperature);
            if(myObj.humidity !== undefined) gaugeHum.value = Number(myObj.humidity);
            if(myObj.temperature !== undefined) gaugeTemp.value = Number(myObj.temperature);
            if(myObj.light !== undefined) gaugeLight.value = Number(myObj.light);
            if(myObj.CO2 !== undefined) gaugeCO2.value = Number(myObj.CO2);
            if(myObj.VOC !== undefined) gaugeVOC.value = Number(myObj.VOC);
            if(myObj.AQI !== undefined) gaugeAQI.value = Number(myObj.AQI);
        }
    }; 
    xhr.open("GET", "/curr_readings", true);
    xhr.send();
}

// Server-Sent Events Listener
if (!!window.EventSource) {
    var source = new EventSource('/events');
    
    source.addEventListener('open', function(e) { console.log("Events Connected"); }, false);
    source.addEventListener('error', function(e) {
        if (e.target.readyState != EventSource.OPEN) console.log("Events Disconnected");
    }, false);
    
    // FIXED: Added missing gauge updates to the event listener
    source.addEventListener('new_readings', function(e) {
        console.log("new_readings", e.data);
        var myObj = JSON.parse(e.data);
        
        if(myObj.water_temperature !== undefined) gaugeWaterTemp.value = Number(myObj.water_temperature);
        if(myObj.temperature !== undefined) gaugeTemp.value = Number(myObj.temperature);
        if(myObj.humidity !== undefined) gaugeHum.value = Number(myObj.humidity);
        if(myObj.light !== undefined) gaugeLight.value = Number(myObj.light);
        if(myObj.CO2 !== undefined) gaugeCO2.value = Number(myObj.CO2);
        if(myObj.VOC !== undefined) gaugeVOC.value = Number(myObj.VOC);
        if(myObj.AQI !== undefined) gaugeAQI.value = Number(myObj.AQI);
    }, false);
}