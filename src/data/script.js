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

var gaugeBmp580Temp = new LinearGauge(Object.assign({}, darkLinear, {
    renderTo: 'gauge-bmp580-temp',
    minValue: 0, maxValue: 50,
    valueDec: 1, valueInt: 2,
    majorTicks: ['0','10','20','30','40','50'],
    minorTicks: 4,
    highlights: [{ from: 33, to: 50, color: 'rgba(239,68,68,0.25)' }],
})).draw();

var gaugePressure = new RadialGauge(Object.assign({}, darkRadial, {
    renderTo: 'gauge-pressure',
    minValue: 900, maxValue: 1100,
    valueDec: 1, valueInt: 4,
    majorTicks: ['900','940','980','1020','1060','1100'],
    minorTicks: 4,
    strokeTicks: true,
    highlights: [
        { from: 900,  to: 960,  color: 'rgba(99,102,241,0.2)'  },
        { from: 960,  to: 1040, color: 'rgba(16,185,129,0.15)' },
        { from: 1040, to: 1100, color: 'rgba(245,158,11,0.2)'  },
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

// ── History mode (recent samples vs hourly aggregations) ──────────────────────
var hourlyMode = false;

function switchHistoryMode() {
    hourlyMode = document.getElementById('hourlyToggle').checked;
    var title = document.getElementById('historyTabTitle');
    if (title) title.textContent = hourlyMode ? 'Hourly Aggregations (48h)' : 'Sample History';
    loadCharts();
}

// ── Load charts from /all_samples or /hourly_samples ─────────────────────────
function loadCharts() {
    var endpoint = hourlyMode ? '/hourly_samples' : '/all_samples';
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState !== 4 || this.status !== 200) return;

        var lines = this.responseText.trim().split('\n');
        if (lines.length < 2) {
            var msg = hourlyMode
                ? 'No hourly data yet — first entry recorded after 1 h uptime'
                : 'No samples yet — data will appear after the first reading';
            var title = document.getElementById('historyTabTitle');
            if (title) title.textContent = hourlyMode ? 'Hourly Aggregations (48h)' : 'Sample History';
            // Show placeholder text inside each chart canvas
            ['chart-water-temp','chart-outside-temp','chart-hum','chart-light',
             'chart-co2','chart-voc','chart-aqi','chart-pressure'].forEach(function(id) {
                var canvas = document.getElementById(id);
                if (!canvas) return;
                if (charts[id]) { charts[id].destroy(); delete charts[id]; }
                var ctx = canvas.getContext('2d');
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                ctx.save();
                ctx.font = '14px Inter, sans-serif';
                ctx.fillStyle = '#94a3b8';
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                ctx.fillText(msg, (canvas.width || canvas.offsetWidth || 200) / 2,
                                 (canvas.height || canvas.offsetHeight || 80) / 2);
                ctx.restore();
            });
            return;
        }

        var labels = [], timestamps = [];
        var temps = [], waterTemps = [], hums = [], lights = [];
        var co2s = [], vocs = [], aqis = [], pressures = [], bmp580Temps = [];

        for (var i = 1; i < lines.length; i++) {
            var p = lines[i].split(',');
            if (p.length < 8) continue;
            var ts = parseInt(p[0]);
            timestamps.push(ts);
            temps.push(parseFloat(p[1]));
            hums.push(parseFloat(p[2]));
            waterTemps.push(parseFloat(p[3]));
            lights.push(parseFloat(p[4]));
            aqis.push(parseFloat(p[5]));
            vocs.push(parseFloat(p[6]));
            co2s.push(parseFloat(p[7]));
            pressures.push(p.length > 8 ? parseFloat(p[8]) : null);
            bmp580Temps.push(p.length > 9 ? parseFloat(p[9]) : null);
        }

        // Labels: time-formatted for hourly, index for recent samples
        if (hourlyMode && timestamps.length > 0 && timestamps[0] > 0) {
            labels = timestamps.map(function(t) {
                if (!t) return '?';
                var d = new Date(t * 1000);
                return ('0' + d.getHours()).slice(-2) + ':00';
            });
        } else {
            for (var j = 0; j < temps.length; j++) labels.push(j + 1);
        }

        var xTitle = hourlyMode ? 'Hour' : 'Sample #';

        // Simple line chart with data + MA overlay
        function simpleChart(id, label, data, color, yOpts) {
            var ctx = document.getElementById(id).getContext('2d');
            if (charts[id]) charts[id].destroy();
            var cfg = JSON.parse(JSON.stringify(chartDefaults));
            cfg.options.scales.x.title.text = xTitle;
            if (yOpts) Object.assign(cfg.options.scales.y, yOpts);
            cfg.data = {
                labels: labels,
                datasets: [
                    makeDataset(label, data, color),
                    makeMADataset(label + ' MA', data, color),
                ]
            };
            charts[id] = new Chart(ctx, cfg);
        }

        // Water-tank temperature
        simpleChart('chart-water-temp', 'Water Tank', waterTemps, 'rgb(5,150,105)', { ticks: { stepSize: 0.5, color: '#64748b', font: { size: 10 } } });

        // Outside temperature: ENS160 + BMP580 on the same axis
        (function() {
            var ctx = document.getElementById('chart-outside-temp').getContext('2d');
            if (charts['chart-outside-temp']) charts['chart-outside-temp'].destroy();
            var cfg = JSON.parse(JSON.stringify(chartDefaults));
            cfg.options.scales.x.title.text = xTitle;
            Object.assign(cfg.options.scales.y, { ticks: { stepSize: 0.5, color: '#64748b', font: { size: 10 } } });
            cfg.data = {
                labels: labels,
                datasets: [
                    makeDataset('ENS160', temps, 'rgb(8,145,178)'),
                    makeMADataset('ENS160 MA', temps, 'rgb(8,145,178)'),
                    makeDataset('BMP580', bmp580Temps, 'rgb(234,88,12)'),
                    makeMADataset('BMP580 MA', bmp580Temps, 'rgb(234,88,12)'),
                ]
            };
            cfg.options.plugins.legend.display = true;
            charts['chart-outside-temp'] = new Chart(ctx, cfg);
        })();

        simpleChart('chart-hum',      'Humidity', hums,      'rgb(99,102,241)');
        simpleChart('chart-light',    'Lux',      lights,    'rgb(245,158,11)');
        simpleChart('chart-co2',      'CO₂',      co2s,      'rgb(239,68,68)');
        simpleChart('chart-voc',      'VOC',      vocs,      'rgb(168,85,247)');
        simpleChart('chart-pressure', 'Pressure', pressures, 'rgb(99,102,241)', { min: 950, max: 1000 });

        // AQI: colour-coded bar chart (1=excellent … 5=unhealthy)
        (function() {
            var aqiColors = ['', '#10b981', '#84cc16', '#f59e0b', '#ef4444', '#7c3aed'];
            var barColors = aqis.map(function(v) { return aqiColors[Math.round(v)] || '#94a3b8'; });
            var ctx = document.getElementById('chart-aqi').getContext('2d');
            if (charts['chart-aqi']) charts['chart-aqi'].destroy();
            charts['chart-aqi'] = new Chart(ctx, {
                type: 'bar',
                data: {
                    labels: labels,
                    datasets: [{
                        label: 'AQI',
                        data: aqis,
                        backgroundColor: barColors,
                        borderWidth: 0,
                        borderRadius: 3,
                    }]
                },
                options: {
                    responsive: true,
                    animation: false,
                    plugins: {
                        legend: { display: false },
                        tooltip: {
                            callbacks: {
                                label: function(ctx) {
                                    var names = ['', 'Excellent', 'Good', 'Moderate', 'Poor', 'Unhealthy'];
                                    return names[Math.round(ctx.parsed.y)] || ctx.parsed.y;
                                }
                            }
                        }
                    },
                    scales: {
                        y: {
                            min: 0, max: 5,
                            ticks: { stepSize: 1 },
                            grid: { color: 'rgba(0,0,0,0.06)' }
                        },
                        x: { display: hourlyMode, ticks: { color: '#94a3b8', font: { size: 10 } } }
                    }
                }
            });
        })();
    };
    xhr.open('GET', endpoint, true);
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
    if (obj.pressure          !== undefined) gaugePressure.value   = Number(obj.pressure);
    if (obj.temperature_bmp580 !== undefined) gaugeBmp580Temp.value = Number(obj.temperature_bmp580);
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

function setOtaDelay(btn) {
    var input = document.getElementById('otaDelayInput');
    var val = parseInt(input.value);
    if (isNaN(val) || val < 5)   val = 5;
    if (val > 300)               val = 300;
    input.value = val;
    btn.classList.add('active');
    setTimeout(function() { btn.classList.remove('active'); }, 1000);
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/set_ota_delay?delay=' + val, true);
    xhr.send();
}

function resetDevice() {
    if (!confirm('Reset the device now?')) return;
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/reset', true);
    xhr.send();

    var modal = document.getElementById('reset-modal');
    var countEl = document.getElementById('reset-countdown');
    modal.style.display = 'flex';
    var n = 5;
    countEl.textContent = n;
    var t = setInterval(function() {
        n--;
        countEl.textContent = n;
        if (n <= 0) {
            clearInterval(t);
            window.location.href = '/';
        }
    }, 1000);
}

// ── Sensor enable toggles ─────────────────────────────────────────────────────
var SENSOR_AHT21   = 1;
var SENSOR_ENS160  = 2;
var SENSOR_BH1750  = 4;
var SENSOR_BMP580  = 8;
var SENSOR_DS18B20 = 16;

// Maps sensor id -> gauge canvas IDs that depend on it
var sensorGauges = {
    aht21:   ['gauge-temperature', 'gauge-humidity'],
    ens160:  ['gauge-co2', 'gauge-voc', 'gauge-aqi'],
    bh1750:  ['gauge-light'],
    bmp580:  ['gauge-pressure', 'gauge-bmp580-temp'],
    ds18b20: ['gauge-water-temperature'],
};

function loadSensorStatus() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState !== 4 || this.status !== 200) return;
        var s = JSON.parse(this.responseText);
        var titles = { ok: 'OK', fault: 'Fault — check wiring / I²C bus', disabled: 'Disabled' };
        var msgs   = { fault: 'sensor fault', disabled: 'disabled' };
        ['aht21','ens160','bh1750','bmp580','ds18b20'].forEach(function(id) {
            var dot = document.getElementById('sts-' + id);
            if (dot) {
                var st = s[id] || 'fault';
                dot.className = 'sensor-sts ' + st;
                dot.title = titles[st] || st;
            }
            var inactive = (s[id] === 'fault' || s[id] === 'disabled');
            var msg = msgs[s[id]] || '';
            (sensorGauges[id] || []).forEach(function(canvasId) {
                var canvas = document.getElementById(canvasId);
                if (!canvas) return;
                var card = canvas.closest('.card');
                if (!card) return;
                if (inactive) {
                    card.classList.add('sensor-inactive');
                    var head = card.querySelector('.card-head');
                    if (head) head.setAttribute('data-inactive-msg', msg);
                } else {
                    card.classList.remove('sensor-inactive');
                    var head = card.querySelector('.card-head');
                    if (head) head.removeAttribute('data-inactive-msg');
                }
            });
        });
    };
    xhr.open('GET', '/sensor_status', true);
    xhr.send();
}

