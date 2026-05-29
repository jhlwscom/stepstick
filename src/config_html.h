#pragma once
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

        h1, h2, h3 { margin: 0 0 16px 0; font-weight: 600; }
        h2 { font-size: 1.1rem; font-weight: 700; letter-spacing: 0.04em; text-transform: uppercase; color: var(--text-muted); border-bottom: 1px solid var(--border); padding-bottom: 12px; margin-bottom: 20px; }

        .card {
            background: var(--bg-card);
            padding: 24px;
            border-radius: var(--radius);
            box-shadow: var(--shadow-card);
            margin-bottom: 24px;
        }

        .card-header { display: flex; align-items: center; justify-content: space-between; border-bottom: 1px solid var(--border); padding-bottom: 12px; margin-bottom: 20px; }
        .card-header h2 { border: none; padding: 0; margin: 0; }

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

        label { font-weight: 600; font-size: 0.95rem; color: var(--text-main); }
        .help-text { font-size: 0.8rem; color: var(--text-muted); margin-top: -4px; }
        ::placeholder { color: var(--text-hint); }

        input[type="text"] {
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
        input[type="text"]:focus { border-color: var(--accent); box-shadow: 0 0 0 3px rgba(0,255,204,0.15); }

        select {
            width: 100%;
            padding: 10px 36px 10px 14px;
            box-sizing: border-box;
            background: var(--bg-input) url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='8' viewBox='0 0 12 8'%3E%3Cpath d='M1 1l5 5 5-5' stroke='%23aaaabc' stroke-width='2' fill='none' stroke-linecap='round'/%3E%3C/svg%3E") no-repeat right 12px center;
            -webkit-appearance: none;
            -moz-appearance: none;
            appearance: none;
            border: 1px solid var(--border);
            color: var(--text-main);
            border-radius: 8px;
            font-size: 0.95rem;
            transition: border-color 0.2s;
            outline: none;
        }
        select:focus { border-color: var(--accent); box-shadow: 0 0 0 3px rgba(0,255,204,0.15); }

        /* Custom Color Picker */
        input[type="color"] {
            -webkit-appearance: none;
            border: 2px solid var(--border);
            width: 36px;
            height: 36px;
            border-radius: 50%;
            padding: 0;
            overflow: hidden;
            cursor: pointer;
            background: none;
            flex-shrink: 0;
            transition: border-color 0.2s;
        }
        input[type="color"]:hover { border-color: var(--accent); }
        input[type="color"]::-webkit-color-swatch-wrapper { padding: 0; }
        input[type="color"]::-webkit-color-swatch { border: none; border-radius: 50%; }

        /* Custom Toggle Switch */
        .switch { position: relative; display: inline-block; width: 44px; height: 24px; flex-shrink: 0; }
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
        .btn-save {
            background: var(--accent);
            color: #111;
            border: none;
            margin-top: 16px;
            width: 100%;
            padding: 14px 20px;
            font-size: 1rem;
            border-radius: 8px;
            cursor: pointer;
            font-weight: 700;
            transition: all 0.2s;
            box-shadow: 0 4px 12px rgba(0,255,204,0.2);
        }
        .btn-save:hover { background: var(--accent-hover); transform: translateY(-1px); }
        .btn-secondary {
            background: transparent;
            color: var(--accent);
            border: 1px solid var(--accent);
            padding: 10px 16px;
            font-size: 0.9rem;
            border-radius: 8px;
            cursor: pointer;
            font-weight: 600;
            transition: all 0.2s;
        }
        .btn-secondary:hover { background: rgba(0,255,204,0.08); }
        .btn-danger-outline {
            background: transparent;
            color: var(--text-muted);
            border: 1px solid rgba(255,255,255,0.12);
            margin-top: 16px;
            width: 100%;
            padding: 10px 20px;
            font-size: 0.875rem;
            border-radius: 8px;
            cursor: pointer;
            font-weight: 500;
            transition: all 0.2s;
        }
        .btn-danger-outline:hover { color: var(--danger); border-color: rgba(255,68,68,0.3); background: rgba(255,68,68,0.05); }
        .btn:focus-visible, .btn-save:focus-visible, .btn-secondary:focus-visible, .btn-danger-outline:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }

        /* OBS Copy Box */
        .copy-box { display: flex; gap: 8px; margin-top: 12px; }
        .copy-box input { font-family: monospace; font-size: 0.85rem; }
        .btn-copy { background: var(--bg-input); border: 1px solid var(--border); color: var(--text-main); padding: 0 16px; border-radius: 8px; cursor: pointer; font-weight: 600; transition: 0.2s; white-space: nowrap; }
        .btn-copy:hover { background: #3a3a45; }
        .btn-copy.success { background: var(--accent); color: #000; border-color: var(--accent); }

        /* Tabs */
        .tabs { display: flex; background: var(--bg-base); border-radius: 8px; padding: 4px; margin-bottom: 20px; border: 1px solid var(--border); }
        .tab-btn { flex: 1; padding: 10px; text-align: center; background: transparent; color: var(--text-muted); border: none; border-radius: 6px; cursor: pointer; font-weight: 600; font-size: 0.9rem; transition: 0.2s; }
        .tab-btn:hover { color: var(--text-main); }
        .tab-btn.active { background: var(--bg-card); color: var(--accent); box-shadow: 0 2px 8px rgba(0,0,0,0.2); }

        .tab-pane { display: none; animation: fadeIn 0.3s ease; }
        .tab-pane.active { display: block; }
        @keyframes fadeIn { from { opacity: 0; transform: translateY(5px); } to { opacity: 1; transform: translateY(0); } }

        /* Toast */
        .toast { position: fixed; top: 24px; left: 50%; transform: translateX(-50%) translateY(-20px); background: var(--accent); color: #000; padding: 12px 24px; border-radius: 8px; font-weight: 600; opacity: 0; transition: all 0.3s cubic-bezier(0.175, 0.885, 0.32, 1.275); pointer-events: none; z-index: 100; box-shadow: 0 8px 24px rgba(0,255,204,0.3); }
        .toast.show { opacity: 1; transform: translateX(-50%) translateY(0); }

        /* Overflow Menu */
        .overflow-menu-wrap { position: relative; display: none; }
        .btn-overflow { background: var(--bg-input); border: 1px solid var(--border); color: var(--text-muted); width: 36px; height: 36px; border-radius: 8px; cursor: pointer; font-size: 1.1rem; display: flex; align-items: center; justify-content: center; transition: 0.2s; flex-shrink: 0; }
        .btn-overflow:hover { color: var(--text-main); background: #3a3a45; }
        .overflow-menu { display: none; position: absolute; right: 0; top: calc(100% + 8px); background: var(--bg-card); border: 1px solid var(--border); border-radius: 8px; box-shadow: 0 8px 24px rgba(0,0,0,0.4); min-width: 200px; z-index: 50; overflow: hidden; }
        .overflow-menu.open { display: block; }
        .overflow-menu button { display: block; width: 100%; padding: 12px 16px; background: none; border: none; color: var(--danger); text-align: left; font-size: 0.9rem; font-weight: 600; cursor: pointer; transition: 0.15s; }
        .overflow-menu button:hover { background: rgba(255,68,68,0.1); }

        /* Modal */
        .modal-backdrop { display: none; position: fixed; inset: 0; background: rgba(0,0,0,0.7); z-index: 300; align-items: center; justify-content: center; padding: 20px; box-sizing: border-box; }
        .modal-backdrop.open { display: flex; }
        .modal-box { background: var(--bg-card); border: 1px solid var(--border); border-radius: var(--radius); padding: 28px 24px 20px; max-width: 360px; width: 100%; box-shadow: var(--shadow-card); }
        .modal-title { font-size: 1rem; font-weight: 700; margin: 0 0 8px; color: var(--text-main); }
        .modal-body { font-size: 0.875rem; color: var(--text-muted); margin: 0 0 24px; line-height: 1.6; }
        .modal-actions { display: flex; gap: 12px; }
        .modal-actions .btn-cancel { flex: 1; background: var(--bg-input); color: var(--text-main); border: 1px solid var(--border); padding: 10px 20px; border-radius: 8px; cursor: pointer; font-weight: 600; font-size: 0.9rem; transition: 0.2s; }
        .modal-actions .btn-cancel:hover { background: #3a3a45; }
        .modal-actions .btn-confirm-danger { flex: 1; background: var(--danger); color: #fff; border: none; padding: 10px 20px; border-radius: 8px; cursor: pointer; font-weight: 700; font-size: 0.9rem; transition: 0.2s; }
        .modal-actions .btn-confirm-danger:hover { background: #e03c3c; }

        /* mDNS Warning */
        #mdns-warning { background: #7c5a00; color: #ffe8a0; padding: 10px 16px; font-size: 13px; text-align: center; margin-bottom: 20px; border-radius: 8px; display: none; }
        #mdns-warning.visible { display: block; }
        #mdns-warning a { color: #ffd; }

        /* Responsive: Widget Mode */
        @media (max-width: 599px) {
            body { padding: 16px; padding-bottom: 84px; }
            .card { padding: 20px 16px; }
            #btn-save { position: fixed; bottom: 0; left: 0; right: 0; border-radius: 0; margin: 0; z-index: 100; padding: 18px 20px; box-shadow: 0 -4px 16px rgba(0,0,0,0.4); }
            #btn-reset-config { display: none; }
            .overflow-menu-wrap { display: block; }
            .form-group.color-pair { flex-direction: column; align-items: stretch; gap: 8px; }
            .form-group.color-pair input[type="color"] { width: 100%; height: 44px; border-radius: 8px; }
        }

        /* Responsive: Standard Mode */
        @media (min-width: 600px) {
            body { padding: 24px; max-width: 600px; }
            #btn-reset-config { display: block; }
            .overflow-menu-wrap { display: none; }
        }

        /* Responsive: Dashboard Mode */
        @media (min-width: 1000px) {
            body { max-width: 1040px; padding: 32px; }
            #page-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 24px; align-items: start; }
            #page-grid .card { margin-bottom: 0; }
            #card-obs { grid-column: 1; grid-row: 1; }
            #card-dashboard { grid-column: 1; grid-row: 2; }
            #card-firmware { grid-column: 1; grid-row: 3; }
            #card-overlay { grid-column: 2; grid-row: 1 / span 3; }
        }
    </style>
</head>
<body>
    <div id="mdns-warning"></div>
    <div id="page-grid">
        <div class='card' id='card-obs'>
            <h2>OBS Integration</h2>
            <div class="help-text">Add this URL as a Browser Source (Clear custom CSS)</div>
            <div class="copy-box">
                <input type="text" id="obs-url" readonly>
                <button class="btn-copy" onclick="copyURL()" id="copy-btn">Copy</button>
            </div>
        </div>

        <div class='card' id='card-dashboard'>
            <h2>Dashboard Controls</h2>
            <button class='btn' type='button' id='btn-reset-steps'>Reset Step Count</button>
        </div>

        <div class='card' id='card-firmware'>
            <h2>Firmware Update</h2>
            <div class="form-group">
                <div>
                    <label>Installed version</label>
                    <div class="help-text" id="version-text">...</div>
                </div>
                <button class="btn" style="width:auto;padding:10px 16px;font-size:0.9rem" type="button" id="btn-check-update">Check for Updates</button>
            </div>
            <div id="ota-available" style="display:none;padding-top:4px">
                <div id="ota-available-msg" style="color:var(--accent);font-weight:600;margin-bottom:12px"></div>
                <button class="btn-secondary" type="button" id="btn-do-update">Install Update</button>
            </div>
            <div id="ota-current" style="display:none;color:var(--text-muted);font-size:0.85rem;padding-top:4px"></div>
            <div id="ota-error" style="display:none;color:var(--danger);font-size:0.85rem;padding-top:4px"></div>
            <div id="ota-progress" style="display:none;text-align:center;padding:12px 0">
                <div style="color:var(--accent);font-weight:600;margin-bottom:4px">Updating firmware...</div>
                <div style="color:var(--text-muted);font-size:0.85rem">Do not power off. Waiting for restart...</div>
            </div>
        </div>

        <div class='card' id='card-overlay'>
            <div class="card-header">
                <h2>Overlay Settings</h2>
                <div class="overflow-menu-wrap">
                    <button class="btn-overflow" id="btn-overflow-trigger" type="button" aria-label="More options">&#8943;</button>
                    <div class="overflow-menu" id="overflow-menu">
                        <button type="button" id="btn-reset-config-overflow">Factory Reset Overlay</button>
                    </div>
                </div>
            </div>
            <form id='theme-form'>

                <div class="form-group col" style="margin-bottom: 24px;">
                    <label>Active Overlay Mode</label>
                    <select name="overlayMode" id="mode-select">
                        <option value="0">Simple (Counter Only)</option>
                        <option value="1">Standard (HUD)</option>
                        <option value="2">Advanced (HUD + Stats)</option>
                    </select>
                </div>

                <div class="form-group col" style="margin-bottom: 24px;">
                    <label>Power &amp; Responsivity Mode</label>
                    <div class="help-text">Controls background update speed when screen is off</div>
                    <select name="powerMode">
                        <option value="0">Performance (Fastest UI, High Drain)</option>
                        <option value="1">Balanced (1-sec updates, Default)</option>
                        <option value="2">Battery Saver (Slow updates, Max Life)</option>
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
                        <input type="text" name="simpleFont">
                    </div>
                    <div class="form-group color-pair">
                        <label>Counter Color</label>
                        <input type="color" name="simpleColor">
                    </div>
                </div>

                <div id="tab-hud" class="tab-pane">
                    <div class="form-group col">
                        <label>Still Icon</label>
                        <div class="help-text">Emoji or direct image URL (Leave blank for 🧍)</div>
                        <input type="text" name="iconStill">
                    </div>
                    <div class="form-group col">
                        <label>Walk Icon</label>
                        <div class="help-text">Emoji or direct image URL (Leave blank for 🚶)</div>
                        <input type="text" name="iconWalk">
                    </div>
                    <div class="form-group col">
                        <label>Run Icon</label>
                        <div class="help-text">Emoji or direct image URL (Leave blank for 🏃)</div>
                        <input type="text" name="iconRun">
                    </div>

                    <div class="form-group color-pair">
                        <label>Background</label>
                        <input type="color" name="bgColor">
                    </div>
                    <div class="form-group color-pair">
                        <label>Main Text</label>
                        <input type="color" name="textColor">
                    </div>
                    <div class="form-group color-pair">
                        <label>Walk Highlight</label>
                        <input type="color" name="walkColor">
                    </div>
                    <div class="form-group color-pair">
                        <label>Run Highlight</label>
                        <input type="color" name="runColor">
                    </div>

                    <div class="form-group">
                        <div class="col">
                            <label>Enable Greyscale</label>
                            <div class="help-text">Mutes icons when standing still</div>
                        </div>
                        <label class="switch">
                            <input type="checkbox" name="grey">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div class="form-group">
                        <div class="col">
                            <label>Enable Active Glow</label>
                            <div class="help-text">Adds neon drop-shadow when moving</div>
                        </div>
                        <label class="switch">
                            <input type="checkbox" name="glow">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div class="form-group">
                        <div class="col">
                            <label>Show Activity Emoji</label>
                            <div class="help-text">Displays the status icon in Standard/Advanced modes</div>
                        </div>
                        <label class="switch">
                            <input type="checkbox" name="emoji">
                            <span class="slider"></span>
                        </label>
                    </div>
                </div>

                <button class='btn-save' type='submit' id='btn-save'>Save & Apply Settings</button>
            </form>

            <button class='btn btn-danger-outline' type='button' id='btn-reset-config'>Factory Reset Overlay</button>
        </div>
    </div>

    <div class="toast" id="toast">Message</div>

    <div class="modal-backdrop" id="reset-modal">
        <div class="modal-box">
            <p class="modal-title">Factory Reset Overlay</p>
            <p class="modal-body">Revert all overlay settings to defaults? This cannot be undone.</p>
            <div class="modal-actions">
                <button class="btn-cancel" type="button" id="modal-cancel">Cancel</button>
                <button class="btn-confirm-danger" type="button" id="modal-confirm">Reset</button>
            </div>
        </div>
    </div>

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

        const modeSelect = document.getElementById('mode-select');
        modeSelect.addEventListener('change', (e) => {
            if (e.target.value === "0") switchTab('tab-simple');
            else switchTab('tab-hud');
        });

        // Fetch config from device, populate form, show mDNS hint
        let deviceVersion = 'dev';
        fetch('/api/config').then(r => r.json()).then(cfg => {
            deviceVersion = cfg.version;
            document.getElementById('version-text').textContent = 'v' + cfg.version;

            if (window.location.hostname.endsWith('.local')) {
                const ipUrl = 'http://' + cfg.ip + '/';
                const w = document.getElementById('mdns-warning');
                w.classList.add('visible');
                w.innerHTML = 'Tip: <a href="' + ipUrl + '">' + ipUrl + '</a> loads faster than stepstick.local';
            }

            document.querySelector('[name="overlayMode"]').value = cfg.overlayMode;
            document.querySelector('[name="powerMode"]').value   = cfg.powerMode;
            document.querySelector('[name="simpleFont"]').value  = cfg.simpleFont;
            document.querySelector('[name="simpleColor"]').value = cfg.simpleColor;
            document.querySelector('[name="bgColor"]').value     = cfg.bg;
            document.querySelector('[name="textColor"]').value   = cfg.text;
            document.querySelector('[name="walkColor"]').value   = cfg.walk;
            document.querySelector('[name="runColor"]').value    = cfg.run;
            document.querySelector('[name="iconStill"]').value   = cfg.iconStill;
            document.querySelector('[name="iconWalk"]').value    = cfg.iconWalk;
            document.querySelector('[name="iconRun"]').value     = cfg.iconRun;
            document.querySelector('[name="grey"]').checked  = cfg.grey;
            document.querySelector('[name="glow"]').checked  = cfg.glow;
            document.querySelector('[name="emoji"]').checked = cfg.emoji;

            if (cfg.overlayMode !== 0) switchTab('tab-hud');
        });

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
            fetch('/reset', { method: 'POST' }).then(r => { if(r.ok) showToast("Step Count Reset!"); });
        });

        // Factory Reset Modal
        function openResetModal() { document.getElementById('reset-modal').classList.add('open'); }
        function closeResetModal() { document.getElementById('reset-modal').classList.remove('open'); }

        document.getElementById('reset-modal').addEventListener('click', function(e) {
            if (e.target === this) closeResetModal();
        });
        document.getElementById('modal-cancel').addEventListener('click', closeResetModal);
        document.getElementById('modal-confirm').addEventListener('click', function() {
            closeResetModal();
            fetch('/resetconfig', { method: 'POST' }).then(r => { if(r.ok) window.location.href = '/'; });
        });
        document.getElementById('btn-reset-config').addEventListener('click', openResetModal);
        document.getElementById('btn-reset-config-overflow').addEventListener('click', function() {
            closeOverflowMenu();
            openResetModal();
        });

        // Overflow Menu
        function closeOverflowMenu() { document.getElementById('overflow-menu').classList.remove('open'); }
        document.getElementById('btn-overflow-trigger').addEventListener('click', function(e) {
            e.stopPropagation();
            document.getElementById('overflow-menu').classList.toggle('open');
        });
        document.addEventListener('click', closeOverflowMenu);

        // OTA: CalVer comparison (YYYY.M.PATCH) — done in the browser against GitHub API directly
        function calVerIsNewer(a, b) {
            const pa = a.split('.').map(Number), pb = b.split('.').map(Number);
            for (let i = 0; i < 3; i++) {
                if ((pa[i]||0) !== (pb[i]||0)) return (pa[i]||0) > (pb[i]||0);
            }
            return false;
        }

        let otaUpdateUrl = null;

        function doOtaInstall() {
            if (!otaUpdateUrl) return;
            document.body.classList.remove('banner-visible');
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
        }

        document.getElementById('btn-check-update').addEventListener('click', function() {
            const btn = this;
            btn.textContent = 'Checking...';
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
                    document.getElementById('ota-available-msg').textContent = 'Update available: v' + latest;
                    document.getElementById('ota-available').style.display = 'block';
                } else {
                    document.getElementById('ota-current').textContent = 'Already up to date (v' + deviceVersion + ')';
                    document.getElementById('ota-current').style.display = 'block';
                }
            }).catch((e) => {
                console.log(e)
                btn.textContent = 'Check for Updates';
                btn.disabled = false;
                document.getElementById('ota-error').textContent = 'Could not reach GitHub. Check internet connection.';
                document.getElementById('ota-error').style.display = 'block';
            });
        });

        document.getElementById('btn-do-update').addEventListener('click', doOtaInstall);
    </script>
</body>
</html>
)rawliteral";
