// ── Clock ────────────────────────────────────────────────────────────────────
(function clock() {
    var el = document.getElementById('headerClock');
    function tick() {
        if (el) el.textContent = new Date().toLocaleTimeString([], {hour:'2-digit', minute:'2-digit'});
    }
    tick();
    setInterval(tick, 30000);
})();

// ── Boiler polling ───────────────────────────────────────────────────────────
setInterval(function() {
    var xhr = new XMLHttpRequest();
    xhr.timeout = 2500;
    xhr.ontimeout = function() {
        document.getElementById('serverState').textContent = 'offline';
        document.getElementById('statusDot').className = 'status-dot offline';
    };
    xhr.onreadystatechange = function() {
        if (this.readyState === 4) {
            document.getElementById('serverState').textContent = 'online';
            document.getElementById('statusDot').className = 'status-dot online';
            if (this.status === 200) {
                var parts = this.responseText.split(',');
                var on = parts[0] == '1';
                document.getElementById('output').checked = on;
                document.getElementById('outputState').textContent = on
                    ? 'ON  —  ' + parseFloat(parts[1]).toFixed(1) + ' hr'
                    : 'OFF';
            }
        }
    };
    xhr.open('GET', '/button_state', true);
    xhr.send();
}, 1500);

function toggleCheckbox(el) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/button_update?state=' + (el.checked ? '1' : '0'), true);
    xhr.send();
}

// ── Shared dark gauge defaults ────────────────────────────────────────────────
var darkRadial = {
    width: 240, height: 240,
    colorPlate: '#ffffff',
    colorPlateEnd: '#ffffff',
    colorBorderShadow: 'transparent',
    colorBorderOuter: '#ffffff',
    colorBorderOuterEnd: '#ffffff',
    colorBorderMiddle: '#ffffff',
    colorBorderMiddleEnd: '#ffffff',
    colorBorderInner: '#ffffff',
    colorBorderInnerEnd: '#ffffff',
    borderShadowWidth: 0,
    borders: false,
    colorNeedle: '#0891b2',
    colorNeedleEnd: '#0891b2',
    colorNeedleShadowUp: 'transparent',
    colorNeedleShadowDown: 'transparent',
    needleType: 'line',
    needleWidth: 2,
    colorMajorTicks: '#94a3b8',
    colorMinorTicks: '#cbd5e1',
    colorNumbers: '#64748b',
    colorTitle: 'transparent',
    colorUnits: 'transparent',
    colorValueBoxBackground: '#ffffff',
    colorValueBoxRect: '#ffffff',
    colorValueBoxRectEnd: '#ffffff',
    colorValueBoxShadow: 'transparent',
    valueBoxStroke: 0,
    colorValueText: '#0f172a',
    fontValueSize: 34,
    fontValue: 'JetBrains Mono',
    animationDuration: 1200,
    animationRule: 'linear',
};

var darkLinear = {
    width: 120, height: 360,
    colorPlate: '#ffffff',
    colorPlateEnd: '#ffffff',
    colorBarBackground: '#e2e8f0',
    colorBar: '#0891b2',
    colorBarProgress: '#0891b2',
    colorBarProgressEnd: '#059669',
    colorBarStroke: 'transparent',
    borderShadowWidth: 0,
    borders: false,
    colorMajorTicks: '#94a3b8',
    colorMinorTicks: '#cbd5e1',
    colorNumbers: '#64748b',
    colorTitle: 'transparent',
    colorUnits: 'transparent',
    colorValueBoxBackground: '#ffffff',
    colorValueBoxRect: '#ffffff',
    colorValueBoxRectEnd: '#ffffff',
    colorValueBoxShadow: 'transparent',
    valueBoxStroke: 0,
    colorValueText: '#0f172a',
    fontValueSize: 30,
    fontValue: 'JetBrains Mono',
    needleType: 'arrow',
    colorNeedle: '#0891b2',
    colorNeedleEnd: '#059669',
    animationDuration: 1200,
    animationRule: 'linear',
    barWidth: 8,
    strokeTicks: true,
};

