#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "config.h"

// ===== DISPLAY =====
Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_HEIGHT, SCREEN_WIDTH, &Wire, OLED_RESET);

// ===== AP MODE =====
IPAddress local_IP(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
ESP8266WebServer server(80);

// ===== SENSOR DATA =====
float dist = 0, pct = 0, nvl = 0;
unsigned long last = 0;
unsigned long lastSave = 0;
char dataBuffer[256];

// History for chart (last 60 readings)
#define HISTORY_SIZE 60
float history[HISTORY_SIZE];
int histIdx = 0;

float ler() {
    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(5);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long d = pulseInLong(ECHO_PIN, HIGH, TIMEOUT_SENSOR);
    if (d == 0) return -1;
    float r = d * SOUND_SPEED / 2.0;
    return (r > MAX_DISTANCE) ? -1 : r;
}

float media() {
    float amostras[NUM_LEITURAS];
    int n = 0;
    for (int i = 0; i < NUM_LEITURAS; i++) {
        float r = ler();
        if (r > 0) amostras[n++] = r;
        delay(INTERVALO_LEITURA);
    }
    if (n < 2) return (n == 1) ? amostras[0] : -1;
    for (int i = 0; i < n - 1; i++)
        for (int j = i + 1; j < n; j++)
            if (amostras[i] > amostras[j]) { float t = amostras[i]; amostras[i] = amostras[j]; amostras[j] = t; }
    int inicio = (n > 2) ? 1 : 0;
    int fim = (n > 2) ? n - 1 : n;
    float soma = 0;
    for (int i = inicio; i < fim; i++) soma += amostras[i];
    return soma / (fim - inicio);
}

const char* sts(float p) {
    if (p >= NIVEL_ALTO) return "ALTO";
    if (p >= NIVEL_MEDIO) return "MEDIO";
    if (p >= NIVEL_BAIXO) return "BAIXO";
    if (p >= NIVEL_CRITICO) return "CRITICO";
    return "VAZIO";
}

// ===== HTML PAGE (PROGMEM) =====
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Monitor de Nível</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Instrument+Sans:wght@400;600;700&family=DM+Mono:wght@400;500&display=swap" rel="stylesheet">
<style>
:root {
  --bg: #0a0e1a;
  --surface: #111827;
  --surface2: #1a2332;
  --accent: #06d6a0;
  --accent-dim: #048a6a;
  --warn: #f59e0b;
  --danger: #ef4444;
  --text: #e2e8f0;
  --text-dim: #8892a8;
  --radius: 16px;
  --shadow: 0 8px 32px rgba(0,0,0,0.4);
}
* { margin: 0; padding: 0; box-sizing: border-box; }
body {
  font-family: 'Instrument Sans', sans-serif;
  background: var(--bg);
  color: var(--text);
  min-height: 100vh;
  overflow-x: hidden;
}
.bg-glow {
  position: fixed; inset: 0; z-index: 0;
  background:
    radial-gradient(ellipse 80% 60% at 50% 0%, rgba(6,214,160,0.08) 0%, transparent 70%),
    radial-gradient(ellipse 60% 50% at 80% 100%, rgba(6,214,160,0.04) 0%, transparent 60%);
  pointer-events: none;
}
.container { position: relative; z-index: 1; max-width: 520px; margin: 0 auto; padding: 24px 16px; }

/* Header */
header {
  text-align: center; padding: 32px 0 24px;
  animation: fadeIn 0.8s ease-out;
}
header h1 {
  font-weight: 700; font-size: 1.5rem; letter-spacing: -0.02em;
  background: linear-gradient(135deg, var(--accent), #38bdf8);
  -webkit-background-clip: text; -webkit-text-fill-color: transparent;
}
header p {
  font-family: 'DM Mono', monospace;
  font-size: 0.75rem; color: var(--text-dim); margin-top: 6px;
  letter-spacing: 0.05em;
}

/* Gauge */
.gauge-wrap {
  background: var(--surface); border-radius: var(--radius);
  padding: 32px 24px; margin-bottom: 16px;
  box-shadow: var(--shadow);
  animation: slideUp 0.6s ease-out 0.1s both;
  position: relative; overflow: hidden;
}
.gauge-wrap::before {
  content: ''; position: absolute; top: 0; left: 0; right: 0; height: 2px;
  background: linear-gradient(90deg, transparent, var(--accent), transparent);
}
.gauge-container {
  position: relative; width: 200px; height: 120px;
  margin: 0 auto 16px;
}
canvas#gauge { width: 100%; height: 100%; }
.gauge-value {
  text-align: center;
}
.gauge-value .num {
  font-size: 3.5rem; font-weight: 700; line-height: 1;
  letter-spacing: -0.03em;
  animation: pulseNum 2s ease-in-out infinite;
}
.gauge-value .unit {
  font-size: 1rem; color: var(--text-dim); margin-top: 4px;
  font-family: 'DM Mono', monospace;
}
.gauge-value .status-badge {
  display: inline-block; margin-top: 8px;
  padding: 4px 16px; border-radius: 20px;
  font-size: 0.75rem; font-weight: 600; letter-spacing: 0.05em;
  text-transform: uppercase;
  animation: fadeIn 0.3s ease-out;
}

/* Cards row */
.cards {
  display: grid; grid-template-columns: 1fr 1fr; gap: 12px;
  margin-bottom: 16px;
  animation: slideUp 0.6s ease-out 0.2s both;
}
.card {
  background: var(--surface); border-radius: var(--radius);
  padding: 16px; box-shadow: var(--shadow);
}
.card .label {
  font-size: 0.7rem; text-transform: uppercase; letter-spacing: 0.08em;
  color: var(--text-dim); margin-bottom: 4px;
}
.card .value {
  font-family: 'DM Mono', monospace; font-size: 1.2rem; font-weight: 500;
}
.card .sub {
  font-size: 0.7rem; color: var(--text-dim); margin-top: 2px;
}

/* Chart */
.chart-wrap {
  background: var(--surface); border-radius: var(--radius);
  padding: 20px; box-shadow: var(--shadow);
  animation: slideUp 0.6s ease-out 0.3s both;
  position: relative; overflow: hidden;
}
.chart-wrap::before {
  content: ''; position: absolute; top: 0; left: 0; right: 0; height: 2px;
  background: linear-gradient(90deg, transparent, #38bdf8, transparent);
}
.chart-wrap h3 {
  font-size: 0.8rem; font-weight: 600; text-transform: uppercase;
  letter-spacing: 0.08em; color: var(--text-dim); margin-bottom: 12px;
}
canvas#chart { width: 100%; height: 160px; display: block; border-radius: 8px; }

/* Footer */
footer {
  text-align: center; padding: 24px 0 32px;
  font-family: 'DM Mono', monospace; font-size: 0.65rem;
  color: var(--text-dim); letter-spacing: 0.03em;
}
footer .dot { display: inline-block; width: 6px; height: 6px;
  border-radius: 50%; margin-right: 6px; vertical-align: middle; }
footer .dot.online { background: var(--accent); animation: blink 2s ease-in-out infinite; }

@keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }
@keyframes slideUp { from { opacity: 0; transform: translateY(20px); } to { opacity: 1; transform: translateY(0); } }
@keyframes pulseNum { 0%, 100% { opacity: 1; } 50% { opacity: 0.85; } }
@keyframes blink { 0%, 100% { opacity: 1; } 50% { opacity: 0.3; } }
</style>
</head>
<body>
<div class="bg-glow"></div>
<div class="container">
  <header>
    <h1>Monitor de N&iacute;vel</h1>
    <p>TANQUE &middot; TEMPO REAL</p>
  </header>

  <div class="gauge-wrap">
    <div class="gauge-container">
      <canvas id="gauge"></canvas>
    </div>
    <div class="gauge-value">
      <div class="num" id="levelNum">--</div>
      <div class="unit">percentual</div>
      <div class="status-badge" id="statusBadge">—</div>
    </div>
  </div>

  <div class="cards">
    <div class="card">
      <div class="label">N&iacute;vel</div>
      <div class="value" id="nivelCm">--</div>
      <div class="sub">cent&iacute;metros</div>
    </div>
    <div class="card">
      <div class="label">Dist&acirc;ncia</div>
      <div class="value" id="distCm">--</div>
      <div class="sub">sensor ao topo</div>
    </div>
  </div>

  <div class="chart-wrap">
    <h3>Hist&oacute;rico</h3>
    <canvas id="chart"></canvas>
  </div>

  <footer>
    <span class="dot online" id="statusDot"></span>
    <span id="statusText">Conectado &middot; 192.168.4.1</span>
  </footer>
</div>

<script>
const ctx = document.getElementById('chart').getContext('2d');
const gauge = document.getElementById('gauge');
const gCtx = gauge.getContext('2d');

let history = [];
const MAX = 60;

function resizeCanvas() {
  const rect = gauge.parentElement.getBoundingClientRect();
  gauge.width = rect.width * (devicePixelRatio||1);
  gauge.height = rect.height * (devicePixelRatio||1);
  gCtx.scale(devicePixelRatio||1, devicePixelRatio||1);
}
resizeCanvas();
window.addEventListener('resize', resizeCanvas);

function drawGauge(pct) {
  const w = gauge.width / (devicePixelRatio||1);
  const h = gauge.height / (devicePixelRatio||1);
  gCtx.clearRect(0, 0, w, h);

  const cx = w/2, cy = h - 6, r = w/2 - 8;
  const start = Math.PI + 0.3, end = 2 * Math.PI - 0.3;

  // Arc bg
  gCtx.beginPath();
  gCtx.arc(cx, cy, r, start, end);
  gCtx.strokeStyle = 'rgba(255,255,255,0.06)';
  gCtx.lineWidth = 16; gCtx.lineCap = 'round';
  gCtx.stroke();

  // Active arc
  const val = Math.min(Math.max(pct, 0), 100);
  const activeEnd = start + (end - start) * (val / 100);
  const grad = gCtx.createLinearGradient(0, 0, w, 0);
  grad.addColorStop(0, '#048a6a');
  grad.addColorStop(0.5, '#06d6a0');
  grad.addColorStop(1, '#38bdf8');
  gCtx.beginPath();
  gCtx.arc(cx, cy, r, start, activeEnd);
  gCtx.strokeStyle = grad;
  gCtx.lineWidth = 16; gCtx.lineCap = 'round';
  gCtx.stroke();

  // Label
  gCtx.fillStyle = 'rgba(255,255,255,0.15)';
  gCtx.font = `${Math.round(r*0.45)}px "DM Mono", monospace`;
  gCtx.textAlign = 'center'; gCtx.textBaseline = 'middle';
  gCtx.fillText(Math.round(val)+'%', cx, cy - 10);
}

function drawChart() {
  const rect = document.getElementById('chart').parentElement.getBoundingClientRect();
  const cw = document.getElementById('chart');
  cw.width = rect.width - 40; cw.height = 160;
  const w = cw.width, h = cw.height;
  const ctx2 = cw.getContext('2d');
  ctx2.clearRect(0, 0, w, h);

  if (history.length < 2) {
    ctx2.fillStyle = 'rgba(255,255,255,0.1)';
    ctx2.font = '12px "DM Mono", monospace';
    ctx2.textAlign = 'center';
    ctx2.fillText('aguardando dados...', w/2, h/2);
    return;
  }

  const pad = { top: 8, bottom: 20, left: 8, right: 8 };
  const cw_ = w - pad.left - pad.right;
  const ch = h - pad.top - pad.bottom;
  const max = 100, min = 0;

  // Grid lines
  ctx2.strokeStyle = 'rgba(255,255,255,0.04)';
  ctx2.lineWidth = 1;
  for (let i = 0; i <= 4; i++) {
    const y = pad.top + ch * (1 - i/4);
    ctx2.beginPath(); ctx2.moveTo(pad.left, y); ctx2.lineTo(w-pad.right, y);
    ctx2.stroke();
  }

  // Fill
  const grad2 = ctx2.createLinearGradient(0, pad.top, 0, pad.top+ch);
  grad2.addColorStop(0, 'rgba(6,214,160,0.25)');
  grad2.addColorStop(1, 'rgba(6,214,160,0)');

  ctx2.beginPath();
  history.forEach((v, i) => {
    const x = pad.left + cw_ * (i / (MAX-1));
    const y = pad.top + ch * (1 - v/100);
    i === 0 ? ctx2.moveTo(x, pad.top+ch) : ctx2.lineTo(x, y);
  });
  ctx2.lineTo(pad.left + cw_, pad.top+ch);
  ctx2.closePath();
  ctx2.fillStyle = grad2;
  ctx2.fill();

  // Line
  ctx2.beginPath();
  history.forEach((v, i) => {
    const x = pad.left + cw_ * (i / (MAX-1));
    const y = pad.top + ch * (1 - v/100);
    i === 0 ? ctx2.moveTo(x, y) : ctx2.lineTo(x, y);
  });
  ctx2.strokeStyle = '#06d6a0';
  ctx2.lineWidth = 2;
  ctx2.lineJoin = 'round';
  ctx2.stroke();
}

function updateUI(data) {
  const pct = parseFloat(data.pct) || 0;
  document.getElementById('levelNum').textContent = Math.round(pct) + '%';
  document.getElementById('nivelCm').textContent = (parseFloat(data.nvl)||0).toFixed(1);
  document.getElementById('distCm').textContent = (parseFloat(data.dist)||0).toFixed(1);

  const badge = document.getElementById('statusBadge');
  const status = data.sts || 'OK';
  badge.textContent = status;
  if (status === 'CRITICO' || pct < 10) {
    badge.style.background = 'rgba(239,68,68,0.2)';
    badge.style.color = '#ef4444';
  } else if (status === 'BAIXO' || pct < 25) {
    badge.style.background = 'rgba(245,158,11,0.2)';
    badge.style.color = '#f59e0b';
  } else {
    badge.style.background = 'rgba(6,214,160,0.15)';
    badge.style.color = '#06d6a0';
  }

  drawGauge(pct);

  history.push(pct);
  if (history.length > MAX) history.shift();
  drawChart();
}

async function fetchData() {
  try {
    const r = await fetch('/data');
    const d = await r.json();
    updateUI(d);
  } catch(e) {
    console.log('fetch error', e);
  }
}

setInterval(fetchData, 1500);
fetchData();
</script>
</body>
</html>
)rawliteral";

// ===== JSON DATA ENDPOINT =====
void handleData() {
  char json[128];
  snprintf(json, sizeof(json),
    "{\"dist\":%.2f,\"nvl\":%.2f,\"pct\":%.2f,\"sts\":\"%s\"}",
    dist, nvl, pct, sts(pct));
  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Monitor Tanque v7.0 - AP Mode ===");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Display
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);
  if(!display.begin(SCREEN_ADDRESS, true)) {
    Serial.println("FALHA display");
  } else {
    display.clearDisplay();
    display.setTextSize(1); display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0); display.println("AP Mode");
    display.setCursor(0, 16); display.println("192.168.4.1");
    display.setCursor(0, 32); display.println("Conecte-se!");
    display.display();
  }

  // WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, local_IP, subnet);
  WiFi.softAP("Monitor-Nivel", "12345678");
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // Server
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Server started!");

  display.clearDisplay();
  display.setCursor(0, 0); display.println("WiFi: Monitor-Nivel");
  display.setCursor(0, 16); display.println("Senha: 12345678");
  display.setCursor(0, 40); display.println("IP: 192.168.4.1");
  display.display();

  for (int i = 0; i < HISTORY_SIZE; i++) history[i] = 0;

  Serial.println("Pronto!\n");
  last = millis();
}

