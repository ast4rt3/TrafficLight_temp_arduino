
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <DHT.h>

const char *ssid = "wifi_ni_cheram";
const char *password = "password123";
#define DHTPIN 4       
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

const char HTML_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>Sensor</title>
<style>
:root { --accent: #f97316; --bg: #000000; --card: #0a0a0a; --text: #ffffff; --grid: rgba(255,255,255,0.05); }
* { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
body { margin: 0; background: var(--bg); color: var(--text); font-family: 'Share Tech Mono', monospace; height: 100dvh; display: flex; flex-direction: column; overflow: hidden; }
div.header { height: 50px; display: flex; align-items: center; justify-content: center; background: #000; border-bottom: 2px solid var(--accent); flex-shrink: 0; }
div.logo { font-size: 14px; font-weight: 900; color: var(--accent); letter-spacing: 8px; text-shadow: 0 0 10px var(--accent); }
div.stage { flex-grow: 1; display: flex; flex-direction: column; align-items: center; justify-content: center; padding: 20px; transition: 0.3s; }
div.gauge-box { position: relative; width: 70vw; height: 70vw; max-width: 320px; max-height: 320px; display: flex; align-items: center; justify-content: center; }
svg.gauge-svg { transform: rotate(-90deg); width: 100%; height: 100%; filter: drop-shadow(0 0 15px rgba(249, 115, 22, 0.3)); }
circle.t-track { fill: none; stroke: var(--grid); stroke-width: 14; }
circle.t-fill { fill: none; stroke: var(--accent); stroke-width: 14; stroke-linecap: butt; stroke-dasharray: 817; stroke-dashoffset: 817; transition: 0.8s ease-out; }
div.data-center { position: absolute; text-align: center; }
p.t-main { font-size: 28vw; max-font-size: 110px; font-weight: 900; margin: 0; line-height: 0.8; color: #fff; text-shadow: 0 0 20px var(--accent); }
div.t-sub { font-size: 18px; font-weight: 800; color: var(--accent); margin-top: 15px; letter-spacing: 1px; }
div.grid-shelf { width: 100%; padding: 20px; background: #050505; border-top: 1px solid var(--grid); flex-shrink: 0; display: flex; align-items: center; justify-content: center; }
div.h-box { background: var(--card); border: 1px solid var(--accent); padding: 15px 30px; border-radius: 15px; text-align: center; box-shadow: inset 0 0 15px rgba(249, 115, 22, 0.1); }
div.h-label { font-size: 10px; color: #444; font-weight: 800; text-transform: uppercase; letter-spacing: 2px; }
div.h-val { font-size: 32px; font-weight: 900; color: var(--text); margin-top: 5px; }
</style>
</head>
<body>
<div class="header"><div class="logo">SENSOR</div></div>
<div class="stage">
<div class="gauge-box">
<svg class="gauge-svg" viewBox="0 0 300 300"><circle class="t-track" cx="150" cy="150" r="130"></circle><circle id="fill" class="t-fill" cx="150" cy="150" r="130"></circle></svg>
<div class="data-center"><p class="t-main" id="t-c">--.-</p><div id="t-f" class="t-sub">--.- °F</div></div>
</div>
</div>
<div class="grid-shelf">
<div class="h-box"><div class="h-label">HUMIDITY (%)</div><div class="h-val" id="h-text">--%</div></div>
</div>
<script>
const bar = document.getElementById('fill');
const circ = 817;
async function sync() {
try {
const r = await fetch('/data');
const d = await r.json();
const c = parseFloat(d.t); const h = parseFloat(d.h);
document.getElementById('t-c').innerText = c.toFixed(1);
document.getElementById('t-f').innerText = ((c * 9/5) + 32).toFixed(1) + ' °F';
document.getElementById('h-text').innerText = Math.round(h) + '%';
bar.style.strokeDashoffset = circ - (Math.min(Math.max(c,0),50) / 50) * circ;
} catch (e) {}
}
setInterval(sync, 1000); sync();
</script>
</body>
</html>
)=====";

void handleData() {
  float h = dht.readHumidity(); float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) h = t = 0;
  String json = "{\"t\":" + String(t, 1) + ",\"h\":" + String(h, 0) + "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200); dht.begin();
  WiFi.mode(WIFI_AP_STA); WiFi.softAP(ssid, password);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  server.on("/", HTTP_GET, [](){ server.send(200, "text/html", HTML_PAGE); });
  server.on("/data", HTTP_GET, handleData);
  server.onNotFound([](){ server.sendHeader("Location", "http://192.168.4.1/", true); server.send(302, "text/plain", ""); });
  server.begin();
}
void loop() { dnsServer.processNextRequest(); server.handleClient(); }
