# 🖥️ ESP32 CSI Spatial Intelligence & Theft Radar — Web Dashboard

[![ESP32](https://img.shields.io/badge/Hardware-ESP32%20DevKit%20V1-blue?logo=espressif)](https://www.espressif.com/)
[![Protocol](https://img.shields.io/badge/Radio-Wi--Fi%20802.11n%20CSI-orange)](https://en.wikipedia.org/wiki/Channel_state_information)
[![ML Engine](https://img.shields.io/badge/ML-Random%20Forest%20(94%25%20Acc)-brightgreen?logo=scikitlearn)](https://scikit-learn.org/)
[![Telemetry](https://img.shields.io/badge/WebSocket-10%20Hz%20Bidirectional-blueviolet)](https://websockets.readthedocs.io/)
[![Frontend](https://img.shields.io/badge/UI-HTML5%20%2F%20Chart.js%204.4-cyan?logo=javascript)](https://www.chartjs.org/)
[![Audio](https://img.shields.io/badge/Audio-Web%20Audio%20API-yellow)](https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API)

> **A camera-free, privacy-preserving real-time telemetry dashboard and control center for human presence sensing, perimeter theft detection, and IoT relay actuation using 2.4 GHz Wi-Fi Channel State Information (CSI).**

---

## 📑 Table of Contents
1. [Overview](#-overview)
2. [Dashboard UI Layout](#-dashboard-ui-layout)
3. [Core Features & Capabilities](#-core-features--capabilities)
4. [Signal Processing & Metrics Displayed](#-signal-processing--metrics-displayed)
5. [Frontend & Communication Architecture](#-frontend--communication-architecture)
6. [Data Sources (Live Hardware vs Simulation)](#-data-sources)
7. [Zero-Dependency Audio Synthesizer](#-zero-dependency-audio-synthesizer)
8. [Quick Start & Launch Guide](#-quick-start--launch-guide)
9. [Configuration & Customization](#-configuration--customization)

---

## 🌟 Overview

The **ESP32 CSI Web Dashboard** serves as the graphical command-and-control center for the Wi-Fi CSI Spatial Intelligence system. Operating without cameras, PIR motion detectors, or wearable tags, it translates micro-perturbations of raw 802.11n OFDM radio waves into high-confidence spatial awareness and intrusion alerts.

The interface communicates bidirectionally over WebSockets (`:8765`) at **10 Hz**, providing zero-lag subcarrier spectrum visualization, real-time variance gauges, audio alarms, and manual/automatic hardware relay controls.

---

## 🖥️ Dashboard UI Layout

```text
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ 📡 ESP32 CSI SPATIAL INTELLIGENCE & THEFT RADAR               [🔊 Sound: ON]  [🟢 WS CONNECTED]        │
├────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ 🚨 CRITICAL THEFT INTRUSION DETECTED! (Away Mode Active)                       [ 🔕 MUTE SIREN ]       │
├────────────────────────────────────────────────────┬───────────────────────────────────────────────────┤
│ 🎮 SYSTEM CONTROLS                                 │ 📊 REAL-TIME SPATIAL METRICS                      │
│ ┌──────────────────────┐ ┌───────────────────────┐ │ ┌───────────────────────┐ ┌─────────────────────┐ │
│ │ 🏠 HOME MODE         │ │ 🛡️ AWAY MODE          │ │ │ Amplitude Var (σ_A)   │ │ Phase Disp (σ_Φ)    │ │
│ │ (Automation & Relay) │ │ (Theft Intrusion Def) │ │ │        14.28          │ │        1.42         │ │
│ └──────────────────────┘ └───────────────────────┘ │ │ [██████████░░░] 8.30  │ │ [██████████░░] 0.95 │ │
│                                                    │ └───────────────────────┘ └─────────────────────┘ │
│ Ingestion Source: [ 🔌 ESP32 Hardware | 📁 Replay] │ ML Inference: [ 94.0% ACCURACY RANDOM FOREST ]    │
├────────────────────────────────────────────────────┴───────────────────────────────────────────────────┤
│ 💡 SMART ACTUATOR & RADIO SPECTRUM                                                                    │
│ ┌───────────────────────────────────────────────┐  ┌─────────────────────────────────────────────────┐ │
│ │              💡 BULB STATUS                   │  │ 64-OFDM Baseband Subcarrier Spectrum (10 Hz)    │ │
│ │           [ STEALTH LOCK: OFF ]               │  │ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ │ │
│ └───────────────────────────────────────────────┘  └─────────────────────────────────────────────────┘ │
├────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ 📜 AUDIT & SECURITY EVENT STREAM                                                                       │
│ [12:10:45] [ALERT] Perimeter intrusion detected in AWAY MODE! Pulsating siren triggered.               │
│ [12:10:40] [MODE]  Switched to AWAY MODE (Theft Defense Protocol engaged).                             │
│ [12:08:15] [HOME]  Human presence detected — Relay GPIO 23 energized (Bulb ON).                        │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## ⚡ Core Features & Capabilities

### 1. Dual Operating Modes (1-Click Toggle)
The user can switch operational profiles dynamically without restarting the server or firmware:

| Mode | Visual Theme | Smart Light & Relay Behavior | Audio Behavior | Alert Level |
| :--- | :--- | :--- | :--- | :--- |
| **🏠 Home Mode** | Warm Amber (`#f59e0b`) | **Turns ON (Glows)** on human entry; turns **OFF** when room is vacant | Soft 2-tone chime ($880\text{ Hz} \to 1320\text{ Hz}$) | `HUMAN PRESENT` status badge |
| **🛡️ Away Mode** | Crimson Red (`#ef4444`) | **Strictly Locked OFF** (Stealth protocol — avoids alerting the intruder) | Loud frequency-modulated alarm ($600\text{ Hz} \leftrightarrow 1200\text{ Hz}$) | Full-screen flashing emergency strobe + Siren |

### 2. High-Speed 64-OFDM Spectrum Visualizer
* **Powered by Chart.js 4.4** running on an optimized HTML5 `<canvas>`.
* Renders all 64 baseband subcarriers simultaneously at **10 frames per second**.
* **Dynamic Color-Coding**:
  * 🟢 **Emerald Green (`#10b981`)**: Quiescent baseline (empty room).
  * 🌸 **Rose Pink (`#f43f5e`)**: Dynamic human motion / presence detected.
  * 🔴 **Crimson Red (`#ef4444`)**: Critical perimeter theft intrusion.

### 3. Smart Light Actuation Visualizer
* **Interactive Radial Glow**: When the relay coil closes (`GPIO 23 = LOW`), the virtual bulb displays a blooming golden aura with subtle CSS keyframe pulsing.
* **Stealth Lock Indicator**: When Away Mode is active, a dashed security ring with a padlock badge visually confirms that lighting is locked off.

### 4. Timestamped Activity & Security Audit Log
* Automatically records every change of room occupancy, operating mode switch, and theft intrusion alert.
* Color-coded category tags for quick visual triage during incident reviews.

---

## 📐 Signal Processing & Metrics Displayed

The dashboard displays live mathematical telemetry derived from the Wi-Fi physical layer:

### Amplitude Variation ($\sigma_A$)
$$\sigma_A = \frac{1}{53} \sum_{i=6}^{58} \text{std}_W(A_i)$$
* **What it measures**: The rolling standard deviation of subcarrier wave amplitudes over window $W$.
* **Significance**: Human tissue reflects and absorbs 2.4 GHz radio waves, causing high amplitude variance ($\sigma_A > 8.30$) during movement.

### Differential Phase Dispersion ($\sigma_\Phi$)
$$\sigma_\Phi = \frac{1}{52} \sum_{i=7}^{58} \text{std}_W(\Delta \theta_i)$$
* **What it measures**: The dispersion of the phase difference between adjacent subcarriers:
  $$\Delta \theta_i = \theta_i - \theta_{i-1}$$
* **Significance**: Adjacent differentiation completely eliminates **Carrier Frequency Offset (CFO)** and **Sampling Time Offset (STO)**, leaving pure Doppler turbulence caused by moving biological tissue ($\sigma_\Phi > 0.95$).

---

## 🔊 Zero-Dependency Audio Synthesizer

The dashboard requires **no external `.wav` or `.mp3` media files**. All sound effects are generated synthetically in the browser using the **Web Audio API**:

```javascript
// Browser Native Web Audio Context
const audioCtx = new (window.AudioContext || window.webkitAudioContext)();
```

* **Entry Chime (Home Mode)**: 
  A smooth dual-frequency harmonic sequence ($880\text{ Hz} \to 1320\text{ Hz}$) shaped with an exponential volume envelope.
* **Perimeter Siren (Away Mode)**:
  An aggressive two-tone frequency-modulated oscillator ($600\text{ Hz} \leftrightarrow 1200\text{ Hz}$) that pulses continuously until silenced via the **🔕 MUTE SIREN** button.
* **Audio Unlock Button**:
  Adheres to modern browser autoplay policies (Chrome/Edge/Firefox) with a dedicated top-bar audio unlock toggle.

---

## 🔌 Data Sources

The dashboard supports two ingestion pipelines selectable via the top control deck:

1. **🔌 Live ESP32 Hardware**:
   * Bridges USB Serial at **115200 baud**.
   * Continuously parses 133-value comma-separated strings containing RSSI, noise floor, 64 amplitudes, and 64 phase values.
2. **📁 CSV Simulation Replay**:
   * Replays authentic pre-recorded datasets for offline demonstrations, testing, and validation without needing physical hardware connected.
   * Scenarios include:
     * **Alternating Cycle**: Empty Room $\to$ Intruder Motion $\to$ Empty Room.
     * **Human Presence Only**: Continuous high-motion RF perturbation.
     * **Empty Room Only**: Flat quiescent baseline.

---

## 🏗️ Frontend & Communication Architecture

```
┌──────────────────────────────────────┐        ┌──────────────────────────────────────┐
│       ESP32 Hardware Receiver        │        │   Authentic Recorded CSV Datasets    │
└──────────────────┬───────────────────┘        └──────────────────┬───────────────────┘
                   │ USB Serial (115200 baud)                      │ File Reader
                   └───────────────────┐        ┌──────────────────┘
                                       ▼        ▼
                      ┌────────────────────────────────────────┐
                      │    Python Backend Server (server.py)   │
                      │                                        │
                      │  ├── Signal Processor (CFO/STO Filter) │
                      │  ├── Random Forest Inference (94.0%)   │
                      │  ├── HTTP Web Server (:5000)           │
                      │  └── WebSocket Broadcaster (:8765)     │
                      └───────────────────┬────────────────────┘
                                          │ Bidirectional JSON (10 Hz)
                                          ▼
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                        Browser Dashboard (dashboard.html)                              │
│                                                                                        │
│   ├── Mode Switcher (Home / Away) ───► Emits Mode Change to Server                     │
│   ├── Canvas Chart (Chart.js 4.4) ───► Visualizes 64 Subcarrier Baseband Spectrum     │
│   ├── Web Audio API Synthesizer   ───► Generates Chimes & Anti-Theft Sirens            │
│   ├── Dynamic VU Metric Bars      ───► Renders σ_A and σ_Φ gauges in real time        │
│   └── Event Log Stream            ───► Records timestamped security audit trail        │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 🚀 Quick Start & Launch Guide

### 1. Install Dependencies
```bash
pip install pyserial websockets scikit-learn pandas numpy joblib
```

### 2. Launch the System
Double-click **`run.bat`** (on Windows) or start the server via terminal:
```bash
python server.py
```

### 3. Access the Dashboard
Open your favorite modern web browser (Chrome, Edge, Firefox, Safari) and visit:
👉 **`http://localhost:5000/dashboard.html`**

Click **"🔊 Enable Audio"** in the top navigation bar to activate the sound alert synthesizer.

---

## ⚙️ Configuration & Customization

| Variable / Setting | Location | Default Value | Description |
| :--- | :--- | :--- | :--- |
| **HTTP Port** | `server.py` | `5000` | Port for serving `dashboard.html` |
| **WebSocket Port** | `server.py` | `8765` | Port for live telemetry streaming |
| **Update Rate** | `server.py` | `10 Hz` (`0.1s`) | Telemetry broadcast frequency |
| **Relay GPIO Pin** | `ESP32_CSI_Receiver.ino` | `GPIO 23` | Active-LOW relay coil trigger pin |
| **BSSID Lock** | `ESP32_CSI_Receiver.ino` | Enabled | Hardware filter rejecting external Wi-Fi |

---

## 📄 License & Credits
* Developed for **ESP32 Wi-Fi CSI Spatial Intelligence & Perimeter Security**.
* Open-source and intended for privacy-preserving smart automation, elder care monitoring, and perimeter defense research.