void loop() {
  server.handleClient();
  unsigned long now = millis();
  if (now - last >= INTERVALO_ATUALIZACAO) {
    last = now;
    dist = media();
    if (dist > 0) {
      float nr = ALTURA_TANQUE - (dist - DISTANCIA_SENSOR);
      if (nr < 0) nr = 0;
      if (nr > ALTURA_TANQUE) nr = ALTURA_TANQUE;
      pct = (nr / ALTURA_TANQUE) * 100.0;
      nvl = nr;
      Serial.printf("Dist: %.2f | Nivel: %.2f (%.0f%%)\n", dist, nvl, pct);

      // History
      history[histIdx] = pct;
      histIdx = (histIdx + 1) % HISTORY_SIZE;
    }

    // Update display
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0); display.println("MONITOR DE NIVEL");
    display.drawLine(0, 10, 128, 10, SH110X_WHITE);
    display.setTextSize(3);
    display.setCursor(0, 16); display.print(pct, 1); display.println("%");
    display.setTextSize(1);
    display.setCursor(0, 52); display.print("Nivel: "); display.print(nvl, 1); display.println(" cm");
    display.setCursor(0, 66); display.print("Dist: "); display.print(dist, 1); display.println(" cm");
    display.setCursor(0, 80); display.print("Status: "); display.println(sts(pct));
    display.drawRect(98, 14, 24, 106, SH110X_WHITE);
    int hp = (int)((pct / 100.0) * 102);
    if (hp > 0) display.fillRect(100, 118 - hp, 20, hp, SH110X_WHITE);
    display.display();
  }
}