// ── Gauge definitions ─────────────────────────────────────────────────────────
var gaugeWaterTemp = new LinearGauge(Object.assign({}, darkLinear, {
    renderTo: 'gauge-water-temperature',
    minValue: 0, maxValue: 80,
    valueDec: 1, valueInt: 2,
    majorTicks: ['0','10','20','30','40','50','60','70','80'],
    minorTicks: 4,
    highlights: [{ from: 40, to: 80, color: 'rgba(239,68,68,0.25)' }],
})).draw();

var gaugeTemp = new LinearGauge(Object.assign({}, darkLinear, {
    renderTo: 'gauge-temperature',
    minValue: 0, maxValue: 50,
    valueDec: 1, valueInt: 2,
    majorTicks: ['0','10','20','30','40','50'],
    minorTicks: 4,
    highlights: [{ from: 33, to: 50, color: 'rgba(239,68,68,0.25)' }],
})).draw();

var gaugeHum = new RadialGauge(Object.assign({}, darkRadial, {
    renderTo: 'gauge-humidity',
    minValue: 0, maxValue: 100,
    valueDec: 0,
    majorTicks: ['0','20','40','60','80','100'],
    minorTicks: 4,
    strokeTicks: true,
    highlights: [{ from: 60, to: 100, color: 'rgba(6,182,212,0.2)' }],
})).draw();

var gaugeLight = new RadialGauge(Object.assign({}, darkRadial, {
    renderTo: 'gauge-light',
    minValue: 0, maxValue: 1000,
    valueDec: 0,
    majorTicks: ['0','200','400','600','800','1000'],
    minorTicks: 4,
    strokeTicks: true,
    highlights: [{ from: 200, to: 1000, color: 'rgba(245,158,11,0.15)' }],
})).draw();

var gaugeCO2 = new RadialGauge(Object.assign({}, darkRadial, {
    renderTo: 'gauge-co2',
    minValue: 400, maxValue: 2000,
    valueDec: 0, valueInt: 1,
    majorTicks: ['400','800','1200','1600','2000'],
    minorTicks: 4,
    strokeTicks: true,
    highlights: [
        { from: 400,  to: 1000, color: 'rgba(16,185,129,0.2)' },
        { from: 1000, to: 1500, color: 'rgba(245,158,11,0.2)' },
        { from: 1500, to: 2000, color: 'rgba(239,68,68,0.2)'  },
    ],
})).draw();

var gaugeVOC = new RadialGauge(Object.assign({}, darkRadial, {
    renderTo: 'gauge-voc',
    minValue: 0, maxValue: 500,
    valueDec: 0,
    majorTicks: ['0','100','200','300','400','500'],
    minorTicks: 4,
    strokeTicks: true,
    highlights: [
        { from: 0,   to: 150, color: 'rgba(16,185,129,0.2)' },
        { from: 150, to: 350, color: 'rgba(245,158,11,0.2)' },
        { from: 350, to: 500, color: 'rgba(239,68,68,0.2)'  },
    ],
})).draw();

var gaugeAQI = new RadialGauge(Object.assign({}, darkRadial, {
    renderTo: 'gauge-aqi',
    minValue: 0, maxValue: 5,
    valueDec: 0,
    majorTicks: ['0','1','2','3','4','5'],
    minorTicks: 0,
    strokeTicks: true,
    highlights: [
        { from: 1, to: 2, color: 'rgba(16,185,129,0.3)'  },
        { from: 2, to: 3, color: 'rgba(132,204,22,0.3)'  },
        { from: 3, to: 4, color: 'rgba(245,158,11,0.3)'  },
        { from: 4, to: 5, color: 'rgba(239,68,68,0.3)'   },
    ],
})).draw();

// ── Tab switching ─────────────────────────────────────────────────────────────
function switchTab(name, btn) {
    document.getElementById('tab-live').style.display    = name === 'live'    ? '' : 'none';
    document.getElementById('tab-history').style.display = name === 'history' ? '' : 'none';
    document.getElementById('tab-config').style.display  = name === 'config'  ? '' : 'none';
    document.querySelectorAll('.tab-btn').forEach(function(b) { b.classList.remove('active'); });
    btn.classList.add('active');
    if (name === 'history') {
        loadCharts();
        startHistoryPolling(currentIntervalMs);
    } else {
        stopHistoryPolling();
    }
}

