# from flask import Flask, jsonify, request
# from flask_cors import CORS
# import sqlite3
# import threading
# from datetime import datetime, timedelta

# app = Flask(__name__)
# CORS(app)  # Enable CORS for frontend access

# DB_FILE = "device_logs.db"
# db_lock = threading.Lock()

# # --- DATABASE HELPER FUNCTIONS ---

# def get_db_connection():
#     """Create a database connection"""
#     conn = sqlite3.connect(DB_FILE)
#     conn.row_factory = sqlite3.Row  
#     return conn

# def parse_component(message):
#     """Extract component name from message (e.g., [WIFI], [MQTT])"""
#     if message.startswith('['):
#         end = message.find(']')
#         if end != -1:
#             return message[1:end]
#     return "SYSTEM"

# # --- API ENDPOINTS ---

# @app.route('/api/logs', methods=['GET'])
# def get_logs():
#     """
#     Get logs with optional filtering
#     Query params:
#     - limit: number of logs to return (default: 100)
#     - device: filter by device_id
#     - level: filter by log level (DEBUG, INFO, WARN, ERROR)
#     - component: filter by component
#     """
#     try:
#         # Get query parameters
#         limit = request.args.get('limit', 100, type=int)
#         device = request.args.get('device', '')
#         level = request.args.get('level', '')
#         component = request.args.get('component', '')
        
#         with db_lock:
#             conn = get_db_connection()
#             cursor = conn.cursor()
            
#             # Build query
#             query = "SELECT * FROM logs WHERE 1=1"
#             params = []
            
#             if device:
#                 query += " AND device_id = ?"
#                 params.append(device)
            
#             if level:
#                 query += " AND level = ?"
#                 params.append(level.upper())
            
#             query += " ORDER BY id DESC LIMIT ?"
#             params.append(limit)
            
#             cursor.execute(query, params)
#             rows = cursor.fetchall()
#             conn.close()
        
#         # Format logs for frontend
#         logs = []
#         for row in rows:
#             log = {
#                 'id': row['id'],
#                 'timestamp': row['timestamp'],
#                 'device_id': row['device_id'],
#                 'level': row['level'],
#                 'component': parse_component(row['message']),
#                 'message': row['message']
#             }
#             logs.append(log)
        
#         return jsonify(logs)
    
#     except Exception as e:
#         return jsonify({'error': str(e)}), 500

# @app.route('/api/stats', methods=['GET'])
# def get_stats():
#     """Get statistics about logs and devices"""
#     try:
#         with db_lock:
#             conn = get_db_connection()
#             cursor = conn.cursor()
            
#             # Total logs
#             cursor.execute("SELECT COUNT(*) as count FROM logs")
#             total_logs = cursor.fetchone()['count']
            
#             # Total devices
#             cursor.execute("SELECT COUNT(DISTINCT device_id) as count FROM logs")
#             # cursor.execute("SELECT COUNT(DISTINCT device_id) as count FROM logs WHERE device_id = ?", ("bl602_001",))
#             total_devices = cursor.fetchone()['count']
            
#             # Online devices (logs in last 5 minutes)
#             five_min_ago = (datetime.now() - timedelta(minutes=5)).isoformat()
#             cursor.execute("""
#                 SELECT COUNT(DISTINCT device_id) as count 
#                 FROM logs 
#                 WHERE timestamp > ?
#             """, (five_min_ago,))
#             online_devices = cursor.fetchone()['count']
            
#             conn.close()
        
#         return jsonify({
#             'total_logs': total_logs,
#             'total_devices': total_devices,
#             'online_devices': online_devices
#         })
    
#     except Exception as e:
#         return jsonify({'error': str(e)}), 500

# @app.route('/api/devices', methods=['GET'])
# def get_devices():
#     """Get list of all devices with their status"""
#     try:
#         with db_lock:
#             conn = get_db_connection()
#             cursor = conn.cursor()
            
#             # Get all devices with their last log timestamp
#             cursor.execute("""
#                 SELECT 
#                     device_id,
#                     COUNT(*) as log_count,
#                     MAX(timestamp) as last_seen
#                 FROM logs
#                 GROUP BY device_id
#                 ORDER BY last_seen DESC
#             """)
#             rows = cursor.fetchall()
#             conn.close()
        
#         # Format device data
#         devices = []
#         five_min_ago = datetime.now() - timedelta(minutes=5)
        
#         for row in rows:
#             last_seen = datetime.fromisoformat(row['last_seen'].replace('Z', '+00:00'))
#             is_online = last_seen > five_min_ago
            
#             device = {
#                 'device_id': row['device_id'],
#                 'log_count': row['log_count'],
#                 'last_seen': row['last_seen'],
#                 'status': 'online' if is_online else 'offline'
#             }
#             devices.append(device)
        
#         return jsonify(devices)
    
#     except Exception as e:
#         return jsonify({'error': str(e)}), 500

# @app.route('/api/clear', methods=['POST'])
# def clear_logs():
#     """Clear all logs from database"""
#     try:
#         with db_lock:
#             conn = sqlite3.connect(DB_FILE)
#             cursor = conn.cursor()
#             cursor.execute("DELETE FROM logs")
#             conn.commit()
#             conn.close()
        
