#include "esp_pm.h"
#include "version.h"
#include <ESPmDNS.h>
#include <ImprovWiFiLibrary.h>
#include <M5Unified.h>
#include <Preferences.h>
#include <SparkFun_BMI270_Arduino_Library.h>
#include <HTTPUpdate.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <esp_wifi.h>

#define GITHUB_REPO "jhlwscom/stepstick"

BMI270 imu;
WebSocketsServer webSocket = WebSocketsServer(81);
WebServer server(80);
Preferences prefs;
ImprovWiFi improvSerial(&Serial);

// --- Activity States (matches BMI270 sensor output) ---
enum Activity : uint8_t { STILL = 0, WALKING = 1, RUNNING = 2, UNKNOWN = 255 };

// --- Step Logic Variables ---
uint32_t hwStepCount = 0;
uint32_t stepOffset = 0;
uint32_t displaySteps = 0;
uint32_t lastBroadcastSteps = 0;
Activity currentActivity = STILL;
Activity lastActivity = UNKNOWN;
Activity lastSentActivity = STILL;
String activityString = "🧍";
int batteryLevel = 0;

// --- Power Management ---
unsigned long lastInteractionTime = 0;
const unsigned long SCREEN_TIMEOUT = 15000;
bool isScreenOn = true;
enum class ResetState : uint8_t { IDLE, WARNING, ARMED, HOLDING };
ResetState resetState = ResetState::IDLE;
unsigned long resetHoldStart = 0;
int resetCountdownLast = -1;
bool displayDirty = true;

// --- OTA Update ---
static bool otaUpdatePending = false;
static String otaPendingUrl;

// --- Overlay Modes ---
int overlayMode = 1; // 0 = Simple, 1 = Standard, 2 = Advanced
int powerMode = 1;   // 0 = Performance, 1 = Balanced, 2 = Battery Saver
String simpleFont;
String simpleColor;

// --- Theme Variables ---
String themeBgColor;
String themeTextColor;
String themeWalkColor;
String themeRunColor;

String iconStill;
String iconWalk;
String iconRun;

bool themeGlow = true;
bool themeGreyscale = true;
bool themeShowEmoji = true;

// --- Helper: Secure JSON Generation ---
String buildStateJson() {
  // Prevent JSON injection by escaping quotes in the activity string
  String escapedActivity = activityString;
  escapedActivity.replace("\"", "\\\"");

  char jsonBuffer[256];
  snprintf(jsonBuffer, sizeof(jsonBuffer),
           "{\"steps\":%lu,\"state\":%d,\"activity\":\"%s\",\"battery\":%d}",
           displaySteps, currentActivity, escapedActivity.c_str(),
           batteryLevel);
  return String(jsonBuffer);
}

// --- WebSocket Event Handler ---
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t length) {
  (void)payload; // Silence unused parameter warnings
  (void)length;

  if (type == WStype_CONNECTED) {
    String payload = buildStateJson();
    webSocket.sendTXT(num, payload);
  }
}

// --- HTML PAYLOADS ---
#include "config_html.h"
#include "overlay_html.h"

// --- OTA + CONFIG API ---

static String jsonEsc(const String& s) {
  String out;
  out.reserve(s.length() + 4);
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else out += c;
  }
  return out;
}

void handleApiConfig() {
  String body;
  body.reserve(6800);
  body += "{\"version\":\"";     body += FIRMWARE_VERSION;
  body += "\",\"ip\":\"";        body += WiFi.localIP().toString();
  body += "\",\"overlayMode\":"; body += overlayMode;
  body += ",\"powerMode\":";     body += powerMode;
  body += ",\"simpleFont\":\"";  body += jsonEsc(simpleFont);
  body += "\",\"simpleColor\":\""; body += simpleColor;
  body += "\",\"bg\":\"";        body += themeBgColor;
  body += "\",\"text\":\"";      body += themeTextColor;
  body += "\",\"walk\":\"";      body += themeWalkColor;
  body += "\",\"run\":\"";       body += themeRunColor;
  body += "\",\"iconStill\":\""; body += jsonEsc(iconStill);
  body += "\",\"iconWalk\":\"";  body += jsonEsc(iconWalk);
  body += "\",\"iconRun\":\"";   body += jsonEsc(iconRun);
  body += "\",\"glow\":";        body += themeGlow ? "true" : "false";
  body += ",\"grey\":";          body += themeGreyscale ? "true" : "false";
  body += ",\"emoji\":";         body += themeShowEmoji ? "true" : "false";
  body += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", body);
}

