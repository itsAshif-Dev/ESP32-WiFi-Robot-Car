#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ================= MOTOR PINS FOR MX1508 =================
#define IN1 25
#define IN2 26
#define IN3 27
#define IN4 14

// ================= BATTERY PIN =================
#define BATTERY_PIN 34

// ================= HOTSPOT SETTINGS =================
const char* ssid = "ESP32_CAR";
const char* password = "12345678";

// ================= WEB SERVER =================
AsyncWebServer server(80);

// ================= SPEED =================
int motorSpeed = 100;

// ================= MILLIS TIMER =================
unsigned long previousMillis = 0;
const unsigned long LOOP_INTERVAL = 10; // ms — replaces delay(10)

// ================= BATTERY TIMER =================
unsigned long previousBatteryMillis = 0;
const unsigned long BATTERY_INTERVAL = 5000; // Read battery every 5 seconds

// ================= GLOBAL BATTERY VARIABLES =================
float currentBatteryVoltage = 0;
int currentBatteryPercent = 0;

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=========================================");
  Serial.println("ESP32 WiFi Car — HUD Edition");
  Serial.println("=========================================");
  
  // Initialize battery monitoring
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);

  // LEDC Setup - Works with both ESP32 core v2.x and v3.x
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    // ESP32 core v3.x - Use ledcAttach
    ledcAttach(IN1, 5000, 8);
    ledcAttach(IN2, 5000, 8);
    ledcAttach(IN3, 5000, 8);
    ledcAttach(IN4, 5000, 8);
  #else
    // ESP32 core v2.x - Use ledcSetup and ledcAttachPin
    ledcSetup(0, 5000, 8); ledcAttachPin(IN1, 0);
    ledcSetup(1, 5000, 8); ledcAttachPin(IN2, 1);
    ledcSetup(2, 5000, 8); ledcAttachPin(IN3, 2);
    ledcSetup(3, 5000, 8); ledcAttachPin(IN4, 3);
  #endif

  stopCar();

  Serial.println("Creating WiFi Hotspot...");
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));

  Serial.println("HOTSPOT READY");
  Serial.print("SSID: "); Serial.println(ssid);
  Serial.print("PASS: "); Serial.println(password);
  Serial.print("IP:   "); Serial.println(WiFi.softAPIP());

  setupServer();
  server.begin();
  Serial.println("Web server started — open http://192.168.4.1");
  
  // Initial battery reading
  updateBatteryReading();
}

// ================= BATTERY FUNCTIONS =================
void updateBatteryReading() {
  // Take 20 samples and average
  long total = 0;
  for(int i = 0; i < 20; i++) {
    total += analogRead(BATTERY_PIN);
    delay(2);
  }
  
  int adcValue = total / 20;
  
  float gpioVoltage = (adcValue / 4095.0) * 3.3;
  
  // Voltage divider ratio = 3
  float batteryVoltage = gpioVoltage * 3.0;
  
  // Calibration factor (adjust based on actual measured voltage)
  batteryVoltage = batteryVoltage * 1.094;
  
  currentBatteryVoltage = batteryVoltage;
  
  // Calculate percentage (6.8V - 8.4V range for 2S LiPo)
  currentBatteryPercent = map(batteryVoltage * 100, 680, 840, 0, 100);
  currentBatteryPercent = constrain(currentBatteryPercent, 0, 100);
  
  // Print to serial for debugging
  Serial.print("ADC Value: ");
  Serial.println(adcValue);
  Serial.print("Battery Voltage: ");
  Serial.print(currentBatteryVoltage);
  Serial.println(" V");
  Serial.print("Battery Percentage: ");
  Serial.print(currentBatteryPercent);
  Serial.println(" %");
  Serial.println("----------------");
}

float getBatteryVoltage() {
  return currentBatteryVoltage;
}

int getBatteryPercent() {
  return currentBatteryPercent;
}