#         return jsonify({'message': 'All logs cleared successfully'})
    
#     except Exception as e:
#         return jsonify({'error': str(e)}), 500

# @app.route('/api/levels', methods=['GET'])
# def get_log_levels():
#     """Get distinct log levels"""
#     try:
#         with db_lock:
#             conn = get_db_connection()
#             cursor = conn.cursor()
#             cursor.execute("SELECT DISTINCT level FROM logs ORDER BY level")
#             rows = cursor.fetchall()
#             conn.close()
        
#         levels = [row['level'] for row in rows]
#         return jsonify(levels)
    
#     except Exception as e:
#         return jsonify({'error': str(e)}), 500

# @app.route('/api/components', methods=['GET'])
# def get_components():
#     """Get distinct components from logs"""
#     try:
#         with db_lock:
#             conn = get_db_connection()
#             cursor = conn.cursor()
#             cursor.execute("SELECT DISTINCT message FROM logs")
#             rows = cursor.fetchall()
#             conn.close()
        
#         # Extract unique components
#         components = set()
#         for row in rows:
#             component = parse_component(row['message'])
#             components.add(component)
        
#         return jsonify(sorted(list(components)))
    
#     except Exception as e:
#         return jsonify({'error': str(e)}), 500

# @app.route('/api/health', methods=['GET'])
# def health_check():
#     """Health check endpoint"""
#     return jsonify({
#         'status': 'healthy',
#         'timestamp': datetime.now().isoformat()
#     })

# @app.route('/')
# def index():
#     """Serve the HTML frontend"""
#     return app.send_static_file('index.html')

# # --- RUN SERVER ---

# if __name__ == '__main__':
#     print("🚀 Starting REST API server...")
#     print("📡 API available at: http://localhost:5000")
#     print("🌐 Frontend available at: http://localhost:5000")
#     print("\nAvailable endpoints:")
#     print("  GET  /api/logs       - Get logs with filtering")
#     print("  GET  /api/stats      - Get statistics")
#     print("  GET  /api/devices    - Get device list")
#     print("  GET  /api/levels     - Get log levels")
#     print("  GET  /api/components - Get components")
#     print("  POST /api/clear      - Clear all logs")
#     print("  GET  /api/health     - Health check")
    
#     # Run the Flask app
#     app.run(host='0.0.0.0', port=5000, debug=True, threaded=True)






from flask import Flask, jsonify, request
from flask_cors import CORS
import sqlite3
import threading
from datetime import datetime, timedelta

app = Flask(__name__)
CORS(app)  # Enable CORS for frontend access

DB_FILE = "device_logs.db"
db_lock = threading.Lock()

# --- DATABASE HELPER FUNCTIONS ---

def get_db_connection():
    """Create a database connection"""
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row  
    return conn

def parse_component(message):
    """Extract component name from message (e.g., [WIFI], [MQTT])"""
    if message.startswith('['):
        end = message.find(']')
        if end != -1:
            return message[1:end]
    return "SYSTEM"

def parse_timestamp(timestamp_str):
    """Safely parse timestamp string to datetime object (timezone-naive)"""
    try:
        # Remove timezone info to keep it naive (easier for comparison)
        timestamp_str = timestamp_str.split('+')[0].split('Z')[0].strip()
        
        # Try different formats
        for fmt in ['%Y-%m-%d %H:%M:%S.%f', '%Y-%m-%d %H:%M:%S', '%Y-%m-%dT%H:%M:%S.%f', '%Y-%m-%dT%H:%M:%S']:
            try:
                return datetime.strptime(timestamp_str, fmt)
            except:
                continue
        
        # If format parsing fails, try fromisoformat
        return datetime.fromisoformat(timestamp_str)
    except:
        pass
    
    # If all parsing fails, return current time (naive)
    return datetime.now()

# --- API ENDPOINTS ---

