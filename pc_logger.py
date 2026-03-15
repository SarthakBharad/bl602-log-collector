#!/usr/bin/env python3
"""
pc_logger.py - Centralized Log Collector Server for PineCone BL602 Devices

Receives plain-text logs via MQTT and performs server-side:
- Timestamp assignment
- Log level classification (INFO, WARN, ERROR)
- Subsystem extraction (WIFI, MQTT, SYSTEM, etc.)
- Device ID parsing (from topic or message)
- SQLite database storage

MULTI-DEVICE SUPPORT:
- Subscribes to: logs/# (wildcard)
- Each device publishes to: logs/<DEVICE_ID>
- Example: logs/BL602_6C21

Log Format from Device: "DEVICE_ID|TICK|[SUBSYSTEM] message"
Example: "BL602_6C21|12345|[WIFI] Connected to network"
"""

import paho.mqtt.client as mqtt
import ssl
import json
import sqlite3
import threading
from datetime import datetime
from zoneinfo import ZoneInfo

BROKER_IP = "localhost"
PORT = 8883
TOPIC = "logs/#"

DB_FILE = "device_logs.db"
TIMEZONE = "Europe/Berlin"

DEFAULT_DEVICE_ID = "unknown_device"

db_lock = threading.Lock()

def init_db():
    """Initialize SQLite database with enhanced schema"""
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            device_id TEXT NOT NULL,
            device_tick INTEGER DEFAULT 0,
            level TEXT NOT NULL,
            subsystem TEXT NOT NULL DEFAULT 'SYSTEM',
            message TEXT NOT NULL
        )
    """)
    
    cursor.execute("PRAGMA table_info(logs)")
    existing_columns = [row[1] for row in cursor.fetchall()]
    
    if 'subsystem' not in existing_columns:
        print("📦 Migrating database: adding 'subsystem' column...")
        cursor.execute("ALTER TABLE logs ADD COLUMN subsystem TEXT NOT NULL DEFAULT 'SYSTEM'")
    
    if 'device_tick' not in existing_columns:
        print("📦 Migrating database: adding 'device_tick' column...")
        cursor.execute("ALTER TABLE logs ADD COLUMN device_tick INTEGER DEFAULT 0")
    
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_device ON logs(device_id)")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_level ON logs(level)")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_subsystem ON logs(subsystem)")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_timestamp ON logs(timestamp)")
    
    conn.commit()
    conn.close()
    print(f"✅ Database initialized: {DB_FILE}")

def save_to_db(timestamp, device_id, device_tick, level, subsystem, message):
    """Save log entry to database"""
    with db_lock:
        conn = sqlite3.connect(DB_FILE)
        cursor = conn.cursor()
        cursor.execute("""
            INSERT INTO logs (timestamp, device_id, device_tick, level, subsystem, message)
            VALUES (?, ?, ?, ?, ?, ?)
        """, (timestamp, device_id, device_tick, level, subsystem, message))
        conn.commit()
        conn.close()

def detect_log_level(message):
    """
    Classify log level based on message content.
    Priority: ERROR > WARN > INFO
    """
    msg_lower = message.lower()
    
    error_keywords = ("error", "failed", "panic", "fatal", "fault", 
                      "exception", "crash", "out of memory", "err_")
    if any(k in msg_lower for k in error_keywords):
        return "ERROR"
    
    warn_keywords = ("warn", "warning", "retry", "timeout", "unstable", 
                     "reconnect", "disconnect", "lost", "slow")
    if any(k in msg_lower for k in warn_keywords):
        return "WARN"
    
    debug_keywords = ("debug", "trace", "verbose")
    if any(k in msg_lower for k in debug_keywords):
        return "DEBUG"
    
    return "INFO"

def extract_subsystem(message):
    """
    Extract subsystem from bracketed prefix in message.
    Example: "[WIFI] Connected" -> "WIFI"
    """
    if message.startswith("["):
        end_bracket = message.find("]")
        if end_bracket > 1:
            return message[1:end_bracket].upper()
    
    msg_lower = message.lower()
    if "wifi" in msg_lower or "wlan" in msg_lower:
        return "WIFI"
    if "mqtt" in msg_lower:
        return "MQTT"
    if "memory" in msg_lower or "heap" in msg_lower:
        return "MEMORY"
    if "tcp" in msg_lower or "ip" in msg_lower or "lwip" in msg_lower:
        return "NETWORK"
    if "flush" in msg_lower or "buffer" in msg_lower:
        return "LOG"
    if "boot" in msg_lower or "start" in msg_lower or "init" in msg_lower:
        return "SYSTEM"
    
    return "SYSTEM"

def extract_device_id_from_topic(topic):
    """
    Extract device ID from MQTT topic.
    
    Example: "logs/BL602_6C21" -> "BL602_6C21"
    Example: "logs/system" -> None (use message device ID)
    """
    parts = topic.split("/")
    if len(parts) >= 2:
        device_id = parts[1]
        if device_id.lower() != "system":
            return device_id
    return None

def parse_log_line(line):
    """
    Parse a log line from the device.
    
    Expected format: "DEVICE_ID|TICK|message"
    Example: "BL602_6C21|12345|[WIFI] Connected to network"
    
    Returns: (device_id, tick, message) or None if invalid
    """
    parts = line.split("|", 2)
    
    if len(parts) == 3:
        device_id = parts[0].strip()
        try:
            tick = int(parts[1].strip())
        except ValueError:
            tick = 0
        message = parts[2].strip()
        return (device_id, tick, message)
    
    elif len(parts) == 2:
        device_id = parts[0].strip()
        message = parts[1].strip()
        return (device_id, 0, message)
    
    else:
        return (DEFAULT_DEVICE_ID, 0, line.strip())

def on_connect(client, userdata, flags, rc):
    """Handle MQTT connection"""
    if rc == 0:
        print(f"✅ Connected to MQTT broker")
        print(f"📡 Subscribed to: {TOPIC} (wildcard - receives from ALL devices)")
        client.subscribe(TOPIC)
    else:
        error_messages = {
            1: "Incorrect protocol version",
            2: "Invalid client identifier",
            3: "Server unavailable",
            4: "Bad username or password",
            5: "Not authorized"
        }
        print(f"❌ Connection failed: {error_messages.get(rc, f'Unknown error {rc}')}")

def on_message(client, userdata, msg):
    """Process incoming MQTT messages"""
    try:
        payload = msg.payload.decode(errors="ignore").strip()
        if not payload:
            return

        topic_device_id = extract_device_id_from_topic(msg.topic)

        lines = payload.split("\n")

        for line in lines:
            line = line.strip()
            if not line:
                continue

            timestamp = datetime.now(ZoneInfo(TIMEZONE)).isoformat(timespec="milliseconds")
            
            try:
                data = json.loads(line, strict=False)
                message = str(data.get("msg", "")).strip()
                device_id = data.get("device", DEFAULT_DEVICE_ID)
                tick = data.get("tick", 0)
                
                if not message:
                    continue
                    
            except json.JSONDecodeError:
                parsed = parse_log_line(line)
                if parsed is None:
                    print(f"⚠️ Could not parse: {line[:80]}")
                    continue
                    
                device_id, tick, message = parsed
                
                if not message:
                    continue

            if topic_device_id:
                device_id = topic_device_id

            if not message or message.lower() in ('ack', 'ok', 'ping'):
                continue

            level = detect_log_level(message)
            subsystem = extract_subsystem(message)
            
            save_to_db(timestamp, device_id, tick, level, subsystem, message)
            
            level_emoji = {"ERROR": "🔴", "WARN": "🟡", "INFO": "🟢", "DEBUG": "⚪"}.get(level, "⚪")
            print(f"{level_emoji} {timestamp} | {device_id:12} | {level:5} | {subsystem:8} | {message}")

    except Exception as e:
        print(f"❌ Message handling error: {e}")

def on_disconnect(client, userdata, rc):
    """Handle MQTT disconnection"""
    if rc == 0:
        print("📴 Disconnected cleanly")
    else:
        print(f"⚠️ Unexpected disconnect (rc={rc}), will attempt reconnect...")

def print_stats():
    """Print database statistics"""
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    
    cursor.execute("SELECT COUNT(*) FROM logs")
    total = cursor.fetchone()[0]
    
    cursor.execute("SELECT device_id, COUNT(*) FROM logs GROUP BY device_id")
    devices = cursor.fetchall()
    
    cursor.execute("SELECT level, COUNT(*) FROM logs GROUP BY level")
    levels = cursor.fetchall()
    
    cursor.execute("SELECT subsystem, COUNT(*) FROM logs GROUP BY subsystem ORDER BY COUNT(*) DESC LIMIT 5")
    subsystems = cursor.fetchall()
    
    conn.close()
    
    print("\n📊 Database Statistics:")
    print(f"   Total logs: {total}")
    print(f"   By device: {dict(devices)}")
    print(f"   By level: {dict(levels)}")
    print(f"   Top subsystems: {dict(subsystems)}")
    print()

if __name__ == "__main__":
    print("=" * 60)
    print("  PineCone BL602 Centralized Log Collector")
    print("  Multi-Device Support Enabled")
    print("=" * 60)
    
    init_db()

    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect

    client.tls_set(
        ca_certs="ca.crt",
        cert_reqs=ssl.CERT_NONE
    )

    try:
        print(f"🔌 Connecting to {BROKER_IP}:{PORT} ...")
        client.connect(BROKER_IP, PORT, 60)
        client.loop_forever()

    except KeyboardInterrupt:
        print("\n")
        print_stats()
        print("🛑 Logger stopped")

    except Exception as e:
        print(f"❌ Connection failed: {e}")