// ================= WEB SERVER SETUP =================
void setupServer() {

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    float bv = getBatteryVoltage();
    int bp = getBatteryPercent();

    // Battery bar width (0–100%)
    String bw = String(bp);

    // Battery colour: green >50, amber 20-50, red <20
    String bColor = bp > 50 ? "#3dd68c" : (bp > 20 ? "#f5a623" : "#e8463a");

    String html = R"rawhtml(<!DOCTYPE html>
<html>
<head>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>ESP32 CAR — CTRL</title>
<link href='https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Rajdhani:wght@400;600;700&display=swap' rel='stylesheet'>
<style>
*{margin:0;padding:0;box-sizing:border-box;}
:root{
  --bg:#0a0a0c;
  --panel:#111116;
  --border:#2a2a35;
  --amber:#f5a623;
  --amber-dim:#7a5010;
  --red:#e8463a;
  --green:#3dd68c;
  --text:#e8e4d8;
  --muted:#5a5a6e;
  --mono:'Share Tech Mono',monospace;
  --head:'Rajdhani',sans-serif;
}
body{background:var(--bg);color:var(--text);font-family:var(--mono);min-height:100vh;display:flex;justify-content:center;align-items:flex-start;padding:0;}
.hud{width:100%;max-width:420px;padding:16px;display:flex;flex-direction:column;gap:12px;}

.topbar{display:flex;align-items:center;justify-content:space-between;border-bottom:1px solid var(--border);padding-bottom:10px;}
.topbar-left{display:flex;flex-direction:column;}
.unit-id{font-family:var(--head);font-size:22px;font-weight:700;letter-spacing:3px;color:var(--amber);}
.unit-sub{font-size:10px;color:var(--muted);letter-spacing:2px;margin-top:1px;}
.status-pill{display:flex;align-items:center;gap:6px;background:#0d1a0f;border:1px solid #1a3a22;border-radius:4px;padding:5px 10px;}
.dot{width:7px;height:7px;border-radius:50%;background:var(--green);animation:pulse 2s infinite;}
@keyframes pulse{0%,100%{opacity:1;}50%{opacity:0.4;}}
.status-text{font-size:10px;color:var(--green);letter-spacing:1px;}

.panel{background:var(--panel);border:1px solid var(--border);border-radius:6px;padding:14px;}
.panel-label{font-size:9px;letter-spacing:2px;color:var(--muted);margin-bottom:10px;text-transform:uppercase;}

.battery-row{display:flex;align-items:center;gap:12px;}
.batt-icon{display:flex;align-items:center;gap:2px;}
.batt-body{width:34px;height:16px;border:2px solid var(--amber);border-radius:3px;position:relative;display:flex;align-items:center;padding:2px;}
.batt-tip{width:4px;height:8px;background:var(--amber);border-radius:0 2px 2px 0;margin-left:2px;}
.batt-fill{height:100%;border-radius:1px;}
.batt-stats{display:flex;flex-direction:column;}
.batt-big{font-family:var(--head);font-size:28px;font-weight:700;line-height:1;}
.batt-sub{font-size:10px;color:var(--muted);margin-top:2px;}
.batt-bar-wrap{flex:1;height:6px;background:#1a1a22;border-radius:3px;overflow:hidden;}
.batt-bar{height:100%;border-radius:3px;}

.speed-header{display:flex;justify-content:space-between;align-items:baseline;}
.speed-val{font-family:var(--head);font-size:32px;font-weight:700;color:var(--text);}
.speed-unit{font-size:10px;color:var(--muted);}
input[type=range]{width:100%;-webkit-appearance:none;height:4px;background:#1e1e2e;border-radius:2px;outline:none;margin:14px 0 4px;}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:20px;height:20px;background:var(--amber);border-radius:50%;cursor:pointer;border:2px solid var(--bg);}
.speed-marks{display:flex;justify-content:space-between;font-size:9px;color:var(--muted);}

.dpad{display:grid;grid-template-columns:1fr 1fr 1fr;grid-template-rows:1fr 1fr 1fr;gap:8px;aspect-ratio:1/1;max-width:240px;margin:0 auto;}
.dpad-btn{background:#141420;border:1px solid var(--border);border-radius:6px;display:flex;align-items:center;justify-content:center;cursor:pointer;user-select:none;transition:background 0.08s,border-color 0.08s,transform 0.08s;-webkit-tap-highlight-color:transparent;}
.dpad-btn:active,.dpad-btn.pressed{background:#1e1a08;border-color:var(--amber);transform:scale(0.95);}
.dpad-btn svg{width:28px;height:28px;stroke:var(--muted);transition:stroke 0.08s;}
.dpad-btn:active svg,.dpad-btn.pressed svg{stroke:var(--amber);}
.center-btn{background:#180a0a;border-color:#3a1a18;}
.center-btn:active,.center-btn.pressed{background:#2a0f0d;border-color:var(--red);}
.center-btn svg{stroke:#5a2a28;}
.center-btn:active svg,.center-btn.pressed svg{stroke:var(--red);}
.dpad-empty{background:transparent;border:none;pointer-events:none;}

.telemetry{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;}
.tele-cell{background:#0d0d14;border:1px solid var(--border);border-radius:5px;padding:10px 8px;text-align:center;}
.tele-val{font-family:var(--head);font-size:18px;font-weight:600;color:var(--text);}
.tele-label{font-size:9px;color:var(--muted);letter-spacing:1px;margin-top:3px;}

.footer{display:flex;align-items:center;justify-content:space-between;padding-top:4px;border-top:1px solid var(--border);}
.footer-ip{font-size:10px;color:var(--muted);}
.footer-keys{font-size:9px;color:var(--muted);letter-spacing:1px;}

.scanline{position:fixed;top:0;left:0;width:100%;height:100%;pointer-events:none;background:repeating-linear-gradient(0deg,rgba(0,0,0,0.03) 0px,rgba(0,0,0,0.03) 1px,transparent 1px,transparent 2px);z-index:0;}
.hud{position:relative;z-index:1;}
</style>
</head>
<body>
<div class='scanline'></div>
<div class='hud'>

  <div class='topbar'>
    <div class='topbar-left'>
      <div class='unit-id'>ESP-CAR</div>
      <div class='unit-sub'>MX1508 &middot; DUAL MOTOR &middot; AP MODE</div>
    </div>
    <div class='status-pill'>
      <div class='dot'></div>
      <span class='status-text'>ONLINE</span>
    </div>
  </div>

  <div class='panel'>
    <div class='panel-label'>Power Cell</div>
    <div class='battery-row'>
      <div class='batt-icon'>
        <div class='batt-body'>
          <div class='batt-fill' style='width:)rawhtml";

    html += bw;
    html += R"rawhtml(%;background:)rawhtml";
    html += bColor;
    html += R"rawhtml(;'></div>
        </div>
        <div class='batt-tip' style='background:)rawhtml";
    html += bColor;
    html += R"rawhtml(;'></div>
      </div>
      <div class='batt-stats'>
        <div class='batt-big' style='color:)rawhtml";
    html += bColor;
    html += R"rawhtml(;'>)rawhtml";
    html += String(bp);
    html += R"rawhtml(%</div>
        <div class='batt-sub'>)rawhtml";
    html += String(bv, 2);
    html += R"rawhtml(V &middot; 2S LIPO</div>
      </div>
      <div class='batt-bar-wrap'>
        <div class='batt-bar' style='width:)rawhtml";
    html += bw;
    html += R"rawhtml(%;background:)rawhtml";
    html += bColor;
    html += R"rawhtml(;'></div>
      </div>
    </div>
  </div>

  <div class='panel'>
    <div class='panel-label'>Throttle</div>
    <div class='speed-header'>
      <div>
        <span class='speed-val' id='speedVal'>)rawhtml";
    html += String(motorSpeed);
    html += R"rawhtml(</span>
        <span class='speed-unit'> / 255 PWM</span>
      </div>
      <span style='font-size:11px;color:var(--amber)' id='speedPct'>)rawhtml";
    html += String(motorSpeed * 100 / 255);
    html += R"rawhtml(%</span>
    </div>
    <input type='range' min='0' max='255' value=')rawhtml";
    html += String(motorSpeed);
    html += R"rawhtml(' id='slider' oninput='updateSpd(this.value)'>
    <div class='speed-marks'><span>0</span><span>64</span><span>128</span><span>192</span><span>255</span></div>
  </div>

  <div class='panel'>
    <div class='panel-label'>Directional Control</div>
    <div class='dpad'>
      <div class='dpad-empty'></div>
      <div class='dpad-btn' id='btn-fwd'
        ontouchstart='press("forward");event.preventDefault();'
        ontouchend='rel();'
        onmousedown='press("forward")'
        onmouseup='rel()'>
        <svg viewBox='0 0 24 24' fill='none' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>
          <polyline points='18 15 12 9 6 15'/>
        </svg>
      </div>
      <div class='dpad-empty'></div>

      <div class='dpad-btn' id='btn-lft'
        ontouchstart='press("left");event.preventDefault();'
        ontouchend='rel();'
        onmousedown='press("left")'
        onmouseup='rel()'>
        <svg viewBox='0 0 24 24' fill='none' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>
          <polyline points='15 18 9 12 15 6'/>
        </svg>
      </div>
      <div class='dpad-btn center-btn' id='btn-stp' onclick='sendCmd("stop")'>
        <svg viewBox='0 0 24 24' fill='none' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>
          <rect x='6' y='6' width='12' height='12' rx='2'/>
        </svg>
      </div>
      <div class='dpad-btn' id='btn-rgt'
        ontouchstart='press("right");event.preventDefault();'
        ontouchend='rel();'
        onmousedown='press("right")'
        onmouseup='rel()'>
        <svg viewBox='0 0 24 24' fill='none' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>
          <polyline points='9 18 15 12 9 6'/>
        </svg>
      </div>

      <div class='dpad-empty'></div>
      <div class='dpad-btn' id='btn-bwd'
        ontouchstart='press("backward");event.preventDefault();'
        ontouchend='rel();'
        onmousedown='press("backward")'
        onmouseup='rel()'>
        <svg viewBox='0 0 24 24' fill='none' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>
          <polyline points='6 9 12 15 18 9'/>
        </svg>
      </div>
      <div class='dpad-empty'></div>
    </div>
  </div>

  <div class='panel'>
    <div class='panel-label'>Telemetry</div>
    <div class='telemetry'>
      <div class='tele-cell'>
        <div class='tele-val' id='stateVal'>STOP</div>
        <div class='tele-label'>STATE</div>
      </div>
      <div class='tele-cell'>
        <div class='tele-val' id='pwmVal'>)rawhtml";
    html += String(motorSpeed);
    html += R"rawhtml(</div>
        <div class='tele-label'>PWM</div>
      </div>
      <div class='tele-cell'>
        <div class='tele-val' id='battPercent'>)rawhtml";
    html += String(bp);
    html += R"rawhtml(%</div>
        <div class='tele-label'>BATT</div>
      </div>
    </div>
  </div>

  <div class='footer'>
    <span class='footer-ip'>192.168.4.1 &middot; PORT 80</span>
    <span class='footer-keys'>ARROWS &middot; SPACE=STOP</span>
  </div>

</div>
<script>
function sendCmd(c){fetch('/'+c);}

function updateSpd(v){
  document.getElementById('speedVal').textContent=v;
  document.getElementById('pwmVal').textContent=v;
  document.getElementById('speedPct').textContent=Math.round(v/255*100)+'%';
  fetch('/speed?value='+v);
}

var activeBtn=null;
var btnMap={forward:'btn-fwd',backward:'btn-bwd',left:'btn-lft',right:'btn-rgt'};
var labelMap={forward:'FWD',backward:'REV',left:'LEFT',right:'RIGHT',stop:'STOP'};

function press(c){
  sendCmd(c);
  activeBtn=c;
  if(btnMap[c]) document.getElementById(btnMap[c]).classList.add('pressed');
  document.getElementById('stateVal').textContent=labelMap[c]||'---';
}

function rel(){
  if(activeBtn){
    if(btnMap[activeBtn]) document.getElementById(btnMap[activeBtn]).classList.remove('pressed');
    activeBtn=null;
  }
  sendCmd('stop');
  document.getElementById('stateVal').textContent='STOP';
}

document.addEventListener('keydown',function(e){
  var map={ArrowUp:'forward',ArrowDown:'backward',ArrowLeft:'left',ArrowRight:'right',' ':'stop'};
  if(map[e.key]){
    if(e.key===' ') rel();
    else press(map[e.key]);
    e.preventDefault();
  }
});
document.addEventListener('keyup',function(e){
  if(['ArrowUp','ArrowDown','ArrowLeft','ArrowRight'].includes(e.key)) rel();
});

// Auto-refresh battery percentage every 3 seconds
setInterval(function() {
  fetch('/battery')
    .then(response => response.json())
    .then(data => {
      document.getElementById('battPercent').textContent = data.percent + '%';
      // Update battery bar colors if needed
      var battFill = document.querySelector('.batt-fill');
      var battTip = document.querySelector('.batt-tip');
      var battBig = document.querySelector('.batt-big');
      var color = data.percent > 50 ? '#3dd68c' : (data.percent > 20 ? '#f5a623' : '#e8463a');
      battFill.style.width = data.percent + '%';
      battFill.style.background = color;
      battTip.style.background = color;
      battBig.style.color = color;
      battBig.textContent = data.percent + '%';
      document.querySelector('.batt-sub').innerHTML = data.voltage.toFixed(2) + 'V &middot; 2S LIPO';
    });
}, 3000);
</script>
</body>
</html>)rawhtml";

    request->send(200, "text/html", html);
  });

  server.on("/forward", HTTP_GET, [](AsyncWebServerRequest *request) {
    forward(); request->send(200, "text/plain", "OK");
  });
  server.on("/backward", HTTP_GET, [](AsyncWebServerRequest *request) {
    backward(); request->send(200, "text/plain", "OK");
  });
  server.on("/left", HTTP_GET, [](AsyncWebServerRequest *request) {
    left(); request->send(200, "text/plain", "OK");
  });
  server.on("/right", HTTP_GET, [](AsyncWebServerRequest *request) {
    right(); request->send(200, "text/plain", "OK");
  });
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    stopCar(); request->send(200, "text/plain", "OK");
  });
  server.on("/speed", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      motorSpeed = constrain(request->getParam("value")->value().toInt(), 0, 255);
    }
    request->send(200, "text/plain", "OK");
  });
  
  // New endpoint for battery data
  server.on("/battery", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{\"voltage\":" + String(currentBatteryVoltage) + ",\"percent\":" + String(currentBatteryPercent) + "}";
    request->send(200, "application/json", json);
  });
}