@app.route('/api/logs', methods=['GET'])
def get_logs():
    """
    Get logs with optional filtering
    Query params:
    - limit: number of logs to return (default: 100)
    - device: filter by device_id
    - level: filter by log level (DEBUG, INFO, WARN, ERROR)
    - component: filter by component
    """
    try:
        # Get query parameters
        limit = request.args.get('limit', 100, type=int)
        device = request.args.get('device', '')
        level = request.args.get('level', '')
        component = request.args.get('component', '')
        
        with db_lock:
            conn = get_db_connection()
            cursor = conn.cursor()
            
            # Build query
            query = "SELECT * FROM logs WHERE 1=1"
            params = []
            
            if device:
                query += " AND device_id = ?"
                params.append(device)
            
            if level:
                query += " AND level = ?"
                params.append(level.upper())
            
            query += " ORDER BY id DESC LIMIT ?"
            params.append(limit)
            
            cursor.execute(query, params)
            rows = cursor.fetchall()
            conn.close()
        
        # Format logs for frontend
        logs = []
        for row in rows:
            log = {
                'id': row['id'],
                'timestamp': row['timestamp'],
                'device_id': row['device_id'],
                'level': row['level'],
                'component': parse_component(row['message']),
                'message': row['message']
            }
            logs.append(log)
        
        return jsonify(logs)
    
    except Exception as e:
        print(f"Error in /api/logs: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/stats', methods=['GET'])
def get_stats():
    """Get statistics about logs and devices"""
    try:
        with db_lock:
            conn = get_db_connection()
            cursor = conn.cursor()
            
            # Total logs
            cursor.execute("SELECT COUNT(*) as count FROM logs")
            total_logs = cursor.fetchone()['count']
            
            # Total devices
            cursor.execute("SELECT COUNT(DISTINCT device_id) as count FROM logs")
            # cursor.execute("SELECT COUNT(DISTINCT device_id) as count FROM logs WHERE device_id = ?", ("bl602_001",))
            total_devices = cursor.fetchone()['count']
            
            # Online devices (logs in last 5 minutes)
            five_min_ago = (datetime.now() - timedelta(minutes=5)).isoformat()
            cursor.execute("""
                SELECT COUNT(DISTINCT device_id) as count 
                FROM logs 
                WHERE timestamp > ?
            """, (five_min_ago,))
            online_devices = cursor.fetchone()['count']
            
            conn.close()
        
        return jsonify({
            'total_logs': total_logs,
            'total_devices': total_devices,
            'online_devices': online_devices
        })
    
    except Exception as e:
        print(f"Error in /api/stats: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/devices', methods=['GET'])
def get_devices():
    """Get list of all devices with their status"""
    try:
        with db_lock:
            conn = get_db_connection()
            cursor = conn.cursor()
            
            # Get all devices with their last log timestamp
            cursor.execute("""
                SELECT 
                    device_id,
                    COUNT(*) as log_count,
                    MAX(timestamp) as last_seen
                FROM logs
                GROUP BY device_id
                ORDER BY last_seen DESC
            """)
            rows = cursor.fetchall()
            conn.close()
        
        # Format device data
        devices = []
        five_min_ago = datetime.now() - timedelta(minutes=5)
        
        for row in rows:
            try:
                # Safely parse the timestamp
                last_seen = parse_timestamp(row['last_seen'])
                is_online = last_seen > five_min_ago
                
                device = {
                    'device_id': row['device_id'],
                    'log_count': row['log_count'],
                    'last_seen': row['last_seen'],
                    'status': 'online' if is_online else 'offline'
                }
                devices.append(device)
            except Exception as e:
                print(f"Error parsing device {row['device_id']}: {e}")
                # Still add the device even if timestamp parsing fails
                device = {
                    'device_id': row['device_id'],
                    'log_count': row['log_count'],
                    'last_seen': row['last_seen'],
                    'status': 'unknown'
                }
                devices.append(device)
        
        return jsonify(devices)
    
    except Exception as e:
        print(f"Error in /api/devices: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500

@app.route('/api/clear', methods=['POST'])
def clear_logs():
    """Clear all logs from database"""
    try:
        with db_lock:
            conn = sqlite3.connect(DB_FILE)
            cursor = conn.cursor()
            cursor.execute("DELETE FROM logs")
            conn.commit()
            conn.close()
        
        return jsonify({'message': 'All logs cleared successfully'})
    
    except Exception as e:
        print(f"Error in /api/clear: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/levels', methods=['GET'])
def get_log_levels():
    """Get distinct log levels"""
    try:
        with db_lock:
            conn = get_db_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT DISTINCT level FROM logs ORDER BY level")
            rows = cursor.fetchall()
            conn.close()
        
        levels = [row['level'] for row in rows]
        return jsonify(levels)
    
    except Exception as e:
        print(f"Error in /api/levels: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/components', methods=['GET'])
def get_components():
    """Get distinct components from logs"""
    try:
        with db_lock:
            conn = get_db_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT DISTINCT message FROM logs")
            rows = cursor.fetchall()
            conn.close()
        
        # Extract unique components
        components = set()
        for row in rows:
            component = parse_component(row['message'])
            components.add(component)
        
        return jsonify(sorted(list(components)))
    
    except Exception as e:
        print(f"Error in /api/components: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'timestamp': datetime.now().isoformat()
    })

@app.route('/')
def index():
    """Serve the HTML frontend"""
    return app.send_static_file('index.html')

# --- RUN SERVER ---

if __name__ == '__main__':
    print("🚀 Starting REST API server...")
    print("📡 API available at: http://localhost:5000")
    print("🌐 Frontend available at: http://localhost:5000")
    print("\nAvailable endpoints:")
    print("  GET  /api/logs       - Get logs with filtering")
    print("  GET  /api/stats      - Get statistics")
    print("  GET  /api/devices    - Get device list")
    print("  GET  /api/levels     - Get log levels")
    print("  GET  /api/components - Get components")
    print("  POST /api/clear      - Clear all logs")
    print("  GET  /api/health     - Health check")
    
    # Run the Flask app
    app.run(host='0.0.0.0', port=5000, debug=True, threaded=True)