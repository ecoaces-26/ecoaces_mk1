#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>

// ====== EDIT THESE: your phone's hotspot (must be 2.4 GHz) ======
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
// ================================================================

// Right side = motors 1 & 2 (L298N channel A: OUT1/OUT2)
#define ENA 25
#define IN1 26
#define IN2 27
// Left side = motors 3 & 4 (L298N channel B: OUT3/OUT4)
#define IN3 32
#define IN4 33
#define ENB 14

#define DHTPIN 4
#define DHTTYPE DHT22
#define MQ2_PIN 34

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

int speedVal = 200;              // 0-255
unsigned long lastCmd = 0;
bool moving = false;
float temperature = 0, humidity = 0;
unsigned long lastDht = 0;
unsigned long lastWifiCheck = 0;
unsigned long lastIpPrint = 0;

// dir: 1 = forward, -1 = reverse, 0 = off
void setRight(int dir) {
  digitalWrite(IN1, dir == 1);
  digitalWrite(IN2, dir == -1);
  ledcWrite(ENA, dir == 0 ? 0 : speedVal);
}
void setLeft(int dir) {
  digitalWrite(IN3, dir == 1);
  digitalWrite(IN4, dir == -1);
  ledcWrite(ENB, dir == 0 ? 0 : speedVal);
}
void drive(int r, int l) {
  setRight(r);
  setLeft(l);
  moving = (r != 0 || l != 0);
}

void handleCmd() {
  if (server.hasArg("s")) speedVal = constrain(server.arg("s").toInt(), 80, 255);
  String c = server.arg("c");
  lastCmd = millis();
  if      (c == "W") drive(1, 1);     // forward: all motors forward
  else if (c == "S") drive(-1, -1);   // reverse: all motors reverse
  else if (c == "A") drive(1, 0);     // left turn: motors 1&2 fwd, 3&4 off
  else if (c == "D") drive(0, 1);     // right turn: motors 3&4 fwd, 1&2 off
  else if (c == "Q") drive(1, -1);    // pivot left: 1&2 fwd, 3&4 reverse
  else if (c == "E") drive(-1, 1);    // pivot right: 3&4 fwd, 1&2 reverse
  else               drive(0, 0);     // stop
  server.send(200, "text/plain", "OK");
}

void handleData() {
  int gas = analogRead(MQ2_PIN);
  String j = "{\"t\":" + String(temperature, 1) + ",\"h\":" + String(humidity, 1) +
             ",\"g\":" + String(gas) + "}";
  server.send(200, "application/json", j);
}

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<title>Rover Control</title>
<style>
body{font-family:sans-serif;background:#111;color:#eee;text-align:center;margin:0;padding:10px}
.pad{display:grid;grid-template-columns:repeat(3,90px);gap:10px;justify-content:center;margin:20px auto}
button{height:90px;font-size:26px;border:none;border-radius:12px;background:#2a2a2a;color:#fff;touch-action:none}
button.on{background:#1e90ff}
.stop{background:#b22}
.card{display:inline-block;background:#222;border-radius:10px;padding:10px 18px;margin:5px}
</style></head><body>
<h2>Tank Rover</h2>
<div>
  <div class="card">Temp<br><b id="t">--</b> &deg;C</div>
  <div class="card">Humidity<br><b id="h">--</b> %</div>
  <div class="card">Gas (raw)<br><b id="g">--</b></div>
</div>
<div class="pad">
  <button data-k="Q">Q<br>&#8630;</button>
  <button data-k="W">W<br>&#9650;</button>
  <button data-k="E">E<br>&#8631;</button>
  <button data-k="A">A<br>&#9664;</button>
  <button class="stop" data-k="X">STOP</button>
  <button data-k="D">D<br>&#9654;</button>
  <span></span>
  <button data-k="S">S<br>&#9660;</button>
  <span></span>
</div>
<div>Speed: <input type="range" id="sp" min="80" max="255" value="200"> <span id="spv">200</span></div>
<p style="color:#888">Hold W A S D Q E on the keyboard. Release to stop.</p>
<script>
const keys=['W','A','S','D','Q','E'];
let cur='X', speed=200;
const btns={};
document.querySelectorAll('button').forEach(b=>btns[b.dataset.k]=b);
function send(c){fetch('/cmd?c='+c+'&s='+speed).catch(()=>{});}
function setCur(c){
  cur=c; send(c);
  keys.forEach(k=>btns[k].classList.toggle('on',k===c));
}
document.addEventListener('keydown',e=>{
  const k=e.key.toUpperCase();
  if(keys.includes(k)&&k!==cur) setCur(k);
});
document.addEventListener('keyup',e=>{
  const k=e.key.toUpperCase();
  if(k===cur) setCur('X');
});
keys.forEach(k=>{
  btns[k].addEventListener('pointerdown',()=>setCur(k));
  btns[k].addEventListener('pointerup',()=>setCur('X'));
  btns[k].addEventListener('pointerleave',()=>{if(cur===k)setCur('X');});
});
btns['X'].addEventListener('pointerdown',()=>setCur('X'));
window.addEventListener('blur',()=>setCur('X'));
// heartbeat: keeps the rover moving while a key is held
setInterval(()=>{if(cur!=='X')send(cur);},250);
document.getElementById('sp').oninput=e=>{
  speed=e.target.value; document.getElementById('spv').textContent=speed;
};
setInterval(()=>{
  fetch('/data').then(r=>r.json()).then(d=>{
    t.textContent=d.t; h.textContent=d.h; g.textContent=d.g;
  }).catch(()=>{});
},2000);
</script></body></html>
)rawliteral";

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to hotspot");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected! Open: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Not connected yet - check hotspot is ON, 2.4 GHz, name/password. Retrying in loop...");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcAttach(ENA, 1000, 8);   // ESP32 core 3.x
  ledcAttach(ENB, 1000, 8);
  drive(0, 0);

  dht.begin();
  analogReadResolution(12);

  connectWiFi();

  if (MDNS.begin("rover")) {
    Serial.println("Or open: http://rover.local (iPhone/Windows/Mac)");
  }

  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/cmd", handleCmd);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();

  // Failsafe: stop if the browser stops sending commands (WiFi drop, tab closed)
  if (moving && millis() - lastCmd > 700) drive(0, 0);

  // Reconnect to the hotspot if it drops
  if (millis() - lastWifiCheck > 10000) {
    lastWifiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi lost, reconnecting...");
      drive(0, 0);
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
  }

  // Print the IP every 10 seconds so you can always find it in Serial Monitor
  if (millis() - lastIpPrint > 10000) {
    lastIpPrint = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Rover page: http://");
      Serial.println(WiFi.localIP());
    }
  }

  // Read the DHT22 every 2 seconds
  if (millis() - lastDht > 2000) {
    lastDht = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) temperature = t;
    if (!isnan(h)) humidity = h;
  }
}