// ================= LOOP =================
void loop() {
  unsigned long currentMillis = millis();

  // Non-blocking interval for main control loop
  if (currentMillis - previousMillis >= LOOP_INTERVAL) {
    previousMillis = currentMillis;
    // Main control tasks (if needed)
  }
  
  // Update battery readings every 5 seconds
  if (currentMillis - previousBatteryMillis >= BATTERY_INTERVAL) {
    previousBatteryMillis = currentMillis;
    updateBatteryReading();
  }
}

// ================= MOTORS =================
void forward() {
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    // ESP32 core v3.x - Use pin numbers directly
    ledcWrite(IN1, motorSpeed); 
    ledcWrite(IN2, 0);
    ledcWrite(IN3, motorSpeed); 
    ledcWrite(IN4, 0);
  #else
    // ESP32 core v2.x - Use channel numbers
    ledcWrite(0, motorSpeed); 
    ledcWrite(1, 0);
    ledcWrite(2, motorSpeed); 
    ledcWrite(3, 0);
  #endif
  Serial.println("FWD spd=" + String(motorSpeed));
}

void backward() {
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcWrite(IN1, 0); 
    ledcWrite(IN2, motorSpeed);
    ledcWrite(IN3, 0); 
    ledcWrite(IN4, motorSpeed);
  #else
    ledcWrite(0, 0); 
    ledcWrite(1, motorSpeed);
    ledcWrite(2, 0); 
    ledcWrite(3, motorSpeed);
  #endif
  Serial.println("REV spd=" + String(motorSpeed));
}

