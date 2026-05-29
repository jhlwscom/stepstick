#pragma once
const char *configHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>StepStick</title>
    <link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='.9em' font-size='90'>🦶</text></svg>">
    <style>
        :root {
            --bg-base: #121212;
            --bg-card: #1e1e1e;
            --bg-input: #2a2a35;
            --border: #333344;
            --accent: #00ffcc;
            --accent-hover: #00e6b8;
            --danger: #ff4444;
            --text-main: #ffffff;
            --text-muted: #aaaabc;
            --text-hint: #888899;
            --radius: 12px;
            --shadow-card: 0 4px 8px rgba(0,0,0,0.4), 0 8px 24px rgba(0,0,0,0.3);
        }

        body {
            font-family: system-ui, -apple-system, sans-serif;
            background: var(--bg-base);
            color: var(--text-main);
            padding: 20px;
            margin: 0 auto;
            line-height: 1.5;
            box-sizing: border-box;
        }

        h2 { margin: 0 0 16px; font-size: 1.1rem; font-weight: 700; letter-spacing: 0.04em;
             text-transform: uppercase; color: var(--text-muted); border-bottom: 1px solid var(--border);
             padding-bottom: 12px; margin-bottom: 20px; }

        .card { background: var(--bg-card); padding: 24px; border-radius: var(--radius);
                box-shadow: var(--shadow-card); margin-bottom: 24px; }

        .form-group { display: flex; justify-content: space-between; align-items: center;
                      margin-bottom: 16px; padding-bottom: 12px;
                      border-bottom: 1px solid rgba(255,255,255,0.05); }
        .form-group:last-child { border-bottom: none; margin-bottom: 0; padding-bottom: 0; }
        .form-group.col { flex-direction: column; align-items: stretch; gap: 8px; }

        label { font-weight: 600; font-size: 0.95rem; }
        .help-text { font-size: 0.8rem; color: var(--text-muted); }

        select {
            width: 100%; padding: 10px 36px 10px 14px; box-sizing: border-box;
            background: var(--bg-input) url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='8' viewBox='0 0 12 8'%3E%3Cpath d='M1 1l5 5 5-5' stroke='%23aaaabc' stroke-width='2' fill='none' stroke-linecap='round'/%3E%3C/svg%3E") no-repeat right 12px center;
            -webkit-appearance: none; appearance: none;
            border: 1px solid var(--border); color: var(--text-main);
            border-radius: 8px; font-size: 0.95rem; outline: none; transition: border-color 0.2s;
        }
        select:focus { border-color: var(--accent); box-shadow: 0 0 0 3px rgba(0,255,204,0.15); }

        .btn { background: var(--bg-input); color: var(--text-main); border: 1px solid var(--border);
               padding: 12px 20px; font-size: 1rem; border-radius: 8px; cursor: pointer;
               width: 100%; font-weight: 600; transition: all 0.2s; }
        .btn:hover { background: #3a3a45; }
        .btn-save { background: var(--accent); color: #111; border: none; margin-top: 8px;
                    width: 100%; padding: 14px 20px; font-size: 1rem; border-radius: 8px;
                    cursor: pointer; font-weight: 700; transition: all 0.2s;
                    box-shadow: 0 4px 12px rgba(0,255,204,0.2); }
        .btn-save:hover { background: var(--accent-hover); transform: translateY(-1px); }
        .btn-secondary { background: transparent; color: var(--accent); border: 1px solid var(--accent);
                         padding: 10px 16px; font-size: 0.9rem; border-radius: 8px; cursor: pointer;
                         font-weight: 600; transition: all 0.2s; }
        .btn-secondary:hover { background: rgba(0,255,204,0.08); }

        /* OBS copy box */
        .copy-box { display: flex; gap: 8px; margin-top: 12px; }
        .copy-box input { flex: 1; padding: 10px 14px; background: var(--bg-input);
                          border: 1px solid var(--border); color: var(--text-main);
                          border-radius: 8px; font-family: monospace; font-size: 0.85rem; outline: none; }
        .btn-copy { background: var(--bg-input); border: 1px solid var(--border); color: var(--text-main);
                    padding: 0 16px; border-radius: 8px; cursor: pointer; font-weight: 600;
                    transition: 0.2s; white-space: nowrap; }
        .btn-copy:hover { background: #3a3a45; }
        .btn-copy.success { background: var(--accent); color: #000; border-color: var(--accent); }

        /* Toast */
        .toast { position: fixed; top: 24px; left: 50%; transform: translateX(-50%) translateY(-20px);
                 background: var(--accent); color: #000; padding: 12px 24px; border-radius: 8px;
                 font-weight: 600; opacity: 0; transition: all 0.3s cubic-bezier(0.175,0.885,0.32,1.275);
                 pointer-events: none; z-index: 100; box-shadow: 0 8px 24px rgba(0,255,204,0.3); }
        .toast.show { opacity: 1; transform: translateX(-50%) translateY(0); }

        /* mDNS tip */
        #mdns-tip { background: #7c5a00; color: #ffe8a0; padding: 10px 16px; font-size: 13px;
                    text-align: center; margin-bottom: 20px; border-radius: 8px; display: none; }
        #mdns-tip.visible { display: block; }
        #mdns-tip a { color: #ffd; }

        @media (max-width: 599px) {
            body { padding: 16px; }
            .card { padding: 20px 16px; }
        }
        @media (min-width: 600px) {
            body { padding: 24px; max-width: 560px; }
        }
        @media (min-width: 1000px) {
            body { max-width: 960px; padding: 32px; }
            #page-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 24px; align-items: start; }
            #page-grid .card { margin-bottom: 0; }
        }
    </style>
</head>
<body>
    <div id="mdns-tip"></div>
    <div id="page-grid">

        <div class="card">
            <h2>OBS Integration</h2>
            <div class="help-text">Download a theme from the gallery, then add the file as a <strong>Local</strong> Browser Source in OBS.</div>
            <div style="margin-top:16px;display:flex;gap:10px;flex-wrap:wrap">
                <a class="btn-secondary" href="https://jhlwscom.github.io/stepstick/gallery/" target="_blank" style="text-decoration:none;display:inline-block">Open Theme Gallery →</a>
            </div>
            <div style="margin-top:20px;border-top:1px solid var(--border);padding-top:16px">
                <div class="help-text">WebSocket endpoint (for custom themes / direct connection)</div>
                <div class="copy-box">
                    <input type="text" id="ws-url" readonly>
                    <button class="btn-copy" id="copy-btn" onclick="copyURL()">Copy</button>
                </div>
            </div>
        </div>

        <div class="card">
            <h2>Dashboard</h2>
            <div class="form-group">
                <div>
                    <label>Step Count</label>
                    <div class="help-text" id="live-steps">—</div>
                </div>
                <button class="btn" style="width:auto;padding:10px 16px;font-size:0.9rem" id="btn-reset-steps">Reset</button>
            </div>
        </div>

        <div class="card">
            <h2>Power Mode</h2>
            <div class="form-group col">
                <label>Background update speed</label>
                <div class="help-text">Controls how often the device wakes the radio when the screen is off.</div>
                <select id="power-select">
                    <option value="0">Performance — fastest updates, highest drain</option>
                    <option value="1">Balanced — 1-second updates (default)</option>
                    <option value="2">Battery Saver — slow updates, maximum life</option>
                </select>
            </div>
            <button class="btn-save" id="btn-save-power">Save</button>
        </div>

        <div class="card">
            <h2>Firmware Update</h2>
            <div class="form-group">
                <div>
                    <label>Installed version</label>
                    <div class="help-text" id="version-text">…</div>
                </div>
                <button class="btn" style="width:auto;padding:10px 16px;font-size:0.9rem" id="btn-check-update">Check for Updates</button>
            </div>
            <div id="ota-available" style="display:none;padding-top:4px">
                <div id="ota-msg" style="color:var(--accent);font-weight:600;margin-bottom:12px"></div>
                <button class="btn-secondary" id="btn-do-update">Install Update</button>
            </div>
            <div id="ota-current"  style="display:none;color:var(--text-muted);font-size:0.85rem;padding-top:4px"></div>
            <div id="ota-error"    style="display:none;color:var(--danger);font-size:0.85rem;padding-top:4px"></div>
            <div id="ota-progress" style="display:none;text-align:center;padding:12px 0">
                <div style="color:var(--accent);font-weight:600;margin-bottom:4px">Updating firmware…</div>
                <div style="color:var(--text-muted);font-size:0.85rem">Do not power off. Waiting for restart…</div>
            </div>
        </div>

    </div>

    <div class="toast" id="toast"></div>

    <script>
        function showToast(msg) {
            const t = document.getElementById('toast');
            t.textContent = msg;
            t.classList.add('show');
            setTimeout(() => t.classList.remove('show'), 3000);
        }

        function copyURL() {
            const el = document.getElementById('ws-url');
            el.select();
            document.execCommand('copy');
            const btn = document.getElementById('copy-btn');
            btn.textContent = 'Copied!';
            btn.classList.add('success');
            setTimeout(() => { btn.textContent = 'Copy'; btn.classList.remove('success'); }, 2000);
        }

        // Populate fields from /api/config
        let deviceVersion = 'dev';
        fetch('/api/config').then(r => r.json()).then(cfg => {
            deviceVersion = cfg.version;
            document.getElementById('version-text').textContent = 'v' + cfg.version;
            document.getElementById('ws-url').value = 'ws://' + cfg.ip + ':81/';
            document.getElementById('power-select').value = cfg.powerMode;

            if (window.location.hostname.endsWith('.local')) {
                const ipUrl = 'http://' + cfg.ip + '/';
                const w = document.getElementById('mdns-tip');
                w.classList.add('visible');
                w.innerHTML = 'Tip: <a href="' + ipUrl + '">' + ipUrl + '</a> loads faster than stepstick.local';
            }
        });

        // Live step count via WebSocket
        const wsHost = window.location.hostname;
        let liveWs = null;
        function connectLive() {
            try { liveWs = new WebSocket('ws://' + wsHost + ':81/'); } catch(_) { return; }
            liveWs.onmessage = ev => {
                try {
                    const d = JSON.parse(ev.data);
                    if (d.steps !== undefined) {
                        document.getElementById('live-steps').textContent = d.steps.toLocaleString() + ' steps';
                    }
                } catch(_) {}
            };
            liveWs.onclose = () => setTimeout(connectLive, 3000);
        }
        connectLive();

        // Reset step count
        document.getElementById('btn-reset-steps').addEventListener('click', () => {
            fetch('/reset', { method: 'POST' }).then(r => { if (r.ok) showToast('Step count reset'); });
        });

        // Save power mode
        document.getElementById('btn-save-power').addEventListener('click', () => {
            const fd = new FormData();
            fd.append('powerMode', document.getElementById('power-select').value);
            fetch('/saveconfig', { method: 'POST', body: fd })
                .then(r => { if (r.ok) showToast('Power mode saved'); });
        });

        // OTA: CalVer comparison (YYYY.M.PATCH)
        function calVerIsNewer(a, b) {
            const pa = a.split('.').map(Number), pb = b.split('.').map(Number);
            for (let i = 0; i < 3; i++) {
                if ((pa[i]||0) !== (pb[i]||0)) return (pa[i]||0) > (pb[i]||0);
            }
            return false;
        }

        let otaUpdateUrl = null;

        document.getElementById('btn-check-update').addEventListener('click', function() {
            const btn = this;
            btn.textContent = 'Checking…';
            btn.disabled = true;
            ['ota-available','ota-current','ota-error'].forEach(id => document.getElementById(id).style.display = 'none');
            fetch('https://api.github.com/repos/jhlwscom/stepstick/releases/latest', {
                headers: { 'Accept': 'application/vnd.github+json' }
            }).then(r => r.json()).then(data => {
                btn.textContent = 'Check for Updates';
                btn.disabled = false;
                const latest = data.tag_name;
                if (!latest) throw new Error('no tag');
                const needsUpdate = deviceVersion === 'dev' || calVerIsNewer(latest, deviceVersion);
                if (needsUpdate) {
                    otaUpdateUrl = 'https://github.com/jhlwscom/stepstick/releases/download/' + latest + '/firmware.bin';
                    document.getElementById('ota-msg').textContent = 'Update available: v' + latest;
                    document.getElementById('ota-available').style.display = 'block';
                } else {
                    document.getElementById('ota-current').textContent = 'Already up to date (v' + deviceVersion + ')';
                    document.getElementById('ota-current').style.display = 'block';
                }
            }).catch(() => {
                btn.textContent = 'Check for Updates';
                btn.disabled = false;
                document.getElementById('ota-error').textContent = 'Could not reach GitHub. Check internet connection.';
                document.getElementById('ota-error').style.display = 'block';
            });
        });

        document.getElementById('btn-do-update').addEventListener('click', () => {
            if (!otaUpdateUrl) return;
            document.getElementById('ota-available').style.display = 'none';
            document.getElementById('btn-check-update').disabled = true;
            document.getElementById('ota-progress').style.display = 'block';
            const fd = new FormData();
            fd.append('url', otaUpdateUrl);
            fetch('/ota/update', { method: 'POST', body: fd }).then(() => {
                const poll = setInterval(() => {
                    fetch('/', { cache: 'no-store', signal: AbortSignal.timeout(2000) })
                        .then(() => { clearInterval(poll); location.reload(); })
                        .catch(() => {});
                }, 3000);
            }).catch(() => {
                document.getElementById('ota-progress').style.display = 'none';
                document.getElementById('btn-check-update').disabled = false;
                document.getElementById('ota-error').textContent = 'Update request failed. Try again.';
                document.getElementById('ota-error').style.display = 'block';
            });
        });
    </script>
</body>
</html>
)rawliteral";
