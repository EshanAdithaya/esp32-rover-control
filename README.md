# ESP32 Smart Delivery Robot

Dual ESP32 system for smart delivery robot with internet control, voice commands, camera streaming, and comprehensive sensor integration.

## 📋 Hardware Requirements

### Main ESP32 Board
- ESP32 Development Board (any variant)
- 2x DC Motors (for wheels)
- HC-SR04 Ultrasonic Sensor
- Temperature Sensor (analog)
- Battery Voltage Monitoring Circuit
- 2x Status LEDs
- Motor Driver (L298N recommended)

### ESP32-CAM Module
- ESP32-CAM Board (AI-Thinker model)
- MicroSD Card (optional)
- External antenna (for better WiFi range)

## 🔌 Pin Connections

### Main ESP32 (`delivery_bot.ino`)

#### Motor Control
```
Left Motor:
- Pin 13 (D13) → Motor Driver IN1
- Pin 12 (D12) → Motor Driver IN2

Right Motor:
- Pin 14 (D14) → Motor Driver IN3
- Pin 27 (D27) → Motor Driver IN4
```

#### Sensors
```
Ultrasonic Sensor (HC-SR04):
- Pin 5  (D5)  → Trigger Pin
- Pin 18 (D18) → Echo Pin

Temperature Sensor:
- Pin 34 (A0) → Analog Temperature Sensor

Battery Monitoring:
- Pin 35 (A1) → Battery Voltage Divider
```

#### Status LEDs
```
- Pin 2 (Built-in LED) → Status LED
- Pin 4 (D4) → Delivery Status LED
```

### ESP32-CAM (`camera_module.ino`)
Uses standard ESP32-CAM pin configuration:
```
Camera Pins (AI-Thinker ESP32-CAM):
- PWDN:  Pin 32
- RESET: -1 (not connected)
- XCLK:  Pin 0
- SIOD:  Pin 26 (SDA)
- SIOC:  Pin 27 (SCL)
- Y9:    Pin 35
- Y8:    Pin 34
- Y7:    Pin 39
- Y6:    Pin 36
- Y5:    Pin 21
- Y4:    Pin 19
- Y3:    Pin 18
- Y2:    Pin 5
- VSYNC: Pin 25
- HREF:  Pin 23
- PCLK:  Pin 22

Flash LED: Pin 4
```

## 🔧 Configuration

### WiFi Setup
Update in both files:
```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```

### Backend Communication
In `delivery_bot.ino`:
```cpp
const char* backendHost = "192.168.1.101"; // Your backend server IP
const int backendPort = 3001;
```

In `camera_module.ino`:
```cpp
const char* mainEsp32Host = "192.168.1.100"; // Main ESP32 IP
const int mainEsp32Port = 80;
```

## 🚀 Features

### Main ESP32 Board Features
- **Internet Control**: Full web-based robot control
- **Voice Commands**: Process and execute voice commands
- **Sensor Monitoring**: Real-time sensor data collection
- **Safety Systems**: Emergency stop, obstacle detection
- **Delivery Management**: Track delivery status and operations
- **Battery Monitoring**: Automatic low battery protection
- **Status LEDs**: Visual feedback for robot state
- **Backend Integration**: Real-time communication with backend API

### ESP32-CAM Features  
- **Live Video Streaming**: Real-time camera feed
- **Image Capture**: On-demand photo capture
- **Flash Control**: LED flash for better image quality
- **Mobile Optimized**: Responsive web interface
- **Communication**: Heartbeat with main ESP32
- **Settings Control**: Adjustable camera parameters

## 🌐 Web Interfaces

### Main ESP32 Interface
Access at: `http://[ESP32_IP]`

Features:
- Robot status dashboard
- Manual control buttons
- Live sensor readings
- Delivery management
- Voice command testing
- Emergency controls

### Camera Interface
Access at: `http://[ESP32_CAM_IP]:81`

Features:
- Live video stream
- Image capture
- Flash control
- Camera settings
- Status monitoring

## 📡 API Endpoints

### Main ESP32 Endpoints

#### Basic Movement
- `GET /forward` - Move forward
- `GET /backward` - Move backward  
- `GET /left` - Turn left
- `GET /right` - Turn right
- `GET /stop` - Stop all motors

#### Advanced Control
- `GET /status` - Get robot status (JSON)
- `POST /emergency-stop` - Emergency stop control
- `POST /set-mode` - Change robot mode
- `POST /start-delivery` - Start delivery
- `POST /complete-delivery` - Complete delivery
- `GET /sensors` - Get sensor data
- `POST /voice-command` - Process voice commands

#### Camera Integration
- `GET /camera/stream` - Camera stream endpoint
- `GET /camera/capture` - Capture image

