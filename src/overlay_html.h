#pragma once
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
                    <span>🪫</span>
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
