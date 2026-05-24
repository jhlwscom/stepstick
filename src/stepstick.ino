#include "version.h"
#include <ESPmDNS.h>
#include <ImprovWiFiLibrary.h>
#include <M5Unified.h>
#include <Preferences.h>
#include <SparkFun_BMI270_Arduino_Library.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>

BMI270 imu;
WebSocketsServer webSocket = WebSocketsServer(81);
WebServer server(80);
Preferences prefs;
ImprovWiFi improvSerial(&Serial);

// --- Activity States (matches BMI270 sensor output) ---
enum Activity : uint8_t { STILL = 0, WALKING = 1, RUNNING = 2 };

// --- Step Logic Variables ---
uint32_t hwStepCount = 0;
uint32_t stepOffset = 0;
uint32_t displaySteps = 0;
uint32_t lastBroadcastSteps = 0;
Activity currentActivity = STILL;
Activity lastActivity = (Activity)255;
String activityString = "🧍";
int batteryLevel = 0;

// --- Power Management ---
unsigned long lastInteractionTime = 0;
const unsigned long SCREEN_TIMEOUT = 15000;
bool isScreenOn = true;

// --- Theme Variables ---
String themeBgColor = "#0f0f14";
String themeTextColor = "#ffffff";
String themeWalkColor = "#00ffcc";
String themeRunColor = "#ff4444";

String iconStill = "";
String iconWalk = "";
String iconRun = "";

bool themeGlow = true;
bool themeGreyscale = true;

// --- WebSocket Event Handler ---
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t length) {
  if (type == WStype_CONNECTED) {
    // Send the current known state to the newly connected client immediately
    String json = "{\"steps\":" + String(displaySteps) +
                  ",\"state\":" + String(currentActivity) + ",\"activity\":\"" +
                  activityString + "\"" +
                  ",\"battery\":" + String(batteryLevel) + "}";
    webSocket.sendTXT(num, json);
  }
}