function loadSensorMask() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState !== 4 || this.status !== 200) return;
        var mask = parseInt(this.responseText.trim());
        document.getElementById('sen-aht21').checked   = !!(mask & SENSOR_AHT21);
        document.getElementById('sen-ens160').checked  = !!(mask & SENSOR_ENS160);
        document.getElementById('sen-bh1750').checked  = !!(mask & SENSOR_BH1750);
        document.getElementById('sen-bmp580').checked  = !!(mask & SENSOR_BMP580);
        document.getElementById('sen-ds18b20').checked = !!(mask & SENSOR_DS18B20);
    };
    xhr.open('GET', '/get_sensor_enabled', true);
    xhr.send();
}

function saveSensorMask() {
    var mask = 0;
    if (document.getElementById('sen-aht21').checked)   mask |= SENSOR_AHT21;
    if (document.getElementById('sen-ens160').checked)  mask |= SENSOR_ENS160;
    if (document.getElementById('sen-bh1750').checked)  mask |= SENSOR_BH1750;
    if (document.getElementById('sen-bmp580').checked)  mask |= SENSOR_BMP580;
    if (document.getElementById('sen-ds18b20').checked) mask |= SENSOR_DS18B20;
    if (mask === 0) mask = 1; // prevent all-off
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/set_sensor_enabled?mask=' + mask, true);
    xhr.send();
    document.getElementById('sensor-reset-warning').style.display = 'block';
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
    loadSensorMask();
    loadSensorStatus();
    setInterval(loadSensorStatus, 5000);

    // Read current sampling interval, sync polling rate, highlight active button
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState !== 4) return;
        var ms = (this.status === 200) ? parseInt(this.responseText.trim()) : 5000;
        var btn = document.querySelector('.interval-btn[data-ms="' + ms + '"]');
        if (btn) btn.classList.add('active');
        startReadingsPolling(ms);
    };
    xhr.open('GET', '/get_reporting_interval', true);
    xhr.send();

    // Populate OTA delay input
    var dxhr = new XMLHttpRequest();
    dxhr.onreadystatechange = function() {
        if (this.readyState === 4 && this.status === 200) {
            var n = parseInt(this.responseText.trim());
            var el = document.getElementById('otaDelayInput');
            if (el && n >= 5) el.value = n;
        }
    };
    dxhr.open('GET', '/ota_delay', true);
    dxhr.send();

    // Fetch firmware version and display in header
    var vxhr = new XMLHttpRequest();
    vxhr.onreadystatechange = function() {
        if (this.readyState === 4 && this.status === 200) {
            var el = document.getElementById('versionLabel');
            if (el) el.textContent = 'v' + this.responseText.trim();
        }
    };
    vxhr.open('GET', '/version', true);
    vxhr.send();
});

// ── SSE ───────────────────────────────────────────────────────────────────────
if (window.EventSource) {
    var source = new EventSource('/events');
    source.addEventListener('new_readings', function(e) {
        applyReadings(JSON.parse(e.data));
    }, false);
}