### ESP32-CAM Endpoints
- `GET /` - Camera web interface
- `GET /stream` - Live video stream
- `GET /capture` - Capture image
- `GET /status` - Camera status
- `POST /settings` - Update camera settings
- `POST /flash` - Toggle flash LED

## 🎤 Voice Commands

Supported voice commands:
```
Movement:
- "move forward" / "forward"
- "move backward" / "backward" / "back"
- "turn left" / "left"
- "turn right" / "right"
- "stop" / "halt" / "pause"

Modes:
- "autonomous mode" / "auto mode"
- "manual mode"
- "emergency stop" / "emergency"

Delivery:
- "start delivery"
- "complete delivery"

Status:
- "robot status"
- "battery level"
```

## 🛡️ Safety Features

### Automatic Safety
- **Obstacle Detection**: Stops when objects < 10cm detected
- **Low Battery Protection**: Warning at 20%, emergency stop at 10%
- **Temperature Monitoring**: Alerts when > 50°C
- **Connection Monitoring**: Automatic reconnection attempts
- **Emergency Stop**: Multiple trigger methods available

### Manual Safety
- **Emergency Stop Button**: Web interface and API
- **Mode Switching**: Manual override of autonomous operations
- **Status LEDs**: Visual feedback for robot state
- **Real-time Monitoring**: Continuous sensor data streaming

## 📊 Sensor Data

### Available Sensors
- **Ultrasonic Distance**: 2-400cm range, 2cm accuracy
- **Temperature**: Environmental temperature monitoring
- **Battery Voltage**: Real-time battery level tracking  
- **Motion Status**: Current movement state detection
- **System Health**: Overall robot status monitoring

### Data Format
```json
{
  "temperature": 24.5,
  "ultrasonic": 15.2,
  "battery": 85.3,
  "isMoving": false,
  "status": "IDLE",
  "timestamp": 1234567890
}
```

## 🔄 System Communication

### Communication Flow
```
Backend API ←→ Main ESP32 ←→ ESP32-CAM
                    ↓
              Motor Controllers
                    ↓
               Sensors & LEDs
```

### Data Exchange
- **Status Updates**: Every 10 seconds to backend
- **Sensor Readings**: Every 2 seconds internal update
- **Camera Heartbeat**: Every 30 seconds to main ESP32
- **Command Execution**: Immediate response
- **Emergency Signals**: Real-time propagation

## 🔧 Customization

### Motor Control
Adjust motor behavior in `delivery_bot.ino`:
```cpp
void moveForward() {
  if (!emergencyStop) {
    leftMotorForward();
    rightMotorForward();
    // Add speed control here
  }
}
```

### Sensor Thresholds
Modify safety parameters:
```cpp
// Obstacle detection distance
if (sensors.ultrasonic < 10.0 && sensors.isMoving) {
  stopAllMotors();
}

// Battery warning levels
if (sensors.battery < 20.0) {
  // Low battery warning
}
```

### Camera Settings
Adjust camera quality in `camera_module.ino`:
```cpp
config.frame_size = FRAMESIZE_UXGA; // Image size
config.jpeg_quality = 10;          // Quality (lower = better)
```

## 📱 Mobile Support

Both web interfaces are fully responsive and optimized for:
- Touch controls for robot movement
- Mobile camera streaming
- Voice command activation
- Emergency stop access
- Status monitoring

## 🐛 Troubleshooting

### Common Issues

1. **Robot Not Moving**
   - Check motor connections
   - Verify motor driver power
   - Check emergency stop status

2. **WiFi Connection Failed**
   - Verify credentials in code
   - Check WiFi signal strength
   - Restart ESP32 boards

3. **Camera Not Streaming**
   - Check ESP32-CAM power supply (5V recommended)
   - Verify camera module connection
   - Check pin connections

4. **Sensors Not Reading**
   - Verify pin connections
   - Check sensor power supply
   - Test individual sensors

5. **Voice Commands Not Working**
   - Ensure ESP32 is connected to backend
   - Check voice command syntax
   - Verify backend API is running

### Debug Information

Enable debug output:
```cpp
Serial.begin(115200); // Already enabled in code
```

Monitor serial output for:
- WiFi connection status
- Command execution logs
- Sensor readings
- Error messages

## 🔄 Firmware Updates

### OTA Updates (Future Enhancement)
The code can be extended to support Over-The-Air updates:
1. Add OTA library includes
2. Implement update server
3. Add version checking
4. Enable remote firmware updates

### Manual Updates
1. Connect ESP32 via USB
2. Upload new firmware via Arduino IDE
3. Monitor serial output for successful upload
4. Test all functions after update

## 📄 License

MIT License - See LICENSE file for details

---

🤖 **Smart Delivery Robot** - Advanced ESP32-based autonomous delivery system with internet control and voice commands!