// --- HTML PAYLOADS ---
const char *configHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Step Counter Config</title>
    <link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='.9em' font-size='90'>🦶</text></svg>">
    <style>
        body { font-family: sans-serif; background: #121212; color: #fff; padding: 20px; max-width: 400px; margin: 0 auto; }
        .card { background: #1e1e1e; padding: 20px; border-radius: 8px; text-align: left; margin-bottom: 20px; }
        h1 { color: #00ffcc; text-align: center; }
        h2 { text-align: center; font-size: 1.2rem; }
        .btn { background: #ff4444; color: white; border: none; padding: 15px 20px; font-size: 16px; border-radius: 5px; cursor: pointer; width: 100%; font-weight: bold; margin-top: 10px; }
        .btn-save { background: #00ffcc; color: #000; }
        .btn:hover { opacity: 0.8; }
        .form-group { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; border-bottom: 1px solid #333; padding-bottom: 10px; }
        .form-group.col { flex-direction: column; align-items: flex-start; gap: 8px; }
        input[type="color"] { border: none; width: 50px; height: 30px; background: none; cursor: pointer; }
        input[type="text"] { width: 100%; padding: 8px; box-sizing: border-box; background: #2a2a2a; border: 1px solid #444; color: white; border-radius: 4px; }
        input[type="checkbox"] { width: 20px; height: 20px; cursor: pointer; }
        .help-text { font-size: 12px; color: #888; margin-top: -5px; }
        
        .copy-box { display: flex; gap: 10px; margin-top: 15px; }
        .copy-box input { flex-grow: 1; font-family: monospace; font-size: 14px; cursor: text; }
        .btn-copy { background: #444; color: white; border: none; padding: 0 15px; border-radius: 4px; cursor: pointer; font-weight: bold; white-space: nowrap; transition: 0.2s; }
        .btn-copy:hover { background: #555; }
        .btn-copy.success { background: #00ffcc; color: #000; }
        .toast { position: fixed; top: 20px; left: 50%; transform: translateX(-50%); background: #00ffcc; color: #000; padding: 12px 24px; border-radius: 8px; font-weight: bold; opacity: 0; transition: opacity 0.3s; pointer-events: none; }
        .toast.show { opacity: 1; }
    </style>
</head>
<body>
    <div class='card'>
        <h2>OBS Integration</h2>
        <div class="help-text" style="text-align: center;">Add this as a Browser Source (Clear custom CSS)</div>
        <div class="copy-box">
            <input type="text" id="obs-url" readonly>
            <button class="btn-copy" onclick="copyURL()" id="copy-btn">Copy</button>
        </div>
    </div>

    <div class='card'>
        <h2>Dashboard Controls</h2>
        <form action='/reset' method='POST'>
            <button class='btn' type='submit'>Reset Step Count</button>
        </form>
    </div>
    
    <div class='card'>
        <h2>Overlay Theme & Icons</h2>
        <form action='/savetheme' method='POST'>
            
            <div class="form-group col">
                <label>Still Icon (Emoji or URL)</label>
                <div class="help-text">Leave blank to use default 🧍</div>
                <input type="text" name="iconStill" value="%ISTILL%">
            </div>
            <div class="form-group col">
                <label>Walk Icon (Emoji or URL)</label>
                <div class="help-text">Leave blank to use default 🚶</div>
                <input type="text" name="iconWalk" value="%IWALK%">
            </div>
            <div class="form-group col">
                <label>Run Icon (Emoji or URL)</label>
                <div class="help-text">Leave blank to use default 🏃</div>
                <input type="text" name="iconRun" value="%IRUN%">
            </div>
            
            <div style="height: 20px;"></div>

            <div class="form-group">
                <label>Background Color</label>
                <input type="color" name="bgColor" value="%BG%">
            </div>
            <div class="form-group">
                <label>Text Color</label>
                <input type="color" name="textColor" value="%TEXT%">
            </div>
            <div class="form-group">
                <label>Walk Highlight</label>
                <input type="color" name="walkColor" value="%WALK%">
            </div>
            <div class="form-group">
                <label>Run Highlight</label>
                <input type="color" name="runColor" value="%RUN%">
            </div>

            <div style="height: 10px;"></div>

            <div class="form-group">
                <label>Enable Greyscale (Inactive Status)</label>
                <input type="checkbox" name="grey" %GREY%>
            </div>
            <div class="form-group">
                <label>Enable Active Glow</label>
                <input type="checkbox" name="glow" %GLOW%>
            </div>

            <button class='btn btn-save' type='submit'>Save Settings</button>
        </form>
    </div>

    <div class="toast" id="toast">Settings Saved!</div>
    <script>
        const urlInput = document.getElementById('obs-url');
        urlInput.value = 'http://' + window.location.hostname + '/overlay';

        if (new URLSearchParams(window.location.search).get('saved')) {
            const t = document.getElementById('toast');
            t.classList.add('show');
            setTimeout(() => t.classList.remove('show'), 3000);
            history.replaceState({}, '', '/');
        }

        function copyURL() {
            urlInput.select();
            urlInput.setSelectionRange(0, 99999);
            document.execCommand('copy');
            const btn = document.getElementById('copy-btn');
            btn.innerText = 'Copied!';
            btn.classList.add('success');
            setTimeout(() => {
                btn.innerText = 'Copy';
                btn.classList.remove('success');
            }, 2000);
        }
    </script>
</body>
</html>
)rawliteral";

const char *overlayHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='.9em' font-size='90'>🦶</text></svg>">
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Inter:wght@600;800&display=swap">
    <link rel="stylesheet" href="/theme.css">
    <style>
        body { margin: 0; padding: 20px; background-color: transparent; font-family: 'Inter', sans-serif; color: var(--text-color); }
        .hud-container { display: inline-flex; align-items: center; background: var(--bg-color); border-radius: 16px; padding: 12px 24px; gap: 24px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3); }
        .metric { display: flex; align-items: center; gap: 12px; }
        .step-value { font-size: 42px; font-weight: 800; letter-spacing: -1px; line-height: 1; }
        .step-label { font-size: 14px; font-weight: 600; text-transform: uppercase; letter-spacing: 1px; opacity: 0.6; }
        .divider { width: 2px; height: 40px; background: var(--text-color); opacity: 0.1; border-radius: 2px; }
        .status-icon { display: flex; align-items: center; justify-content: center; font-size: 32px; line-height: 1; opacity: 0.3; transition: all 0.3s ease; }
        .status-icon.active-walk { opacity: 1; }
        .status-icon.active-run { opacity: 1; }
        .emote { height: 48px; width: auto; max-width: 48px; object-fit: contain; }
        .battery-warning { display: none; align-items: center; gap: 6px; font-size: 14px; font-weight: 800; color: #ff4444; background: rgba(255, 68, 68, 0.15); padding: 6px 12px; border-radius: 8px; animation: pulse 2s infinite; }
        .battery-warning.show-warning { display: flex; }
        @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }
    </style>
</head>
<body>
    <div class="hud-container">
        <div class="metric">
            <div class="step-value" id="steps">0</div>
            <div class="step-label">Steps</div>
        </div>
        <div class="divider"></div>
        <div class="metric">
            <div class="status-icon" id="activity">🧍</div>
        </div>
        <div class="battery-warning" id="battery-status">
            <span>🪫</span><span>LOW</span>
        </div>
    </div>
    
    <script>
        const DEBUG = )rawliteral"
#ifdef OVERLAY_DEBUG
                          "true"
#else
                          "false"
#endif
                          R"rawliteral(;
        function dbg(...args) { if (DEBUG) console.log('[stepstick]', ...args); }

        const stepsEl = document.getElementById('steps');
        const activityEl = document.getElementById('activity');
        const batteryStatus = document.getElementById('battery-status');

        let ws = null;
        let reconnectTimer = null;

        function connect() {
            if (ws && (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING)) {
                dbg('connect() called but already open/connecting, skipping');
                return;
            }
            clearTimeout(reconnectTimer);

            const url = `ws://${window.location.hostname}:81/`;
            dbg('connecting to', url);

            try {
                ws = new WebSocket(url);
            } catch (e) {
                dbg('WebSocket constructor threw:', e);
                reconnectTimer = setTimeout(connect, 3000);
                return;
            }

            ws.onopen = () => dbg('connected');

            ws.onmessage = function(event) {
                dbg('message:', event.data);
                try {
                    const data = JSON.parse(event.data);
                    if (data.command === 'reload') { window.location.reload(); return; }
                    if (data.steps !== undefined) stepsEl.innerText = data.steps;

                    if (data.activity !== undefined) {
                        if (data.activity.startsWith("http")) {
                            const img = document.createElement('img');
                            img.src = data.activity;
                            img.className = 'emote';
                            activityEl.innerHTML = '';
                            activityEl.appendChild(img);
                        } else {
                            activityEl.innerText = data.activity;
                        }
                    }

                    if (data.state !== undefined) {
                        activityEl.className = "status-icon";
                        if (data.state === 1) activityEl.classList.add("active-walk");
                        else if (data.state === 2) activityEl.classList.add("active-run");
                    }

                    if (data.battery !== undefined) {
                        if (data.battery <= 15) batteryStatus.classList.add('show-warning');
                        else batteryStatus.classList.remove('show-warning');
                    }
                } catch (e) { console.error('[stepstick] JSON parse error', e, event.data); }
            };

            ws.onerror = (e) => dbg('error', e);

            ws.onclose = (e) => {
                dbg('closed — code:', e.code, 'reason:', e.reason, 'clean:', e.wasClean);
                ws = null;
                reconnectTimer = setTimeout(connect, 2000);
            };
        }

        connect();
    </script>
</body>
</html>
)rawliteral";

// --- WEB ROUTE HANDLERS ---
void handleRoot() {
  String html = configHTML;
  html.replace("%BG%", themeBgColor);
  html.replace("%TEXT%", themeTextColor);
  html.replace("%WALK%", themeWalkColor);
  html.replace("%RUN%", themeRunColor);
  html.replace("%ISTILL%", iconStill);
  html.replace("%IWALK%", iconWalk);
  html.replace("%IRUN%", iconRun);

  html.replace("%GREY%", themeGreyscale ? "checked" : "");
  html.replace("%GLOW%", themeGlow ? "checked" : "");

  server.send(200, "text/html", html);
}

void handleOverlay() { server.send(200, "text/html", overlayHTML); }

void handleThemeCSS() {
  char css[768];
  int len = 0;

  len += snprintf(css + len, sizeof(css) - len,
                  ":root {\n--bg-color: %se6;\n--text-color: %s;\n}\n\n",
                  themeBgColor.c_str(), themeTextColor.c_str());

  len += snprintf(css + len, sizeof(css) - len,
                  ".status-icon { display: flex; align-items: center; "
                  "justify-content: center; "
                  "font-size: 32px; line-height: 1; opacity: 0.3; transition: "
                  "all 0.3s ease; "
                  "%s}\n",
                  themeGreyscale ? "filter: grayscale(100%);" : "");

  const char *walkGS = themeGreyscale ? "grayscale(0%) " : "";
  const char *runGS = themeGreyscale ? "grayscale(0%) " : "";
  const char *noFx = (!themeGreyscale && !themeGlow) ? "none" : "";

  if (themeGlow) {
    len += snprintf(css + len, sizeof(css) - len,
                    ".status-icon.active-walk { opacity: 1; filter: "
                    "%sdrop-shadow(0 0 6px %s); }\n",
                    walkGS, themeWalkColor.c_str());
    len += snprintf(css + len, sizeof(css) - len,
                    ".status-icon.active-run { opacity: 1; filter: "
                    "%sdrop-shadow(0 0 6px %s); }\n",
                    runGS, themeRunColor.c_str());
  } else {
    len += snprintf(css + len, sizeof(css) - len,
                    ".status-icon.active-walk { opacity: 1; filter: %s%s; }\n",
                    walkGS, noFx);
    len += snprintf(css + len, sizeof(css) - len,
                    ".status-icon.active-run { opacity: 1; filter: %s%s; }\n",
                    runGS, noFx);
  }

  snprintf(css + len, sizeof(css) - len,
           ".emote { height: 48px; width: auto; max-width: 48px; object-fit: "
           "contain; }\n");

  server.send(200, "text/css", css);
}

bool isValidHexColor(const String &s) {
  if (s.length() != 7 || s[0] != '#')
    return false;
  for (int i = 1; i < 7; i++) {
    char c = s[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
          (c >= 'A' && c <= 'F')))
      return false;
  }
  return true;
}

void handleSaveTheme() {
  if (server.hasArg("bgColor")) {
    String v = server.arg("bgColor");
    if (isValidHexColor(v)) {
      themeBgColor = v;
      prefs.putString("bg", themeBgColor);
    }
  }
  if (server.hasArg("textColor")) {
    String v = server.arg("textColor");
    if (isValidHexColor(v)) {
      themeTextColor = v;
      prefs.putString("text", themeTextColor);
    }
  }
  if (server.hasArg("walkColor")) {
    String v = server.arg("walkColor");
    if (isValidHexColor(v)) {
      themeWalkColor = v;
      prefs.putString("walk", themeWalkColor);
    }
  }
  if (server.hasArg("runColor")) {
    String v = server.arg("runColor");
    if (isValidHexColor(v)) {
      themeRunColor = v;
      prefs.putString("run", themeRunColor);
    }
  }

  if (server.hasArg("iconStill")) {
    iconStill = server.arg("iconStill");
    prefs.putString("istill", iconStill);
  }
  if (server.hasArg("iconWalk")) {
    iconWalk = server.arg("iconWalk");
    prefs.putString("iwalk", iconWalk);
  }
  if (server.hasArg("iconRun")) {
    iconRun = server.arg("iconRun");
    prefs.putString("irun", iconRun);
  }

  themeGlow = server.hasArg("glow");
  prefs.putBool("glow", themeGlow);

  themeGreyscale = server.hasArg("grey");
  prefs.putBool("grey", themeGreyscale);

  // Broadcast reload command to OBS before redirecting the config page
  webSocket.broadcastTXT("{\"command\":\"reload\"}");

  server.sendHeader("Location", "/?saved=1");
  server.send(303);
}

void handleReset() {
  stepOffset = hwStepCount;
  displaySteps = 0;
  lastBroadcastSteps = 0;
  server.sendHeader("Location", "/");
  server.send(303);
}

void wakeScreen() {
  if (!isScreenOn) {
    M5.Display.setBrightness(255);
    isScreenOn = true;
  }
  lastInteractionTime = millis();
}

// Fallback logic for empty string values
String getIconStr(int state) {
  if (state == 0)
    return (iconStill.length() > 0) ? iconStill : "🧍";
  if (state == 1)
    return (iconWalk.length() > 0) ? iconWalk : "🚶";
  if (state == 2)
    return (iconRun.length() > 0) ? iconRun : "🏃";
  return "🧍";
}

bool connectToWifi(const char *ssid, const char *password) {
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(100);
  }
  return WiFi.status() == WL_CONNECTED;
}

// --- MAIN FIRMWARE ---
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  prefs.begin("theme", false);
  themeBgColor = prefs.getString("bg", "#0f0f14");
  themeTextColor = prefs.getString("text", "#ffffff");
  themeWalkColor = prefs.getString("walk", "#00ffcc");
  themeRunColor = prefs.getString("run", "#ff4444");

  iconStill = prefs.getString("istill", "");
  iconWalk = prefs.getString("iwalk", "");
  iconRun = prefs.getString("irun", "");

  themeGlow = prefs.getBool("glow", true);
  themeGreyscale = prefs.getBool("grey", true);

  M5.Display.setRotation(3);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans9pt7b);

  Wire1.begin(M5.In_I2C.getSDA(), M5.In_I2C.getSCL(), 400000);
  if (imu.beginI2C(BMI2_I2C_PRIM_ADDR, Wire1) != BMI2_OK) {
    M5.Display.fillScreen(TFT_RED);
    M5.Display.drawString("IMU ERROR", M5.Display.width() / 2,
                          M5.Display.height() / 2);
    while (1) {
      delay(10);
    }
  }

  imu.enableFeature(BMI2_STEP_COUNTER);
  imu.enableFeature(BMI2_STEP_ACTIVITY);

  improvSerial.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32_S3, "StepStick",
                             FIRMWARE_VERSION, "StepStick", "http://{LOCAL_IPV4}");
  improvSerial.setCustomConnectWiFi(connectToWifi);

  M5.Display.drawString("Connecting...", M5.Display.width() / 2,
                        M5.Display.height() / 2);
  WiFi.begin();
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) {
    delay(100);
  }

  if (WiFi.status() != WL_CONNECTED) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.drawString("Connect via USB", M5.Display.width() / 2,
                          M5.Display.height() / 2 - 10);
    M5.Display.drawString("to setup WiFi", M5.Display.width() / 2,
                          M5.Display.height() / 2 + 10);
    unsigned long lastDot = 0;
    uint8_t dotCount = 0;
    const char *dots[] = {"   ", ".  ", ".. ", "..."};
    while (WiFi.status() != WL_CONNECTED) {
      improvSerial.handleSerial();
      if (millis() - lastDot >= 500) {
        dotCount = (dotCount + 1) % 4;
        M5.Display.drawString(dots[dotCount], M5.Display.width() / 2,
                              M5.Display.height() / 2 + 30);
        lastDot = millis();
      }
      delay(10);
    }
  }
  WiFi.setSleep(false);
  batteryLevel = M5.Power.getBatteryLevel();

  M5.Display.fillScreen(TFT_BLACK);
  if (MDNS.begin("stepcounter")) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
  }

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/overlay", HTTP_GET, handleOverlay);
  server.on("/theme.css", HTTP_GET, handleThemeCSS);
  server.on("/savetheme", HTTP_POST, handleSaveTheme);
  server.on("/reset", HTTP_POST, handleReset);
  server.begin();

  lastInteractionTime = millis();
}

