#include "esp_pm.h"
#include "version.h"
#include <ESPmDNS.h>
#include <ImprovWiFiLibrary.h>
#include <M5Unified.h>
#include <Preferences.h>
#include <SparkFun_BMI270_Arduino_Library.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <esp_wifi.h>

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
const char *configHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Step Counter Config</title>
    <link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='.9em' font-size='90'>🦶</text></svg>">
    <style>
        :root {
            --bg-base: #0f0f14;
            --bg-card: #1c1c24;
            --bg-input: #2a2a35;
            --border: #333344;
            --accent: #00ffcc;
            --accent-hover: #00e6b8;
            --danger: #ff4444;
            --text-main: #ffffff;
            --text-muted: #888899;
            --radius: 12px;
        }

        body { 
            font-family: system-ui, -apple-system, sans-serif; 
            background: var(--bg-base); 
            color: var(--text-main); 
            padding: 20px; 
            max-width: 480px; 
            margin: 0 auto; 
            line-height: 1.5;
        }

        h1, h2, h3 { margin: 0 0 16px 0; font-weight: 600; }
        h2 { font-size: 1.25rem; border-bottom: 1px solid var(--border); padding-bottom: 12px; margin-bottom: 20px;}
        
        .card { 
            background: var(--bg-card); 
            padding: 24px; 
            border-radius: var(--radius); 
            box-shadow: 0 8px 24px rgba(0,0,0,0.2);
            margin-bottom: 24px; 
        }

        /* Forms & Inputs */
        .form-group { 
            display: flex; 
            justify-content: space-between; 
            align-items: center; 
            margin-bottom: 16px; 
            padding-bottom: 12px;
            border-bottom: 1px solid rgba(255,255,255,0.05);
        }
        .form-group:last-child { border-bottom: none; margin-bottom: 0; padding-bottom: 0; }
        .form-group.col { flex-direction: column; align-items: stretch; gap: 8px; }
        
        label { font-weight: 500; font-size: 0.95rem; }
        .help-text { font-size: 0.8rem; color: var(--text-muted); margin-top: -4px; }

        input[type="text"], select { 
            width: 100%; 
            padding: 10px 14px; 
            box-sizing: border-box; 
            background: var(--bg-input); 
            border: 1px solid var(--border); 
            color: var(--text-main); 
            border-radius: 8px; 
            font-size: 0.95rem;
            transition: border-color 0.2s;
            outline: none;
        }
        input[type="text"]:focus, select:focus { border-color: var(--accent); }

        /* Custom Color Picker */
        input[type="color"] {
            -webkit-appearance: none;
            border: none;
            width: 36px;
            height: 36px;
            border-radius: 50%;
            padding: 0;
            overflow: hidden;
            cursor: pointer;
            background: none;
        }
        input[type="color"]::-webkit-color-swatch-wrapper { padding: 0; }
        input[type="color"]::-webkit-color-swatch { border: none; border-radius: 50%; box-shadow: 0 0 0 1px var(--border) inset; }

        /* Custom Toggle Switch */
        .switch { position: relative; display: inline-block; width: 44px; height: 24px; }
        .switch input { opacity: 0; width: 0; height: 0; }
        .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: var(--bg-input); transition: .3s; border-radius: 24px; border: 1px solid var(--border); }
        .slider:before { position: absolute; content: ""; height: 18px; width: 18px; left: 2px; bottom: 2px; background-color: var(--text-muted); transition: .3s; border-radius: 50%; }
        input:checked + .slider { background-color: var(--accent); border-color: var(--accent); }
        input:checked + .slider:before { transform: translateX(20px); background-color: #000; }

        /* Buttons */
        .btn { 
            background: var(--bg-input); 
            color: var(--text-main); 
            border: 1px solid var(--border); 
            padding: 12px 20px; 
            font-size: 1rem; 
            border-radius: 8px; 
            cursor: pointer; 
            width: 100%; 
            font-weight: 600; 
            transition: all 0.2s; 
        }
        .btn:hover { background: #3a3a45; }
        .btn-save { background: var(--accent); color: #000; border: none; margin-top: 16px; box-shadow: 0 4px 12px rgba(0, 255, 204, 0.2);}
        .btn-save:hover { background: var(--accent-hover); transform: translateY(-1px); }
        .btn-danger-outline { background: transparent; color: var(--danger); border: 1px solid rgba(255,68,68,0.3); margin-top: 24px;}
        .btn-danger-outline:hover { background: rgba(255,68,68,0.1); border-color: var(--danger); }

        /* OBS Copy Box */
        .copy-box { display: flex; gap: 8px; margin-top: 12px; }
        .copy-box input { font-family: monospace; font-size: 0.85rem; }
        .btn-copy { background: var(--bg-input); border: 1px solid var(--border); color: var(--text-main); padding: 0 16px; border-radius: 8px; cursor: pointer; font-weight: 600; transition: 0.2s; white-space: nowrap;}
        .btn-copy:hover { background: #3a3a45; }
        .btn-copy.success { background: var(--accent); color: #000; border-color: var(--accent); }

        /* Tabs */
        .tabs { display: flex; background: var(--bg-base); border-radius: 8px; padding: 4px; margin-bottom: 20px; border: 1px solid var(--border);}
        .tab-btn { flex: 1; padding: 10px; text-align: center; background: transparent; color: var(--text-muted); border: none; border-radius: 6px; cursor: pointer; font-weight: 600; font-size: 0.9rem; transition: 0.2s; }
        .tab-btn:hover { color: var(--text-main); }
        .tab-btn.active { background: var(--bg-card); color: var(--accent); box-shadow: 0 2px 8px rgba(0,0,0,0.2); }
        
        .tab-pane { display: none; animation: fadeIn 0.3s ease; }
        .tab-pane.active { display: block; }
        @keyframes fadeIn { from { opacity: 0; transform: translateY(5px); } to { opacity: 1; transform: translateY(0); } }

        /* Toast */
        .toast { position: fixed; top: 24px; left: 50%; transform: translateX(-50%) translateY(-20px); background: var(--accent); color: #000; padding: 12px 24px; border-radius: 8px; font-weight: 600; opacity: 0; transition: all 0.3s cubic-bezier(0.175, 0.885, 0.32, 1.275); pointer-events: none; z-index: 100; box-shadow: 0 8px 24px rgba(0,255,204,0.3);}
        .toast.show { opacity: 1; transform: translateX(-50%) translateY(0); }
    </style>
</head>
<body>
    %MDNS_WARNING%
    <div class='card'>
        <h2>OBS Integration</h2>
        <div class="help-text">Add this URL as a Browser Source (Clear custom CSS)</div>
        <div class="copy-box">
            <input type="text" id="obs-url" readonly>
            <button class="btn-copy" onclick="copyURL()" id="copy-btn">Copy</button>
        </div>
    </div>

    <div class='card'>
        <h2>Dashboard Controls</h2>
        <button class='btn' type='button' id='btn-reset-steps'>Reset Step Count</button>
    </div>
    
    <div class='card'>
        <h2>Overlay Settings</h2>
        <form id='theme-form'>
            
            <div class="form-group col" style="margin-bottom: 24px;">
                <label>Active Overlay Mode</label>
                <select name="overlayMode" id="mode-select">
                    <option value="0" %MODE0%>Simple (Counter Only)</option>
                    <option value="1" %MODE1%>Standard (HUD)</option>
                    <option value="2" %MODE2%>Advanced (HUD + Stats)</option>
                </select>
            </div>

            <div class="form-group col" style="margin-bottom: 24px;">
                <label>Power &amp; Responsivity Mode</label>
                <div class="help-text">Controls background update speed when screen is off</div>
                <select name="powerMode">
                    <option value="0" %PWR0%>Performance (Fastest UI, High Drain)</option>
                    <option value="1" %PWR1%>Balanced (1-sec updates, Default)</option>
                    <option value="2" %PWR2%>Battery Saver (Slow updates, Max Life)</option>
                </select>
            </div>

            <div class="tabs">
                <button class="tab-btn active" data-target="tab-simple">Simple Design</button>
                <button class="tab-btn" data-target="tab-hud">HUD Design</button>
            </div>

            <div id="tab-simple" class="tab-pane active">
                <div class="form-group col">
                    <label>Font Family</label>
                    <div class="help-text">Available Google Font name (e.g. 'Roboto', 'Inter')</div>
                    <input type="text" name="simpleFont" value="%SFONT%">
                </div>
                <div class="form-group">
                    <label>Counter Color</label>
                    <input type="color" name="simpleColor" value="%SCOLOR%">
                </div>
            </div>

            <div id="tab-hud" class="tab-pane">
                <div class="form-group col">
                    <label>Still Icon</label>
                    <div class="help-text">Emoji or direct image URL (Leave blank for 🧍)</div>
                    <input type="text" name="iconStill" value="%ISTILL%">
                </div>
                <div class="form-group col">
                    <label>Walk Icon</label>
                    <div class="help-text">Emoji or direct image URL (Leave blank for 🚶)</div>
                    <input type="text" name="iconWalk" value="%IWALK%">
                </div>
                <div class="form-group col">
                    <label>Run Icon</label>
                    <div class="help-text">Emoji or direct image URL (Leave blank for 🏃)</div>
                    <input type="text" name="iconRun" value="%IRUN%">
                </div>
                
                <div class="form-group">
                    <label>Background</label>
                    <input type="color" name="bgColor" value="%BG%">
                </div>
                <div class="form-group">
                    <label>Main Text</label>
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

                <div class="form-group">
                    <div class="col">
                        <label>Enable Greyscale</label>
                        <div class="help-text">Mutes icons when standing still</div>
                    </div>
                    <label class="switch">
                        <input type="checkbox" name="grey" %GREY%>
                        <span class="slider"></span>
                    </label>
                </div>
                <div class="form-group">
                    <div class="col">
                        <label>Enable Active Glow</label>
                        <div class="help-text">Adds neon drop-shadow when moving</div>
                    </div>
                    <label class="switch">
                        <input type="checkbox" name="glow" %GLOW%>
                        <span class="slider"></span>
                    </label>
                </div>
                <div class="form-group">
                    <div class="col">
                        <label>Show Activity Emoji</label>
                        <div class="help-text">Displays the status icon in Standard/Advanced modes</div>
                    </div>
                    <label class="switch">
                        <input type="checkbox" name="emoji" %EMOJI%>
                        <span class="slider"></span>
                    </label>
                </div>
            </div>

            <button class='btn btn-save' type='submit' id='btn-save'>Save & Apply Settings</button>
        </form>
        
        <button class='btn btn-danger-outline' type='button' id='btn-reset-config'>Factory Reset Overlay</button>
    </div>

    <div class="toast" id="toast">Message</div>

    <script>
        const urlInput = document.getElementById('obs-url');
        urlInput.value = 'http://' + window.location.hostname + '/overlay';

        function showToast(msg) {
            const t = document.getElementById('toast');
            t.innerText = msg;
            t.classList.add('show');
            setTimeout(() => t.classList.remove('show'), 3000);
        }

        function copyURL() {
            urlInput.select();
            document.execCommand('copy');
            const btn = document.getElementById('copy-btn');
            btn.innerText = 'Copied!';
            btn.classList.add('success');
            setTimeout(() => { btn.innerText = 'Copy'; btn.classList.remove('success'); }, 2000);
        }

        // Tab Switching Logic
        function switchTab(targetId) {
            document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
            document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));
            document.querySelector(`[data-target="${targetId}"]`).classList.add('active');
            document.getElementById(targetId).classList.add('active');
        }

        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.addEventListener('click', (e) => {
                e.preventDefault();
                switchTab(btn.dataset.target);
            });
        });

        // Smart dropdown: changing mode automatically flips to the relevant tab
        const modeSelect = document.getElementById('mode-select');
        modeSelect.addEventListener('change', (e) => {
            if (e.target.value === "0") switchTab('tab-simple');
            else switchTab('tab-hud');
        });

        // Initialize correct tab on load based on saved mode
        if (modeSelect.value !== "0") switchTab('tab-hud');

        // Async Save Settings
        document.getElementById('theme-form').addEventListener('submit', function(e) {
            e.preventDefault();
            const btn = document.getElementById('btn-save');
            const originalText = btn.innerText;
            btn.innerText = 'Saving...';
            
            fetch('/savetheme', { method: 'POST', body: new FormData(this) })
                .then(response => {
                    btn.innerText = originalText;
                    if(response.ok) showToast("Settings Applied!");
                }).catch(() => { btn.innerText = originalText; });
        });

        // Async Reset Steps
        document.getElementById('btn-reset-steps').addEventListener('click', function() {
            fetch('/reset', { method: 'POST' }).then(response => {
                if(response.ok) showToast("Step Count Reset!");
            });
        });

        // Async Reset Customizations
        document.getElementById('btn-reset-config').addEventListener('click', function() {
            if(confirm("Are you sure you want to revert all overlay settings to default?")) {
                fetch('/resetconfig', { method: 'POST' }).then(response => {
                    if(response.ok) window.location.href = '/';
                });
            }
        });
    </script>
</body>
</html>
)rawliteral";

const char *overlayHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Inter:wght@600;800&display=swap">
    <style>
        %DYNAMIC_CSS%
        /* Reactivity Engine: 1rem dynamically scales based on OBS Browser Source width */

        html { font-size: 4vw; width: 100%; height: 100%; } 
        
        body { 
            margin: 0; 
            padding: 1.25rem; 
            background-color: transparent; 
            color: var(--text-color); 
            font-family: 'Inter', sans-serif;
            box-sizing: border-box;
            
            /* Centering Magic */
            display: flex;
            align-items: center;
            justify-content: center;
            width: 100%;
            height: 100%;
            overflow: hidden; /* Kills weird phantom OBS scrollbars */
        }
        
        /* App Container visibility logic */
        #app.mode-0 .widget-wrapper { display: none !important; }
        #app.mode-1 .simple-mode, #app.mode-1 .advanced-panel { display: none !important; }
        #app.mode-2 .simple-mode { display: none !important; }

        /* Simple Mode */
        .simple-mode { font-size: 4rem; font-weight: bold; text-shadow: 0.125rem 0.125rem 0.25rem rgba(0,0,0,0.5); }

        /* Unified Widget Wrapper */
        .widget-wrapper {
            display: inline-flex;
            flex-direction: column;
            background: var(--bg-color);
            border-radius: 1rem;
            box-shadow: 0 0.5rem 2rem rgba(0, 0, 0, 0.3);
            width: fit-content;
            overflow: hidden; /* Keeps the canvas nicely clipped to the bottom rounded corners */
        }

        /* Standard HUD (Top Section) */
        .hud-container { 
            display: flex; 
            align-items: center; 
            padding: 0.75rem 1.5rem; 
            gap: 1.5rem; 
        }
        .metric { display: flex; align-items: center; gap: 0.75rem; }
        .step-value { font-size: 2.625rem; font-weight: 800; letter-spacing: -0.0625rem; line-height: 1; }
        .step-label { font-size: 0.875rem; font-weight: 600; text-transform: uppercase; letter-spacing: 0.0625rem; opacity: 0.6; }
        .divider { width: 0.125rem; height: 2.5rem; background: var(--text-color); opacity: 0.1; border-radius: 0.125rem; }
        .status-icon { display: flex; align-items: center; justify-content: center; width: 3.75rem; height: 3.75rem; font-size: 3rem; line-height: 1; opacity: 0.3; transition: all 0.3s ease; transform: translateZ(0); will-change: filter; flex-shrink: 0; }
        .emote { width: 100%; height: 100%; object-fit: contain; transform: translateZ(0); display: block; }
        .status-icon.active-walk { opacity: 1; }
        .status-icon.active-run { opacity: 1; }
        .battery-warning { display: none; align-items: center; gap: 0.375rem; font-size: 0.875rem; font-weight: 800; color: #ff4444; background: rgba(255, 68, 68, 0.15); padding: 0.375rem 0.75rem; border-radius: 0.5rem; animation: pulse 2s infinite; }
        .battery-warning.show-warning { display: flex; }
        
        /* Advanced Panel (Bottom Section) */
        .advanced-panel { 
            padding: 0 1.5rem 1.25rem 1.5rem; 
            width: 100%;
            box-sizing: border-box;
        }
        .stats-row { display: flex; justify-content: space-between; font-size: 0.75rem; font-weight: bold; opacity: 0.7; margin-bottom: 0.5rem; text-transform: uppercase;}
        
        /* Chart takes up full container width visually, but internal resolution is high for crisp lines */
        canvas { background: rgba(0,0,0,0.2); border-radius: 0.25rem; width: 100%; height: auto; aspect-ratio: 11/3; display: block;}
        
        @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }
    </style>
</head>
<body>
    <div id="app" class="mode-%OVERLAY_MODE%">
        
        <div class="simple-mode" id="simple-steps" style="font-family: '%SFONT%'; color: %SCOLOR%;">-</div>

        <div class="widget-wrapper">
            
            <div class="hud-container">
                <div class="metric">
                    <div class="step-value" id="steps">-</div>
                    <div class="step-label">Steps</div>
                </div>
                <div class="divider" id="activity-divider"></div>
                <div class="metric" id="activity-metric">
                    <div class="status-icon" id="activity">🧍</div>
                </div>
                <div class="battery-warning" id="battery-status">
                    <span>🪫</span><span>LOW</span>
                </div>
            </div>

            <div class="advanced-panel">
                <div class="stats-row">
                    <span>Activity Rate (Last 2m) </span>
                    <span id="spm">0 SPM</span>
                </div>
                <canvas id="stepChart" width="440" height="120"></canvas>
            </div>

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
        const simpleStepsEl = document.getElementById('simple-steps');
        const activityEl = document.getElementById('activity');
        const batteryStatus = document.getElementById('battery-status');
        const spmEl = document.getElementById('spm');
        
        // Chart logic
        const canvas = document.getElementById('stepChart');
        const ctx = canvas.getContext('2d');
        const chartData = new Array(24).fill(0); // 24 data points
        let currentTotalSteps = 0;
        let lastIntervalSteps = 0;

        function updateChart() {
            // Calculate delta every 5 seconds (24 points = 2 mins history)
            let delta = currentTotalSteps - lastIntervalSteps;
            lastIntervalSteps = currentTotalSteps;
            
            chartData.shift();
            chartData.push(delta);
            
            // Calculate Steps Per Minute (sum of last 12 points = 1 min)
            let recentSum = chartData.slice(-12).reduce((a, b) => a + b, 0);
            spmEl.innerText = recentSum + " SPM";

            // Draw Chart
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            let maxVal = Math.max(...chartData, 10); // Minimum scale of 10
            
            ctx.beginPath();
            ctx.strokeStyle = getComputedStyle(document.body).getPropertyValue('--text-color');
            ctx.lineWidth = 4; // Thickened slightly to account for the high internal resolution
            
            let xStep = canvas.width / (chartData.length - 1);
            for(let i = 0; i < chartData.length; i++) {
                let x = i * xStep;
                let y = canvas.height - ((chartData[i] / maxVal) * canvas.height);
                if(i === 0) ctx.moveTo(x, y);
                else ctx.lineTo(x, y);
            }
            ctx.stroke();
            
            // Fill area
            ctx.lineTo(canvas.width, canvas.height);
            ctx.lineTo(0, canvas.height);
            ctx.fillStyle = getComputedStyle(document.body).getPropertyValue('--text-color') + '22'; // 22 hex for transparency
            ctx.fill();
        }

        setInterval(updateChart, 5000); // 5 second intervals

        // WebSocket Logic
        let ws = null;
        let reconnectTimer = null;

        function connect() {
            clearTimeout(reconnectTimer);
            const url = `ws://${window.location.hostname}:81/`;
            try { ws = new WebSocket(url); } catch (e) {
                reconnectTimer = setTimeout(connect, 3000); return;
            }

            ws.onmessage = function(event) {
                try {
                    const data = JSON.parse(event.data);
                    if (data.command === 'reload') { window.location.reload(); return; }
                    if (data.steps !== undefined) {
                        stepsEl.innerText = data.steps;
                        simpleStepsEl.innerText = data.steps;
                        currentTotalSteps = data.steps;
                        if(lastIntervalSteps === 0) lastIntervalSteps = data.steps; // init
                    }

                    if (data.activity !== undefined) {
                        if (data.activity.startsWith("http")) {
                            activityEl.innerHTML = `<img src="${data.activity}" class="emote">`;
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
                } catch (e) {}
            };
            ws.onclose = () => { ws = null; reconnectTimer = setTimeout(connect, 2000); };
        }
        connect();
    </script>
</body>
</html>
)rawliteral";

// --- WEB ROUTE HANDLERS ---
void handleRoot() {
  String html;
  html.reserve(strlen(configHTML) + 700);
  html = configHTML;

  // Insert mode selection states
  html.replace("%MODE0%", overlayMode == 0 ? "selected" : "");
  html.replace("%MODE1%", overlayMode == 1 ? "selected" : "");
  html.replace("%MODE2%", overlayMode == 2 ? "selected" : "");
  html.replace("%SCOLOR%", simpleColor);
  html.replace("%SFONT%", simpleFont);

  // Existing theme replacements
  html.replace("%BG%", themeBgColor);
  html.replace("%TEXT%", themeTextColor);
  html.replace("%WALK%", themeWalkColor);
  html.replace("%RUN%", themeRunColor);
  html.replace("%ISTILL%", iconStill);
  html.replace("%IWALK%", iconWalk);
  html.replace("%IRUN%", iconRun);
  html.replace("%GREY%", themeGreyscale ? "checked" : "");
  html.replace("%GLOW%", themeGlow ? "checked" : "");
  html.replace("%EMOJI%", themeShowEmoji ? "checked" : "");
  html.replace("%PWR0%", powerMode == 0 ? "selected" : "");
  html.replace("%PWR1%", powerMode == 1 ? "selected" : "");
  html.replace("%PWR2%", powerMode == 2 ? "selected" : "");

  String mdnsWarning = "";
  if (server.hostHeader().endsWith(".local")) {
    String ipUrl = "http://" + WiFi.localIP().toString() + "/";
    mdnsWarning = "<div style='background:#7c5a00;color:#ffe;padding:10px 16px;"
                  "font-size:13px;text-align:center'>"
                  "Tip: <a href='" + ipUrl + "' style='color:#ffd'>" + ipUrl + "</a>"
                  " loads faster than stepstick.local</div>";
  }
  html.replace("%MDNS_WARNING%", mdnsWarning);

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html", html);
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
                  "all 0.3s ease; flex-shrink: 0; transform: translateZ(0); will-change: filter; "
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
        if (secondsLeft < 0) secondsLeft = 0;
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

    if (displayDirty || displaySteps != lastDrawnSteps || batteryLevel != lastDrawnBattery) {
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