# 📡 BL602 Log Collector

A **production-ready, real-time, multi-device logging and monitoring platform** for **PineCone BL602 IoT devices** using **MQTT aggregation + REST API + SQLite + Web Dashboard**.

Collect logs from **many embedded devices simultaneously**, store them centrally, and visualize everything live from one place.

![Python](https://img.shields.io/badge/Python-3.8+-3776AB?logo=python)
![MQTT](https://img.shields.io/badge/MQTT-Mosquitto-orange)
![Flask](https://img.shields.io/badge/Backend-Flask-black)
![SQLite](https://img.shields.io/badge/Database-SQLite-003B57)
![License](https://img.shields.io/badge/License-MIT-green)

---

# 📋 Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [Screenshots](#-screenshots)
- [System Architecture](#-system-architecture)
- [Project Structure](#-project-structure)
- [Requirements](#-requirements)
- [Installation](#-installation)
- [Running the System](#-running-the-system)
- [Device Log Format](#-device-log-format)
- [API Endpoints](#-api-endpoints)
- [Database Schema](#-database-schema)
- [Building the Device Firmware](#-building-the-device-firmware)
- [Dashboard Capabilities](#-dashboard-capabilities)
- [Troubleshooting](#-troubleshooting)
- [Security Notes](#-security-notes)
- [Future Improvements](#-future-improvements)
- [License](#-license)
- [Contributors](#-contributors)
- [Acknowledgments](#-acknowledgments)

---

# 📖 Overview

This project provides a **complete end-to-end logging pipeline**:

### Device → Server → Database → API → Dashboard

It consists of:

### 🔹 Device Firmware (C++ / FreeRTOS)
- WiFi + MQTT TLS client
- Publishes logs to `logs/<DEVICE_ID>`

### 🔹 PC Logger (Python)
- Subscribes to `logs/#`
- Classifies severity
- Extracts subsystem
- Stores to SQLite

### 🔹 REST API (Flask)
- Query logs
- Filter devices
- Provide stats
- Manage database

### 🔹 Web Dashboard
- Real-time visualization
- Filtering
- Device health
- Analytics

---

# ✨ Features

- 🚀 Multi-device support via MQTT wildcard (`logs/#`)
- 📡 Real-time log aggregation
- 📊 Interactive web dashboard
- 🧠 Automatic level classification (ERROR/WARN/INFO/DEBUG)
- 🗄️ Persistent SQLite storage with indexes
- 🔐 TLS-secured MQTT communication
- 🌐 REST API for automation & integrations
- ⚡ Lightweight, easy deployment
- 🌍 Timezone-aware timestamps

---

# 📸 Screenshots

### Dashboard – Device Specific Log Stream (BL602_9551)
![Dashboard 1](images/DASHBOARD_SCREENSHOT_1.png)

---

### Dashboard – Device Specific Log Stream (BL602_6C21)
![Dashboard 2](images/DASHBOARD_SCREENSHOT_2.png)

---

### Dashboard – Multiple Device Management
![Dashboard 3](images/DASHBOARD_SCREENSHOT_3.png)

---

### Terminal – Collector Running
![Terminal Screenshot](images/TERMINAL_SCREENSHOT.png)

---

# 🏗 System Architecture

### Data Flow
![Data Flow](images/DATA_FLOW.png)

```
BL602 Devices
     ↓ MQTT TLS
pc_logger.py → SQLite DB ← api_server.py
                               ↓
                        Web Dashboard
```

### Flow

1. Devices publish logs  
2. Logger receives + processes  
3. Data stored in SQLite  
4. API serves queries  
5. Dashboard visualizes in real time  

---

# 📁 Project Structure

### Repository Layout
![File Structure](images/FILE_STRUCTURE.png)

```
bl602-log-collector/
├── api_server.py            # Flask REST API
├── pc_logger.py             # MQTT collector & processor
├── static/index.html        # Dashboard UI
├── suas_app_mqtt/           # BL602 firmware
├── certs/                   # TLS client certificates
├── ca_certificates/         # CA files
├── device_logs.db           # SQLite DB (auto-created)
├── requirements.txt
└── README.md
```

---

# 📦 Requirements

## PC / Server
- Python 3.8+
- pip
- Mosquitto MQTT broker

## Device
- BL602 SDK
- FreeRTOS
- C++17 toolchain

---

# 🔧 Installation

### Clone
```bash
git clone https://github.com/SarthakBharad/bl602-log-collector.git
cd bl602-log-collector
```

### Install dependencies
```bash
pip install -r requirements.txt
```

### Start MQTT broker
```bash
mosquitto
```

---

# ▶ Running the System

### Terminal 1 – Start Collector
```bash
python pc_logger.py
```

### Terminal 2 – Start API + Dashboard
```bash
python api_server.py
```

Open:
```
http://localhost:5000
```

---

# 🧾 Device Log Format

### Topic
```
logs/<DEVICE_ID>
```

### Message
```
DEVICE_ID|TICK|[SUBSYSTEM] message
```

### Example
```
BL602_6C21|12345|[WIFI] Connected
```

---

# 🔌 API Endpoints

| Endpoint | Method | Purpose |
|----------|-----------|----------------|
| /api/logs | GET | Filter logs |
| /api/stats | GET | System stats |
| /api/devices | GET | Device list |
| /api/levels | GET | Log levels |
| /api/components | GET | Subsystems |
| /api/health | GET | Health check |
| /api/clear | POST | Clear logs |

Example:
```
/api/logs?limit=100&device=BL602_6C21&level=ERROR
```

---

# 🗄 Database Schema

```sql
CREATE TABLE logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT,
    device_id TEXT,
    device_tick INTEGER,
    level TEXT,
    subsystem TEXT,
    message TEXT
);
```

Indexed for fast queries.

---

# 🛠 Building the Device Firmware

### Configure WiFi
Edit:
```
suas_app_mqtt/keys.hpp
```

```cpp
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASSWORD"
```

### Build
```bash
make -f bouffalo.mk
```

### Flash
```bash
bflb_iot_tool --firmware=firmware.bin
```

---

# 🖥 Dashboard Capabilities

- Live streaming logs
- Filter by device
- Filter by severity
- Filter by subsystem
- Online/offline detection
- Expandable messages
- Color-coded levels

---

# 🧪 Troubleshooting

### Cannot connect to MQTT
- Check broker running
- Verify TLS certs
- Check port 8883

### No logs visible
- Collector running?
- Topic correct?
- Browser console errors?

### Port already used
```bash
lsof -i :5000
kill -9 <PID>
```

---

# 🔒 Security Notes

For production:

- Generate your own certificates
- Enable certificate validation
- Use strong WiFi credentials
- Run behind reverse proxy
- Add authentication if exposed publicly

---

# 🚀 Future Improvements

- WebSocket streaming
- Log export (CSV/JSON)
- Authentication
- Grafana integration
- Cloud deployment
- Log rotation

---

# 📜 License

MIT License

---

# 🤝 Contributors

- **Sarthak Bharad** — Project Lead  
- **Gopal Awasthi** — Backend Development  
- **Deepak Rajadurai** — Editor  
- **Vishant Bimbra** — Backend Development  
- **Amit Pal Singh** — Frontend & Dashboard  

---

# 🙏 Acknowledgments

### Course & Mentorship

- **Prof. Ralf Colmar Staudemeyer, PhD** — Course Professor  
- **Tobias Tefke** — Course Tutor  

### Technologies

- PineCone BL602  
- Mosquitto MQTT  
- Flask  
- SQLite  
- FreeRTOS  

---

**Status:** ✅ Production Ready  
**Tested with:** Python 3.8+, Mosquitto 2.0+, BL602 SDK  