void handleOtaUpdate() {
  if (!server.hasArg("url")) {
    server.send(400, "application/json", "{\"error\":\"missing_url\"}");
    return;
  }
  String url = server.arg("url");
  String prefix = "https://github.com/" GITHUB_REPO "/releases/download/";
  if (!url.startsWith(prefix) || !url.endsWith("/firmware.bin")) {
    server.send(400, "application/json", "{\"error\":\"invalid_url\"}");
    return;
  }
  otaUpdatePending = true;
  otaPendingUrl = url;
  server.send(200, "application/json", "{\"status\":\"started\"}");
}

// GitHub release URLs redirect to objects.githubusercontent.com. HTTPUpdate reuses
// the same WiFiClientSecure across the redirect and doesn't re-establish the SSL
// session for the new host — resolve the redirect with a cheap HEAD request first.
static String resolveGitHubUrl(const String& url) {
  WiFiClientSecure c;
  c.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  const char* hdrs[] = {"Location"};
  http.collectHeaders(hdrs, 1);
  if (!http.begin(c, url)) return url;
  int code = http.sendRequest("HEAD");
  String resolved;
  if (code >= 300 && code < 400 && http.hasHeader("Location"))
    resolved = http.header("Location");
  http.end();
  return resolved.length() > 0 ? resolved : url;
}

void performOtaUpdate(const String& url) {
  M5.Display.wakeup();
  M5.Display.setBrightness(255);
  isScreenOn = true;
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Display.drawString("Updating...", M5.Display.width() / 2, M5.Display.height() / 2 - 10);
  M5.Display.drawString("Do not power off", M5.Display.width() / 2, M5.Display.height() / 2 + 10);

  String finalUrl = resolveGitHubUrl(url);

  WiFiClientSecure client;
  client.setInsecure();

  httpUpdate.rebootOnUpdate(true);
  t_httpUpdate_return ret = httpUpdate.update(client, finalUrl);

  // Only reached on failure (success reboots the device)
  String errStr = httpUpdate.getLastErrorString()
                + " (" + String(httpUpdate.getLastError()) + ")";
  M5.Display.fillScreen(TFT_RED);
  M5.Display.setTextColor(TFT_WHITE, TFT_RED);
  M5.Display.drawString("Update failed", M5.Display.width() / 2, M5.Display.height() / 2 - 10);
  M5.Display.drawString(errStr, M5.Display.width() / 2, M5.Display.height() / 2 + 10);
  delay(4000);
  ESP.restart();
}

// --- WEB ROUTE HANDLERS ---
void handleRoot() {
  server.send(200, "text/html", configHTML);
}

void handleOverlay() {
  String html;
  html.reserve(strlen(overlayHTML) + 512);
  html = overlayHTML;

  html.replace("%OVERLAY_MODE%", String(overlayMode));
  html.replace("%SFONT%", simpleFont);
  html.replace("%SCOLOR%", simpleColor);

  String dynamicCSS = ":root {\n";
  dynamicCSS += "--bg-color: " + themeBgColor + "e6;\n"; // e6 adds 90% opacity
  dynamicCSS += "--text-color: " + themeTextColor + ";\n";
  dynamicCSS += "--walk-color: " + themeWalkColor + ";\n";
  dynamicCSS += "--run-color: " + themeRunColor + ";\n";
  dynamicCSS += "}\n";

  if (!themeShowEmoji)
    dynamicCSS +=
        "#activity-metric, #activity-divider { display: none !important; }\n";

  dynamicCSS += ".status-icon { ";
  if (themeGreyscale)
    dynamicCSS += "filter: grayscale(100%); ";
  dynamicCSS += "}\n";

  String walkFilter = themeGreyscale ? "grayscale(0%) " : "";
  String runFilter = themeGreyscale ? "grayscale(0%) " : "";

  if (themeGlow) {
    walkFilter += "drop-shadow(0 0 0.5rem var(--walk-color))";
    runFilter += "drop-shadow(0 0 0.5rem var(--run-color))";
  } else if (!themeGreyscale) {
    walkFilter = "none";
    runFilter = "none";
  }

  dynamicCSS += ".status-icon.active-walk { filter: " + walkFilter + "; }\n";
  dynamicCSS += ".status-icon.active-run { filter: " + runFilter + "; }\n";

  html.replace("%DYNAMIC_CSS%", dynamicCSS);

  server.send(200, "text/html", html);
}