// ── Chart.js shared defaults ──────────────────────────────────────────────────
var chartDefaults = {
    type: 'line',
    options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: false,
        plugins: { legend: { display: false } },
        scales: {
            x: {
                title: { display: true, text: 'Sample #', color: '#94a3b8', font: { size: 11 } },
                ticks: { color: '#94a3b8', font: { size: 10 } },
                grid:  { color: '#e2e8f0' },
            },
            y: {
                ticks: { color: '#64748b', font: { size: 10 } },
                grid:  { color: '#e2e8f0' },
            }
        }
    }
};

function makeDataset(label, data, color) {
    return {
        label: label,
        data: data,
        borderColor: color,
        backgroundColor: color.replace(')', ', 0.08)').replace('rgb', 'rgba'),
        borderWidth: 2,
        pointRadius: 2,
        tension: 0.3,
        fill: true,
    };
}

// Chart instances (created once, updated on refresh)
var charts = {};

// ── Moving average ─────────────────────────────────────────────────────────────
var showMA = true;
var maWindow = 5;

function movingAverage(data, n) {
    return data.map(function(_, i) {
        if (i < n - 1) return null;
        var sum = 0;
        for (var j = i - n + 1; j <= i; j++) sum += data[j];
        return parseFloat((sum / n).toFixed(2));
    });
}

function makeMADataset(label, data, color) {
    return {
        label: label,
        data: movingAverage(data, maWindow),
        borderColor: color,
        backgroundColor: 'transparent',
        borderWidth: 1.5,
        borderDash: [6, 4],
        pointRadius: 0,
        tension: 0.3,
        fill: false,
        hidden: !showMA,
    };
}

function toggleMA() {
    showMA = !showMA;
    var btn = document.getElementById('maToggleBtn');
    if (btn) btn.classList.toggle('active', showMA);
    Object.keys(charts).forEach(function(id) {
        charts[id].data.datasets.forEach(function(ds) {
            if (ds.borderDash && ds.borderDash.length) ds.hidden = !showMA;
        });
        charts[id].update();
    });
}

function initChart(id, datasets) {
    var ctx = document.getElementById(id).getContext('2d');
    if (charts[id]) { charts[id].destroy(); }
    var cfg = JSON.parse(JSON.stringify(chartDefaults));
    cfg.data = { labels: [], datasets: datasets };
    charts[id] = new Chart(ctx, cfg);
}

// ── Load charts from /all_samples ────────────────────────────────────────────
function loadCharts() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState !== 4 || this.status !== 200) return;

        var lines = this.responseText.trim().split('\n');
        if (lines.length < 2) return; // header only = no data

        var labels = [], temps = [], waterTemps = [], hums = [], lights = [];
        var co2s = [], vocs = [], aqis = [];

        for (var i = 1; i < lines.length; i++) {
            var p = lines[i].split(',');
            if (p.length < 8) continue;
            labels.push(i);
            temps.push(parseFloat(p[1]));
            hums.push(parseFloat(p[2]));
            waterTemps.push(parseFloat(p[3]));
            lights.push(parseFloat(p[4]));
            aqis.push(parseFloat(p[5]));
            vocs.push(parseFloat(p[6]));
            co2s.push(parseFloat(p[7]));
        }

        // Temperature: dual y-axes (Ambient left, Water right)
        (function() {
            var ctx = document.getElementById('chart-temp').getContext('2d');
            if (charts['chart-temp']) charts['chart-temp'].destroy();
            var cfg = JSON.parse(JSON.stringify(chartDefaults));
            cfg.data = {
                labels: labels,
                datasets: [
                    Object.assign(makeDataset('Ambient', temps, 'rgb(8,145,178)'), { yAxisID: 'y' }),
                    Object.assign(makeMADataset('Ambient MA', temps, 'rgb(8,145,178)'), { yAxisID: 'y' }),
                    Object.assign(makeDataset('Water', waterTemps, 'rgb(5,150,105)'), { yAxisID: 'y1' }),
                    Object.assign(makeMADataset('Water MA', waterTemps, 'rgb(5,150,105)'), { yAxisID: 'y1' }),
                ]
            };
            cfg.options.plugins.legend.display = true;
            cfg.options.scales.y1 = {
                type: 'linear',
                position: 'right',
                ticks: { color: '#059669', font: { size: 10 } },
                grid: { drawOnChartArea: false },
            };
            charts['chart-temp'] = new Chart(ctx, cfg);
        })();

        // Simple charts with data + MA overlay
        function simpleChart(id, label, data, color) {
            var ctx = document.getElementById(id).getContext('2d');
            if (charts[id]) charts[id].destroy();
            var cfg = JSON.parse(JSON.stringify(chartDefaults));
            cfg.data = {
                labels: labels,
                datasets: [
                    makeDataset(label, data, color),
                    makeMADataset(label + ' MA', data, color),
                ]
            };
            charts[id] = new Chart(ctx, cfg);
        }

        simpleChart('chart-hum',   'Humidity', hums,   'rgb(99,102,241)');
        simpleChart('chart-light', 'Lux',      lights, 'rgb(245,158,11)');
        simpleChart('chart-co2',   'CO₂',      co2s,   'rgb(239,68,68)');
        simpleChart('chart-voc',   'VOC',      vocs,   'rgb(168,85,247)');
        simpleChart('chart-aqi',   'AQI',      aqis,   'rgb(20,184,166)');
    };
    xhr.open('GET', '/all_samples', true);
    xhr.send();
}

