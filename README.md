# 🦶 StepStick

**A plug-and-play, local-first pedometer overlay for live streamers.**

StepStick turns a $20 M5StickS3 development board into a real-time step counter for your stream. It requires no coding, no cloud accounts, and no third-party fitness apps. Just flash it from your browser, connect it to your Wi-Fi, and add it directly to OBS.

[**🌐 Visit the Web Installer & Documentation**](https://jhlwscom.github.io/stepstick/)

<img src="docs/images/m5sticks3-main.webp" alt="M5StickS3 device" height="200"> <img src="docs/images/m5sticks3-detailed.webp" alt="M5StickS3 button layout" height="200">

<img src="docs/images/Screenshot_20260525_191421.png" alt="StepStick overlay live in OBS" width="600">

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


### Automated Dependency Management

This repository uses **Renovate** to strictly manage C++/Arduino library versions. All dependencies in `platformio.ini` are pinned by version tags, and `platformio.lock` enforces subresource integrity. PRs are grouped monthly after a 14-day community soak period to prevent supply chain attacks.

---

## AI Policy

**Code:** I used AI to help write this to get a feel for modern dev tools. I made sure to review and test the generated code before pushing it.

**Assets:** There's no AI art here. The overlays ship without icons, so if you want to add your own, please support a human artist.

---

## License

MIT — see [LICENSE](LICENSE).