void handleThemeCSS() {
  char css[896];
  int len = 0;

  len += snprintf(css + len, sizeof(css) - len,
                  ":root {\n--bg-color: %se6;\n--text-color: %s;\n}\n\n",
                  themeBgColor.c_str(), themeTextColor.c_str());

  len += snprintf(css + len, sizeof(css) - len,
                  ".status-icon { display: flex; align-items: center; "
                  "justify-content: center; width: 60px; height: 60px; "
                  "font-size: 48px; line-height: 1; opacity: 0.3; transition: "
                  "all 0.3s ease; flex-shrink: 0; transform: translateZ(0); "
                  "will-change: filter; "
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
           ".emote { width: 100%%; height: 100%%; object-fit: contain; "
           "transform: translateZ(0); display: block; }\n");

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
  if (server.hasArg("overlayMode")) {
    overlayMode = server.arg("overlayMode").toInt();
    prefs.putInt("mode", overlayMode);
  }
  if (server.hasArg("simpleFont")) {
    simpleFont = server.arg("simpleFont");
    prefs.putString("sfont", simpleFont);
  }
  if (server.hasArg("simpleColor")) {
    String v = server.arg("simpleColor");
    if (isValidHexColor(v)) {
      simpleColor = v;
      prefs.putString("scolor", simpleColor);
    }
  }

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

  // Constrain input to 2048 chars to allow long URLs/Data URIs
  // while preventing NVS storage overflows (NVS max is ~4000 bytes)
  if (server.hasArg("iconStill")) {
    iconStill = server.arg("iconStill").substring(0, 2048);
    prefs.putString("istill", iconStill);
  }
  if (server.hasArg("iconWalk")) {
    iconWalk = server.arg("iconWalk").substring(0, 2048);
    prefs.putString("iwalk", iconWalk);
  }
  if (server.hasArg("iconRun")) {
    iconRun = server.arg("iconRun").substring(0, 2048);
    prefs.putString("irun", iconRun);
  }

  themeGlow = server.hasArg("glow");
  prefs.putBool("glow", themeGlow);

  themeGreyscale = server.hasArg("grey");
  prefs.putBool("grey", themeGreyscale);

  themeShowEmoji = server.hasArg("emoji");
  prefs.putBool("emoji", themeShowEmoji);

  if (server.hasArg("powerMode")) {
    powerMode = server.arg("powerMode").toInt();
    prefs.putInt("power", powerMode);
  }

  // Broadcast reload command to OBS before redirecting the config page
  webSocket.broadcastTXT("{\"command\":\"reload\"}");

  server.sendHeader("Location", "/?saved=1");
  server.send(200, "text/plain", "OK");
}

void handleReset() {
  stepOffset = hwStepCount;
  displaySteps = 0;
  lastBroadcastSteps = 0;
  server.sendHeader("Location", "/");
  server.send(200, "text/plain", "OK");
}

void wakeScreen() {
  if (!isScreenOn) {
    M5.Display.wakeup();
    M5.Display.setBrightness(255);
    isScreenOn = true;
    displayDirty = true;
  }
  lastInteractionTime = millis();
}

void drawResetWarning() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
  M5.Display.drawString("Hold B to reset", M5.Display.width() / 2,
                        M5.Display.height() / 2 - 10);
  M5.Display.drawString("WiFi credentials", M5.Display.width() / 2,
                        M5.Display.height() / 2 + 10);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
}

void drawResetCountdown(int seconds) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
  M5.Display.drawString("Resetting WiFi in", M5.Display.width() / 2,
                        M5.Display.height() / 2 - 28);
  M5.Display.setFont(&fonts::FreeSansBold24pt7b);
  M5.Display.setTextColor(TFT_RED, TFT_BLACK);
  M5.Display.drawString(String(seconds), M5.Display.width() / 2,
                        M5.Display.height() / 2 + 10);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
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

void loadPreferences() {
  overlayMode = prefs.getInt("mode", 1);
  simpleFont = prefs.getString("sfont", "Inter");
  simpleColor = prefs.getString("scolor", "#ffffff");

  themeBgColor = prefs.getString("bg", "#0f0f14");
  themeTextColor = prefs.getString("text", "#ffffff");
  themeWalkColor = prefs.getString("walk", "#00ffcc");
  themeRunColor = prefs.getString("run", "#ff4444");

  iconStill = prefs.getString("istill", "");
  iconWalk = prefs.getString("iwalk", "");
  iconRun = prefs.getString("irun", "");

  themeGlow = prefs.getBool("glow", true);
  themeGreyscale = prefs.getBool("grey", true);
  themeShowEmoji = prefs.getBool("emoji", true);
  powerMode = prefs.getInt("power", 1);
}

