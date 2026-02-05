# BL602 Log Collector

A production-ready, multi-device logging and monitoring system for PineCone BL602 IoT devices with a centralized MQTT log aggregation server, REST API backend, and web-based real-time dashboard.

## Overview

This project provides a complete solution for collecting, storing, and visualizing logs from multiple BL602 devices in real-time. It consists of:

- **Device-side application**: C++ firmware with WiFi and MQTT connectivity for log transmission via `logs/<DEVICE_ID>` topic
- **PC Logger Server**: Python-based MQTT subscriber with server-side log processing (timestamp assignment, level classification, subsystem extraction)
- **REST API Server**: Flask-based API for querying, filtering, and managing logs with SQLite persistence
- **Web Dashboard**: Interactive, responsive frontend for real-time log visualization and filtering

## Key Features

- 🚀 **Multi-Device Support**: Wildcard MQTT topic subscription (`logs/#`) for unlimited device connections
- 📊 **Real-time Processing**: Server-side log level classification (ERROR, WARN, INFO, DEBUG) and subsystem extraction
- 🔐 **Secure Communication**: TLS/SSL encrypted MQTT with certificate-based authentication
- 🌐 **RESTful API**: Comprehensive endpoints for logs, statistics, device management, and filtering
- 💻 **Modern Web Dashboard**: Real-time filtering by device, log level, subsystem, and device tick
- 📈 **Advanced Analytics**: Database statistics, device online/offline status, log aggregation
- 🗄️ **Persistent Storage**: SQLite database with indexed schema for fast queries
- 🔒 **CORS-enabled**: Cross-origin requests for flexible frontend integration
- 🌍 **Timezone Support**: Configurable timezone (default: Europe/Berlin) for timestamp accuracy

## Project Structure

```
bl602-log-collector/
├── api_server.py                    # Flask REST API server (port 5000)
├── pc_logger.py                     # MQTT log aggregator & processor
├── static/
│   └── index.html                   # Web dashboard (responsive, real-time)
├── suas_app_mqtt/                   # BL602 device firmware (C++)
│   ├── main.cpp                     # Application entry point
│   ├── mqtt.cpp/h                   # MQTT client with TLS support
│   ├── wifi.cpp/h                   # WiFi connectivity (FreeRTOS)
│   ├── log.cpp/h                    # Smart logging system
│   ├── keys.hpp                     # WiFi credentials and configuration
│   └── bouffalo.mk                  # Build configuration for BL602 SDK
├── certs/                           # MQTT client certificates
│   ├── client.crt/.key              # Client certificate and private key
│   └── mosquitto.*                  # Mosquitto broker certificates
├── ca_certificates/                 # Certificate authority files
│   ├── ca.crt/.der                  # CA certificate
│   ├── private.pem                  # Private key
│   └── ca.srl                       # Serial number file
├── device_logs.db                   # SQLite database (auto-created)
└── test_config.conf                 # Configuration reference
```

## Prerequisites

### For PC Logger and API Server
- **Python 3.8+**
- **pip** (Python package manager)
- **MQTT Broker** (Mosquitto recommended, running on localhost:8883 with TLS)

### For BL602 Device Application
- **BL602 SDK** (with build tools)
- **FreeRTOS** support
- **C++17 compiler** or compatible
- **MQTT/TLS libraries**

## Installation & Setup

### 1. Clone the Repository

```bash
git clone https://github.com/SarthakBharad/bl602-log-collector.git
cd bl602-log-collector
```

### 2. Install Python Dependencies

```bash
pip install flask flask-cors paho-mqtt
```

Or with requirements file:
```bash
pip install -r requirements.txt
```

### 3. Configure MQTT Broker

Ensure your MQTT broker (Mosquitto) is running with TLS:
```bash
# Default: localhost:8883 (TLS)
# Certificates should be in: ca_certificates/ and certs/
```

### 4. Start the Services

**Terminal 1 - Start the Log Collector:**
```bash
python pc_logger.py
```

Expected output:
```
============================================================
  PineCone BL602 Centralized Log Collector
  Multi-Device Support Enabled
============================================================
✅ Database initialized: device_logs.db
🔌 Connecting to localhost:8883 ...
✅ Connected to MQTT broker
📡 Subscribed to: logs/# (wildcard - receives from ALL devices)
```

**Terminal 2 - Start the API Server (in the same directory):**
```bash
python api_server.py
```

Expected output:
```
🚀 Starting REST API server...
📡 API available at: http://localhost:5000
🌐 Frontend available at: http://localhost:5000

Available endpoints:
  GET  /api/logs       - Get logs with filtering
  GET  /api/stats      - Get statistics
  GET  /api/devices    - Get device list
  GET  /api/levels     - Get log levels
  GET  /api/components - Get components
  POST /api/clear      - Clear all logs
  GET  /api/health     - Health check
```

### 5. Access the Dashboard

Open your browser and navigate to: **http://localhost:5000**

## Log Format & Device Setup

### Device Log Format

Devices should publish to: `logs/<DEVICE_ID>`

Message format: `DEVICE_ID|TICK|[SUBSYSTEM] message`