void loop() {
  M5.update();
  improvSerial.handleSerial();
  webSocket.loop();
  server.handleClient();

  if (M5.BtnA.wasHold()) {
    stepOffset = hwStepCount;
    displaySteps = 0;
    lastBroadcastSteps = 0;
    wakeScreen();
  } else if (M5.BtnA.wasPressed()) {
    wakeScreen();
  }

  if (M5.BtnB.wasPressed()) {
    wakeScreen();
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.drawString("Clearing WiFi...", M5.Display.width() / 2,
                          M5.Display.height() / 2);
    WiFi.disconnect(true, true);
    delay(500);
    ESP.restart();
  }

  if (isScreenOn && (millis() - lastInteractionTime > SCREEN_TIMEOUT)) {
    M5.Display.setBrightness(0);
    M5.Display.fillScreen(TFT_BLACK);
    isScreenOn = false;
  }

  imu.getStepCount(&hwStepCount);
  uint8_t currentSensorActivity;
  imu.getStepActivity(&currentSensorActivity);
  if (currentSensorActivity != (uint8_t)STILL)
    currentActivity = (Activity)currentSensorActivity;

  if (currentActivity >= 1)
    lastInteractionTime = millis();

  static unsigned long lastBatteryPoll = 0;
  if (millis() - lastBatteryPoll >= 30000) {
    batteryLevel = M5.Power.getBatteryLevel();
    lastBatteryPoll = millis();
  }
  if (hwStepCount >= stepOffset)
    displaySteps = hwStepCount - stepOffset;

  activityString = getIconStr(currentActivity);

  if (displaySteps != lastBroadcastSteps || currentActivity != lastActivity) {
    String json = "{\"steps\":" + String(displaySteps) +
                  ",\"state\":" + String(currentActivity) + ",\"activity\":\"" +
                  activityString + "\"" +
                  ",\"battery\":" + String(batteryLevel) + "}";
    webSocket.broadcastTXT(json);
    lastBroadcastSteps = displaySteps;
    lastActivity = currentActivity;
  }

  static unsigned long lastDisplayUpdate = 0;
  if (isScreenOn && millis() - lastDisplayUpdate > 100) {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setFont(&fonts::FreeSansBold24pt7b);
    M5.Display.drawString(String(displaySteps) + "   ", M5.Display.width() / 2,
                          M5.Display.height() / 2 - 5);

    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Display.setTextDatum(bottom_center);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawString("http://" + WiFi.localIP().toString() + "  ",
                          M5.Display.width() / 2, M5.Display.height() - 5);
    lastDisplayUpdate = millis();
  }
}