void handleResetConfig() {
  prefs.clear();

  loadPreferences();

  webSocket.broadcastTXT("{\"command\":\"reload\"}");

  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Power.setExtOutput(false);
  M5.Power.setLed(0);
  M5.Speaker.end();
  M5.Mic.end();
  setCpuFrequencyMhz(80);
  esp_pm_config_esp32s3_t pm_config = {
      .max_freq_mhz = 80, .min_freq_mhz = 10, .light_sleep_enable = true};
  esp_pm_configure(&pm_config);

  prefs.begin("theme", false);

  loadPreferences();

  M5.Display.setRotation(3);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans9pt7b);

#ifndef DUMMY_IMU
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
#endif

  improvSerial.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32_S3, "StepStick",
                             FIRMWARE_VERSION, "StepStick",
                             "http://{LOCAL_IPV4}");
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
  WiFi.setSleep(true);
  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
  batteryLevel = M5.Power.getBatteryLevel();

  M5.Display.fillScreen(TFT_BLACK);
  if (MDNS.begin("stepstick")) {
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
  server.on("/resetconfig", HTTP_POST, handleResetConfig);
  server.on("/api/config", HTTP_GET, handleApiConfig);
  server.on("/ota/update", HTTP_POST, handleOtaUpdate);
  server.begin();

  lastInteractionTime = millis();
}