Example:
```
logs/BL602_6C21 → "BL602_6C21|12345|[WIFI] Connected to network"
logs/BL602_A3F5 → "BL602_A3F5|98765|[MQTT] Published to topic: sensor/temperature"
```

### Device ID Extraction

Device ID is extracted in priority order:
1. From MQTT topic: `logs/BL602_6C21` → `BL602_6C21`
2. From message prefix: `BL602_A3F5|...|...` → `BL602_A3F5`
3. Fallback: `unknown_device`

### Device Tick (Uptime Counter)

Optional counter showing device uptime in milliseconds. Example: `12345` = 12.345 seconds

## API Endpoints

### Get Logs
```
GET /api/logs?limit=100&device=BL602_6C21&level=ERROR&component=WIFI
```
**Parameters:**
- `limit` (int): Number of logs to return (default: 100)
- `device` (str): Filter by device ID
- `level` (str): Filter by log level (ERROR, WARN, INFO, DEBUG)
- `component` (str): Filter by component/subsystem

**Response:**
```json
[
  {
    "id": 1,
    "timestamp": "2026-01-27T12:34:56.789",
    "device_id": "BL602_6C21",
    "level": "INFO",
    "component": "WIFI",
    "message": "[WIFI] Connected to network"
  }
]
```

### Get Statistics
```
GET /api/stats
```
**Response:**
```json
{
  "total_logs": 1543,
  "total_devices": 3,
  "online_devices": 2
}
```

### Get Devices
```
GET /api/devices
```
**Response:**
```json
[
  {
    "device_id": "BL602_6C21",
    "log_count": 512,
    "last_seen": "2026-01-27T12:34:56.789",
    "status": "online"
  },
  {
    "device_id": "BL602_A3F5",
    "log_count": 789,
    "last_seen": "2026-01-27T12:30:00.000",
    "status": "offline"
  }
]
```

### Get Log Levels
```
GET /api/levels
```
**Response:** `["DEBUG", "ERROR", "INFO", "WARN"]`

### Get Components/Subsystems
```
GET /api/components
```
**Response:** `["MQTT", "NETWORK", "SYSTEM", "WIFI"]`

### Clear All Logs
```
POST /api/clear
```
**Response:** `{"message": "All logs cleared successfully"}`

### Health Check
```
GET /api/health
```
**Response:**
```json
{
  "status": "healthy",
  "timestamp": "2026-01-27T12:34:56.789123"
}
```

## Database Schema

### Logs Table

```sql
CREATE TABLE logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,              -- ISO format with timezone
    device_id TEXT NOT NULL,              -- Device identifier
    device_tick INTEGER DEFAULT 0,        -- Device uptime (milliseconds)
    level TEXT NOT NULL,                  -- ERROR, WARN, INFO, DEBUG
    subsystem TEXT NOT NULL DEFAULT 'SYSTEM',  -- WIFI, MQTT, SYSTEM, etc.
    message TEXT NOT NULL                 -- Full log message
);

-- Indexes for fast queries
CREATE INDEX idx_device ON logs(device_id);
CREATE INDEX idx_level ON logs(level);
CREATE INDEX idx_subsystem ON logs(subsystem);
CREATE INDEX idx_timestamp ON logs(timestamp);
```

## Log Level Classification

Server-side automatic detection based on message keywords:

| Level | Keywords | Emoji |
|-------|----------|-------|
| ERROR | error, failed, panic, fatal, fault, exception, crash, out of memory, err_ | 🔴 |
| WARN | warn, warning, retry, timeout, unstable, reconnect, disconnect, lost, slow | 🟡 |
| DEBUG | debug, trace, verbose | ⚪ |
| INFO | (default) | 🟢 |

### Subsystem Extraction

Extracted from message prefix or content:

- `[WIFI]` → WIFI
- `[MQTT]` → MQTT
- Memory-related messages → MEMORY
- TCP/IP/Network messages → NETWORK
- Boot/Start/Init messages → SYSTEM

## Configuration

### PC Logger Configuration (pc_logger.py)

Edit these variables at the top of the file:

```python
BROKER_IP = "localhost"          # MQTT broker address
PORT = 8883                      # MQTT broker port (TLS)
TOPIC = "logs/#"                 # Subscribe to all devices
DB_FILE = "device_logs.db"       # SQLite database location
TIMEZONE = "Europe/Berlin"       # Timezone for timestamps
DEFAULT_DEVICE_ID = "unknown_device"  # Fallback device ID
```

### API Server Configuration (api_server.py)

```python
DB_FILE = "device_logs.db"       # Must match pc_logger.py
# Server runs on: http://0.0.0.0:5000
```

### MQTT TLS Certificates

Place certificates in the project root:
- `ca.crt` - Certificate Authority
- `certs/client.crt` - Client certificate
- `certs/client.key` - Client private key

## Building the Device Application

### Step 1: Configure WiFi Credentials

Edit `suas_app_mqtt/keys.hpp`:

```cpp
#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"
```

### Step 2: Build with BL602 SDK

```bash
cd suas_app_mqtt
# With BL602 toolchain in path:
make -f bouffalo.mk
```

### Step 3: Flash to BL602 Device

