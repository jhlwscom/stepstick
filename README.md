# 🦶 StepStick

**A plug-and-play, local-first pedometer overlay for live streamers.**

StepStick turns a $20 M5StickS3 development board into a real-time step counter for your stream. It requires no coding, no cloud accounts, and no third-party fitness apps. Just flash it from your browser, connect it to your Wi-Fi, and add it directly to OBS.

[**🌐 Visit the Web Installer & Documentation**](https://jhlwscom.github.io/stepstick/)

---

## ✨ Features

* **100% Local & Private:** Processes all movement data on the device and sends it directly to OBS over your local network. No telemetry, no subscriptions.
* **Native OBS Integration:** Runs an onboard web server. Add it to OBS as a standard Browser Source with zero delay.
* **Smart Activity Tracking:** Uses the BMI270 IMU to automatically detect whether you are standing still, walking, or running, updating the stream graphic instantly.
* **Browser Installation:** Uses `esp-web-tools` and the Improv Wi-Fi standard. Flash the firmware and connect to your network directly from Chrome or Edge via USB.
* **Stream-Ready Themes:** Customise colours, icons, and glow effects via the on-device web UI (`http://stepstick.local`) to match your branding.

---

## 🎮 For Streamers: Quick Start

You do not need to download any code to use StepStick. 

1. **Get the Hardware:** Purchase an **M5Stack StickS3** (ESP32-S3).
2. **Install the Software:** Plug the device into your PC via USB. Go to the [StepStick Web Installer](https://jhlwscom.github.io/stepstick/flash.html), and click "Install".
3. **Connect to Wi-Fi:** After installation, click "Configure Wi-Fi" in the browser prompt to securely connect the tracker to your home network.
4. **Add to OBS:** Navigate to `http://stepstick.local`, copy your overlay URL, and paste it as a **Browser Source** in OBS. 

For the complete setup guide, visit the [Setup Page](https://jhlwscom.github.io/stepstick/setup.html).

---

## 💻 For Developers: Building from Source

StepStick is built using the Arduino framework on top of PlatformIO. It is designed with a strict, reproducible build environment.

### Prerequisites
* [VS Code](https://code.visualstudio.com/) with the PlatformIO extension.
* Python 3.13 (recommended).

### Build Instructions
1. **Clone the repository:**
```bash
   git clone [https://github.com/jhlwscom/stepstick.git](https://github.com/jhlwscom/stepstick.git)
   cd stepstick

```

2. **Set up the hermetic build environment:**
We enforce a specific version of the PlatformIO core to prevent toolchain drift.

```bash
   pip install -r requirements.txt

```

3. **Build and Upload:**

```bash
   pio run -e m5stack-sticks3 --target upload

```

### Architecture Overview

StepStick is engineered for low latency and high battery efficiency on the 250mAh M5StickS3:

* **Improv Wi-Fi:** Replaces heavy Captive Portal HTTP servers with Web Serial provisioning, saving heap memory and streamlining onboarding.
* **Event-Driven WebSockets:** Instead of OBS polling the device via HTTP (which drains the battery), the ESP32 acts as a WebSocket server (Port 81). It only broadcasts payloads when a step is registered or an activity state transitions.
* **Dynamic CSS Injection:** Theme preferences are stored in ESP32 NVS. The `/theme.css` endpoint generates a stylesheet dynamically. Saving a new theme pushes a `{"command": "reload"}` WebSocket payload to OBS, refreshing the overlay instantly.

### Automated Dependency Management

This repository uses **Renovate** to strictly manage C++/Arduino library versions. All dependencies in `platformio.ini` are pinned by version tags, and `platformio.lock` enforces subresource integrity. PRs are grouped monthly after a 14-day community soak period to prevent supply chain attacks.

---