void loop() {
  if (otaUpdatePending) {
    otaUpdatePending = false;
    performOtaUpdate(otaPendingUrl);
  }

  M5.update();
  improvSerial.handleSerial();
  webSocket.loop();
  server.handleClient();

  if (M5.BtnA.wasHold()) {
    stepOffset = hwStepCount;
    displaySteps = 0;
    lastBroadcastSteps = 0;
    resetState = ResetState::IDLE;
    displayDirty = true;
    wakeScreen();
  } else if (M5.BtnA.wasPressed()) {
    resetState = ResetState::IDLE;
    displayDirty = true;
    wakeScreen();
  }

  // WiFi reset state machine — requires release between warning and hold
  switch (resetState) {
  case ResetState::IDLE:
    if (M5.BtnB.wasPressed()) {
      wakeScreen();
      drawResetWarning();
      resetState = ResetState::WARNING;
      resetCountdownLast = -1;
    }
    break;

  case ResetState::WARNING:
    // Wait for button to be released before arming
    if (!M5.BtnB.isPressed()) {
      resetState = ResetState::ARMED;
    }
    break;

  case ResetState::ARMED:
    if (M5.BtnB.isPressed()) {
      resetHoldStart = millis();
      resetCountdownLast = -1;
      resetState = ResetState::HOLDING;
    }
    break;

  case ResetState::HOLDING:
    if (!M5.BtnB.isPressed()) {
      // Released before countdown finished — stay at warning screen
      drawResetWarning();
      resetState = ResetState::ARMED;
      resetCountdownLast = -1;
    } else {
      unsigned long elapsed = millis() - resetHoldStart;
      int secondsLeft = 3 - (int)(elapsed / 1000);
      if (secondsLeft < 0)
        secondsLeft = 0;
      if (secondsLeft != resetCountdownLast) {
        resetCountdownLast = secondsLeft;
        drawResetCountdown(secondsLeft);
      }
      if (elapsed >= 3000) {
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setFont(&fonts::FreeSans9pt7b);
        M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Display.drawString("Clearing WiFi...", M5.Display.width() / 2,
                              M5.Display.height() / 2);
        WiFi.disconnect(true, true);
        delay(500);
        ESP.restart();
      }
    }
    break;
  }

  if (isScreenOn && (millis() - lastInteractionTime > SCREEN_TIMEOUT)) {
    M5.Display.setBrightness(0);
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.sleep();
    isScreenOn = false;
  }

  unsigned long imuPollRate;
  int activeSleep, backgroundSleep;
  if (powerMode == 0) {
    imuPollRate = 100;
    activeSleep = 10;
    backgroundSleep = 50;
  } else if (powerMode == 1) {
    imuPollRate = 1000;
    activeSleep = 20;
    backgroundSleep = 250;
  } else {
    imuPollRate = 2000;
    activeSleep = 50;
    backgroundSleep = 1000;
  }

  static unsigned long lastImuPoll = 0;
  if (millis() - lastImuPoll >= imuPollRate) {
    lastImuPoll = millis();
#ifdef DUMMY_IMU
    // Simulate a brisk walk at ~120 steps/min (2 steps/sec) for battery testing
    {
      static unsigned long dummyLastMs = 0;
      static float dummyAccum = 0.0f;
      static bool dummyReady = false;
      unsigned long nowMs = millis();
      if (!dummyReady) {
        dummyLastMs = nowMs;
        dummyReady = true;
      } else {
        dummyAccum += (nowMs - dummyLastMs) / 1000.0f * 2.0f;
        dummyLastMs = nowMs;
        if (dummyAccum >= 1.0f) {
          uint32_t newSteps = (uint32_t)dummyAccum;
          hwStepCount += newSteps;
          dummyAccum -= (float)newSteps;
        }
      }
    }
    currentActivity = WALKING;
#else
    imu.getStepCount(&hwStepCount);
    uint8_t currentSensorActivity;
    imu.getStepActivity(&currentSensorActivity);
    currentActivity = (Activity)currentSensorActivity;
#endif
  }

  static unsigned long lastBatteryPoll = 0;
  if (millis() - lastBatteryPoll >= 30000) {
    batteryLevel = M5.Power.getBatteryLevel();
    lastBatteryPoll = millis();
  }
  if (hwStepCount >= stepOffset)
    displaySteps = hwStepCount - stepOffset;

  activityString = getIconStr(currentActivity);

  static int batchThreshold = random(8, 15);
  if (displaySteps != lastBroadcastSteps || currentActivity != lastActivity) {

    // Only broadcast if the state changed, OR if we hit our jitter threshold
    if (currentActivity != lastActivity ||
        abs((int)(displaySteps - lastBroadcastSteps)) >= batchThreshold) {

      String payload = buildStateJson();
      webSocket.broadcastTXT(payload);

      lastBroadcastSteps = displaySteps;
      lastActivity = currentActivity;

      batchThreshold = random(8, 15);
    }
  }

  if (resetState != ResetState::IDLE)
    return; // hold display until reset flow is dismissed or confirmed
  if (isScreenOn) {
    static uint32_t lastDrawnSteps = UINT32_MAX;
    static int lastDrawnBattery = -1;

    if (displayDirty || displaySteps != lastDrawnSteps ||
        batteryLevel != lastDrawnBattery) {
      displayDirty = false;
      lastDrawnSteps = displaySteps;
      lastDrawnBattery = batteryLevel;

      M5.Display.fillScreen(TFT_BLACK);

      // --- 1. Draw Steps (Center) ---
      M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
      M5.Display.setTextDatum(middle_center);
      M5.Display.setFont(&fonts::FreeSansBold24pt7b);
      M5.Display.drawString(String(displaySteps), M5.Display.width() / 2,
                            M5.Display.height() / 2 - 5);

      // --- 2. Draw IP Address (Bottom) ---
      M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
      M5.Display.setTextDatum(bottom_center);
      M5.Display.setFont(&fonts::FreeSans9pt7b);
      M5.Display.drawString("http://" + WiFi.localIP().toString(),
                            M5.Display.width() / 2, M5.Display.height() - 5);

      // --- 3. Draw Battery (Top Right) ---
      if (batteryLevel <= 20) {
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
      } else {
        M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
      }
      M5.Display.setTextDatum(top_right);
      M5.Display.setFont(&fonts::FreeSans9pt7b);
      M5.Display.drawString(String(batteryLevel) + "%", M5.Display.width() - 5,
                            5);

      // --- 4. Draw CalVer Firmware Version (Top Left) ---
      // Using a muted grey so it stays out of the way of the main metrics
      M5.Display.setTextColor(TFT_DARKGRAY, TFT_BLACK);
      M5.Display.setTextDatum(top_left);
      M5.Display.setFont(&fonts::FreeSans9pt7b);
      M5.Display.drawString(String("v") + FIRMWARE_VERSION, 5, 5);
    }
  }

  if (isScreenOn) {
    vTaskDelay(pdMS_TO_TICKS(activeSleep));
  } else {
    vTaskDelay(pdMS_TO_TICKS(backgroundSleep));
  }
}