```bash
# Use your BL602 flashing tool
bflb_iot_tool --chipname=bl602 --port=/dev/ttyUSB0 --firmware=firmware.bin
```

The device will automatically connect to WiFi and publish logs to `logs/<DEVICE_ID>`

## Web Dashboard Features

- **Real-time Log Viewer**: Automatically updates as logs arrive
- **Advanced Filtering**:
  - Filter by Device ID
  - Filter by Log Level (ERROR, WARN, INFO, DEBUG)
  - Filter by Component/Subsystem
  - Limit number of results (10-1000)
- **Statistics Panel**: Total logs, devices, and online devices
- **Device Status**: View all devices with online/offline status
- **Log Details**: Click to expand and view full message context
- **Color-coded Levels**: Visual indicators for log severity

## Troubleshooting

### PC Logger Issues

**Error: "Connection failed to localhost:8883"**
- Ensure MQTT broker is running: `mosquitto -c /path/to/mosquitto.conf`
- Check certificates are in the correct location
- Verify firewall allows port 8883

**Error: "Could not parse: ..." (warnings)**
- Device is sending logs in unexpected format
- Check device firmware log format matches expected: `DEVICE_ID|TICK|message`

### API Server Issues

**Error: "Address already in use" (port 5000)**
```bash
# Find and kill process using port 5000
lsof -i :5000
kill -9 <PID>
```

**Database locked error**
- Ensure only one `pc_logger.py` and one `api_server.py` are running
- Close and restart both services

### Web Dashboard Issues

**Dashboard loads but shows no logs**
- Verify `pc_logger.py` is running and connected to MQTT
- Check browser console (F12) for JavaScript errors
- Ensure API server is running on port 5000

**Devices showing as "offline" immediately**
- Check device is publishing logs (should appear every few seconds)
- Verify device is actually connected to WiFi
- Check MQTT topic matches: `logs/<DEVICE_ID>`

### Device Connection Issues

**BL602 not connecting to WiFi**
- Verify SSID and password in `keys.hpp`
- Check device is within WiFi range
- Ensure WiFi is 2.4GHz (BL602 doesn't support 5GHz)

**Logs not appearing on PC Logger**
- Verify MQTT broker is accepting connections
- Check device topic: should be `logs/<DEVICE_ID>`
- Verify log format: `DEVICE_ID|TICK|[SUBSYSTEM] message`

## Performance Notes

- **Database**: SQLite can handle ~100k logs efficiently with proper indexing
- **MQTT**: Wildcard topic subscription (`logs/#`) scales to multiple devices
- **API**: Flask development server suitable for <10 devices; use uWSGI/Gunicorn for production
- **Memory**: Web dashboard updates efficiently via REST API polling

## Security

⚠️ **For Development:**
- Sample certificates are included for quick setup
- TLS is enabled but certificate validation is disabled (CERT_NONE)

🔒 **For Production:**
1. Generate new certificates for your MQTT broker
2. Update certificate paths in `pc_logger.py`
3. Enable certificate validation (change `CERT_NONE` to appropriate option)
4. Use strong WiFi credentials
5. Run API server behind a reverse proxy (Nginx/Apache)
6. Implement API authentication if needed
7. Regularly rotate MQTT certificates
8. Use environment variables for sensitive configuration

## Performance Optimization Tips

1. **Reduce update frequency**: Adjust browser polling interval in dashboard
2. **Archival**: Move old logs to backup database periodically
3. **Indexes**: Database already has optimized indexes for common queries
4. **Compression**: Consider gzipping logs if storage is critical
5. **Cluster**: Use uWSGI to run multiple API server instances

## Future Enhancements

- [ ] WebSocket support for real-time log streaming
- [ ] Log export to CSV/JSON
- [ ] Persistent dashboard configuration
- [ ] Advanced analytics and trend detection
- [ ] Log rotation and archival automation
- [ ] API authentication and rate limiting
- [ ] Grafana integration
- [ ] Time-series data export

## License

This project is licensed under the **MIT License** - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Support & Issues

For bugs, feature requests, or questions:
- Open an issue on GitHub
- Check existing issues first
- Include device logs when reporting bugs
- Specify Python version and OS

## Contributors

This project was developed by:

- **Sarthak Bharad** - Project Lead
- **Gopal Awasthi** - Backend Development
- **Deepak Rajadurai** - Editor
- **Vishant Bimbra** - Backend Development
- **Amit Pal Singh** - Frontend & Web Dashboard

## Acknowledgments

### Course & Mentorship
- **Prof. Ralf Colmar Staudemeyer, PhD** - Course Professor
- **Tobias Tefke** - Course Tutor

### Technologies & Libraries
- **PineCone BL602**: Excellent low-cost IoT device with WiFi
- **Mosquitto**: Reliable MQTT broker
- **Paho Python Client**: MQTT library
- **Flask**: Web framework
- **SQLite**: Embedded database
- **FreeRTOS**: Real-time OS on BL602

---

**Project Status**: ✅ Production Ready  
**Last Updated**: January 2026  
**Tested with**: Python 3.8+, Mosquitto 2.0+, BL602 SDK 1.0+
