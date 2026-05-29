/*!
 * stepstick.js — StepStick Overlay SDK
 * Connects overlays to the StepStick device over WebSocket with
 * demo mode, config cascade, watchdog reconnection, and CSS state classes.
 * https://github.com/jhlwscom/stepstick
 */
(function (global) {
  'use strict';

  var WATCHDOG_MS     = 50000; // passive timeout before forced reconnect
  var MAX_BACKOFF_MS  = 15000;
  var DEMO_TICK_MS    = 800;
  var DEMO_ICONS      = ['🧍', '🚶', '🏃'];

  function StepStick(options) {
    options = options || {};
    var params = new URLSearchParams(window.location.search);

    // --- Config cascade: stepstick-config block → URL param overrides ---
    var fileConfig = {};
    var cfgEl = document.getElementById('stepstick-config');
    if (cfgEl) {
      try { fileConfig = JSON.parse(cfgEl.textContent || '{}'); } catch (_) {}
    }
    this._cfg = Object.assign({}, fileConfig);
    var reserved = { host: 1, demo: 1 };
    params.forEach(function (v, k) { if (!reserved[k]) this._cfg[k] = v; }, this);

    // --- Host cascade: ?host= → options.host → stepstick.local ---
    this._host = params.get('host') || options.host || 'stepstick.local';

    this._listeners       = Object.create(null);
    this._ws              = null;
    this._watchdog        = null;
    this._reconnectTimer  = null;
    this._reconnectDelay  = 1000;
    this._demoMode        = params.get('demo') === 'true' || !!window.__STEPSTICK_DEMO__;

    // Demo state
    this._demoSteps    = 0;
    this._demoBattery  = 85;
    this._demoSpeed    = 1;
    this._demoInterval = null;

    if (this._demoMode) {
      this._setConnected(true);
      this._startDemoLoop();
      window.addEventListener('message', this._handlePostMessage.bind(this));
    } else {
      this._connect();
    }
  }

  // ── Public API ────────────────────────────────────────────────────────────

  StepStick.prototype.on = function (event, cb) {
    (this._listeners[event] || (this._listeners[event] = [])).push(cb);
    return this;
  };

  StepStick.prototype.off = function (event, cb) {
    if (this._listeners[event]) {
      this._listeners[event] = this._listeners[event].filter(function (fn) {
        return fn !== cb;
      });
    }
    return this;
  };

  /** Read a config value (merged from stepstick-config block + URL params). */
  StepStick.prototype.config = function (key) {
    return this._cfg[key];
  };

  // ── Private: event emitter ────────────────────────────────────────────────

  StepStick.prototype._emit = function (event, data) {
    (this._listeners[event] || []).forEach(function (cb) {
      try { cb(data); } catch (_) {}
    });
  };

  // ── Private: connection state ─────────────────────────────────────────────

  StepStick.prototype._setConnected = function (yes) {
    document.body.classList.toggle('is-connected',    !!yes);
    document.body.classList.toggle('is-disconnected', !yes);
    this._emit(yes ? 'connect' : 'disconnect', {});
  };

  // ── Private: watchdog ─────────────────────────────────────────────────────

  StepStick.prototype._resetWatchdog = function () {
    clearTimeout(this._watchdog);
    var self = this;
    this._watchdog = setTimeout(function () {
      if (self._ws) { self._ws.onclose = null; self._ws.close(); self._ws = null; }
      self._setConnected(false);
      self._scheduleReconnect();
    }, WATCHDOG_MS);
  };

  // ── Private: WebSocket ────────────────────────────────────────────────────

  StepStick.prototype._scheduleReconnect = function () {
    clearTimeout(this._reconnectTimer);
    var self = this;
    this._reconnectTimer = setTimeout(function () { self._connect(); }, this._reconnectDelay);
    this._reconnectDelay = Math.min(this._reconnectDelay * 2, MAX_BACKOFF_MS);
  };

  StepStick.prototype._connect = function () {
    var self = this;
    try {
      this._ws = new WebSocket('ws://' + this._host + ':81/');
    } catch (_) {
      this._scheduleReconnect();
      return;
    }

    this._ws.onopen = function () {
      self._reconnectDelay = 1000;
      self._setConnected(true);
      self._resetWatchdog();
    };

    this._ws.onmessage = function (ev) {
      self._resetWatchdog();
      try {
        var d = JSON.parse(ev.data);
        if (d.type === 'ping') return;                   // lazy heartbeat — reset watchdog only
        if (d.command === 'reload') { location.reload(); return; }
        self._emit('telemetry', d);
      } catch (_) {}
    };

    this._ws.onclose = function () {
      clearTimeout(self._watchdog);
      self._ws = null;
      self._setConnected(false);
      self._scheduleReconnect();
    };

    this._ws.onerror = function () {}; // onclose handles reconnect
  };

  // ── Private: demo mode ────────────────────────────────────────────────────

  StepStick.prototype._startDemoLoop = function () {
    clearInterval(this._demoInterval);
    var self = this;
    this._demoInterval = setInterval(function () {
      var state = self._demoSpeed === 0 ? 0 : self._demoSpeed < 2 ? 1 : 2;
      if (state > 0) {
        self._demoSteps += Math.max(1, Math.round((Math.random() * 2 + 1) * self._demoSpeed));
      }
      self._emit('telemetry', {
        steps:    self._demoSteps,
        state:    state,
        activity: DEMO_ICONS[state],
        battery:  self._demoBattery,
      });
    }, DEMO_TICK_MS);
  };

  StepStick.prototype._handlePostMessage = function (ev) {
    var msg = ev.data;
    if (!msg || typeof msg !== 'object') return;
    switch (msg.action) {
      case 'speed':
        this._demoSpeed = Math.max(0, isNaN(Number(msg.value)) ? 1 : Number(msg.value));
        break;
      case 'battery':
        this._demoBattery = Math.max(0, Math.min(100, Number(msg.value) || 100));
        break;
      case 'network':
        if (msg.value === 'disconnect') {
          this._setConnected(false);
          clearInterval(this._demoInterval);
          this._demoInterval = null;
        } else if (msg.value === 'connect') {
          this._setConnected(true);
          this._startDemoLoop();
        }
        break;
      case 'batch_update':
        if (msg.value && typeof msg.value === 'object') {
          Object.assign(this._cfg, msg.value);
          this._emit('config', this._cfg);
        }
        break;
    }
  };

  global.StepStick = StepStick;

}(typeof window !== 'undefined' ? window : this));