// ── Sensor update helper ──────────────────────────────────────────────────────
function applyReadings(obj) {
    if (obj.water_temperature !== undefined) gaugeWaterTemp.value = Number(obj.water_temperature);
    if (obj.temperature       !== undefined) gaugeTemp.value      = Number(obj.temperature);
    if (obj.humidity          !== undefined) gaugeHum.value       = Number(obj.humidity);
    if (obj.light             !== undefined) gaugeLight.value     = Number(obj.light);
    if (obj.CO2               !== undefined) gaugeCO2.value       = Number(obj.CO2);
    if (obj.VOC               !== undefined) gaugeVOC.value       = Number(obj.VOC);
    if (obj.AQI               !== undefined) gaugeAQI.value       = Number(obj.AQI);
}

// ── Polling (initial + fallback) ──────────────────────────────────────────────
function getReadings() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState === 4 && this.status === 200) {
            applyReadings(JSON.parse(this.responseText));
        }
    };
    xhr.open('GET', '/curr_readings', true);
    xhr.send();
}

// ── Sampling interval control ─────────────────────────────────────────────────
var readingsTimer = null;
var historyTimer  = null;
var currentIntervalMs = 5000;

function startReadingsPolling(ms) {
    currentIntervalMs = ms;
    if (readingsTimer) clearInterval(readingsTimer);
    readingsTimer = setInterval(getReadings, ms);
    // If history tab is open, restart its timer at the new rate too
    var histTab = document.getElementById('tab-history');
    if (histTab && histTab.style.display !== 'none') {
        startHistoryPolling(ms);
    }
}

function startHistoryPolling(ms) {
    if (historyTimer) clearInterval(historyTimer);
    historyTimer = setInterval(loadCharts, ms);
}

function stopHistoryPolling() {
    if (historyTimer) { clearInterval(historyTimer); historyTimer = null; }
}

function setSamplingInterval(ms, btn) {
    document.querySelectorAll('.interval-btn').forEach(function(b) { b.classList.remove('active'); });
    if (btn) btn.classList.add('active');
    startReadingsPolling(ms);
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/set_reporting_interval?sample_interval=' + ms, true);
    xhr.send();
}

window.addEventListener('load', function() {
    getReadings();

    // Read current sampling interval, sync polling rate, highlight active button
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        var ms = (this.readyState === 4 && this.status === 200)
            ? parseInt(this.responseText.trim())
            : 5000; // fallback
        var btn = document.querySelector('.interval-btn[data-ms="' + ms + '"]');
        if (btn) btn.classList.add('active');
        startReadingsPolling(ms);
    };
    xhr.open('GET', '/get_reporting_interval', true);
    xhr.send();
});

// ── SSE ───────────────────────────────────────────────────────────────────────
if (window.EventSource) {
    var source = new EventSource('/events');
    source.addEventListener('new_readings', function(e) {
        applyReadings(JSON.parse(e.data));
    }, false);
}