void left() {
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcWrite(IN1, 0); 
    ledcWrite(IN2, motorSpeed);
    ledcWrite(IN3, motorSpeed); 
    ledcWrite(IN4, 0);
  #else
    ledcWrite(0, 0); 
    ledcWrite(1, motorSpeed);
    ledcWrite(2, motorSpeed); 
    ledcWrite(3, 0);
  #endif
  Serial.println("LEFT spd=" + String(motorSpeed));
}

void right() {
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcWrite(IN1, motorSpeed); 
    ledcWrite(IN2, 0);
    ledcWrite(IN3, 0); 
    ledcWrite(IN4, motorSpeed);
  #else
    ledcWrite(0, motorSpeed); 
    ledcWrite(1, 0);
    ledcWrite(2, 0); 
    ledcWrite(3, motorSpeed);
  #endif
  Serial.println("RIGHT spd=" + String(motorSpeed));
}

void stopCar() {
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcWrite(IN1, 0); 
    ledcWrite(IN2, 0);
    ledcWrite(IN3, 0); 
    ledcWrite(IN4, 0);
  #else
    ledcWrite(0, 0); 
    ledcWrite(1, 0);
    ledcWrite(2, 0); 
    ledcWrite(3, 0);
  #endif
  Serial.println("STOP");
}