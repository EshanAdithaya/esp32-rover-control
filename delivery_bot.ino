// Smart Delivery Bot - Enhanced ESP32 Code
// Features: Internet control, sensor integration, delivery status, voice command support
// Compatible with dual board setup (main + camera)

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// WiFi credentials - CHANGE THESE TO YOUR WIFI
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Backend API configuration
const char* backendHost = "192.168.1.101"; // Your backend server IP
const int backendPort = 3001;

WebServer server(80);
HTTPClient http;

// Motor pins for 2-wheel robot
int leftMotorPin1 = 13;   // D13
int leftMotorPin2 = 12;   // D12
int rightMotorPin1 = 14;  // D14
int rightMotorPin2 = 27;  // D27

// Sensor pins
int ultrasonicTrigPin = 5;  // D5
int ultrasonicEchoPin = 18; // D18
int temperatureSensorPin = 34; // A0 (analog)
int batteryPin = 35; // A1 (analog) - for battery monitoring

// LED indicators
int statusLedPin = 2;     // Built-in LED
int deliveryLedPin = 4;   // D4 - Delivery status LED

// Robot state
bool emergencyStop = false;
bool isDelivering = false;
String currentDeliveryId = "";
String robotMode = "manual"; // manual, autonomous, emergency
float batteryLevel = 100.0;
unsigned long lastSensorUpdate = 0;
unsigned long lastStatusUpdate = 0;

// Sensor data
struct SensorData {
  float temperature;
  float ultrasonic;
  float battery;
  bool isMoving;
  String status;
};

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  initializePins();
  
  // Connect to WiFi
  connectToWiFi();
  
  // Setup web server routes
  setupWebServer();
  
  Serial.println("🤖 Smart Delivery Bot Ready!");
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
  
  // Initial status LED blink
  blinkStatusLed(3);
  stopAllMotors();
}

void loop() {
  server.handleClient();
  
  // Update sensors every 2 seconds
  if (millis() - lastSensorUpdate > 2000) {
    updateSensors();
    lastSensorUpdate = millis();
  }
  
  // Send status to backend every 10 seconds
  if (millis() - lastStatusUpdate > 10000) {
    sendStatusToBackend();
    lastStatusUpdate = millis();
  }
  
  // Safety checks
  performSafetyChecks();
  
  delay(100);
}

void initializePins() {
  // Motor pins
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);
  
  // Sensor pins
  pinMode(ultrasonicTrigPin, OUTPUT);
  pinMode(ultrasonicEchoPin, INPUT);
  pinMode(temperatureSensorPin, INPUT);
  pinMode(batteryPin, INPUT);
  
  // LED pins
  pinMode(statusLedPin, OUTPUT);
  pinMode(deliveryLedPin, OUTPUT);
  
  Serial.println("✓ Pins initialized");
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    digitalWrite(statusLedPin, !digitalRead(statusLedPin)); // Blink while connecting
  }
  
  digitalWrite(statusLedPin, HIGH); // Solid when connected
  Serial.println();
  Serial.println("✓ WiFi connected!");
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
}

void setupWebServer() {
  // Basic movement endpoints
  server.on("/", handleRoot);
  server.on("/forward", []() { moveForward(); sendResponse("Moving Forward"); });
  server.on("/backward", []() { moveBackward(); sendResponse("Moving Backward"); });
  server.on("/left", []() { turnLeft(); sendResponse("Turning Left"); });
  server.on("/right", []() { turnRight(); sendResponse("Turning Right"); });
  server.on("/stop", []() { stopAllMotors(); sendResponse("Stopped"); });
  
  // Enhanced delivery bot endpoints
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/emergency-stop", HTTP_POST, handleEmergencyStop);
  server.on("/set-mode", HTTP_POST, handleSetMode);
  server.on("/start-delivery", HTTP_POST, handleStartDelivery);
  server.on("/complete-delivery", HTTP_POST, handleCompleteDelivery);
  server.on("/sensors", HTTP_GET, handleSensors);
  server.on("/voice-command", HTTP_POST, handleVoiceCommand);
  
  // Camera endpoints (for future camera module)
  server.on("/camera/stream", HTTP_GET, handleCameraStream);
  server.on("/camera/capture", HTTP_GET, handleCameraCapture);
  
  server.begin();
  Serial.println("✓ Web server started");
}

void handleRoot() {
  String html = generateWebInterface();
  server.send(200, "text/html", html);
}

String generateWebInterface() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>🚀 Smart Delivery Bot</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;margin:0;padding:20px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;}";
  html += "h1{margin-bottom:30px;text-shadow:2px 2px 4px rgba(0,0,0,0.5);}";
  html += ".card{background:rgba(255,255,255,0.1);backdrop-filter:blur(10px);border-radius:15px;padding:20px;margin:10px;border:1px solid rgba(255,255,255,0.2);}";
  html += "button{background:linear-gradient(45deg,#4CAF50,#45a049);border:none;color:white;padding:15px 30px;font-size:16px;margin:8px;cursor:pointer;border-radius:25px;box-shadow:0 4px 8px rgba(0,0,0,0.2);transition:all 0.3s;}";
  html += "button:hover{transform:translateY(-2px);box-shadow:0 6px 12px rgba(0,0,0,0.3);}";
  html += "button:active{transform:translateY(0px);}";
  html += ".emergency{background:linear-gradient(45deg,#f44336,#d32f2f) !important;}";
  html += ".status{text-align:left;font-size:14px;}";
  html += ".sensor-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:10px;margin-top:20px;}";
  html += "</style></head><body>";
  
  html += "<h1>🤖 Smart Delivery Bot Control</h1>";
  html += "<p><strong>Robot IP:</strong> " + WiFi.localIP().toString() + "</p>";
  
  // Status card
  html += "<div class='card'>";
  html += "<h3>🔌 Robot Status</h3>";
  html += "<div class='status'>";
  html += "<p><strong>Mode:</strong> " + robotMode + "</p>";
  html += "<p><strong>Battery:</strong> " + String(batteryLevel, 1) + "%</p>";
  html += "<p><strong>Emergency Stop:</strong> " + String(emergencyStop ? "ACTIVE" : "INACTIVE") + "</p>";
  html += "<p><strong>Delivery:</strong> " + String(isDelivering ? "IN PROGRESS (" + currentDeliveryId + ")" : "IDLE") + "</p>";
  html += "<p><strong>Connection:</strong> " + String(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED") + "</p>";
  html += "</div></div>";
  
  // Control buttons
  html += "<div class='card'>";
  html += "<h3>🎮 Manual Controls</h3>";
  html += "<button onclick=\"sendCommand('/forward')\">⬆️ FORWARD</button><br>";
  html += "<button onclick=\"sendCommand('/left')\">⬅️ LEFT</button>";
  html += "<button onclick=\"sendCommand('/stop')\">⏹️ STOP</button>";
  html += "<button onclick=\"sendCommand('/right')\">➡️ RIGHT</button><br>";
  html += "<button onclick=\"sendCommand('/backward')\">⬇️ BACKWARD</button>";
  html += "</div>";
  
  // Emergency and mode controls
  html += "<div class='card'>";
  html += "<h3>🚨 Safety & Mode</h3>";
  html += "<button class='emergency' onclick=\"emergencyStop()\">🛑 EMERGENCY STOP</button><br>";
  html += "<button onclick=\"setMode('manual')\">👤 MANUAL</button>";
  html += "<button onclick=\"setMode('autonomous')\">🤖 AUTO</button>";
  html += "</div>";
  
  // Delivery controls
  html += "<div class='card'>";
  html += "<h3>📦 Delivery Controls</h3>";
  html += "<button onclick=\"startDelivery()\">🚀 START DELIVERY</button>";
  html += "<button onclick=\"completeDelivery()\">✅ COMPLETE DELIVERY</button>";
  html += "</div>";
  
  // Sensor data
  SensorData sensors = readAllSensors();
  html += "<div class='card'>";
  html += "<h3>📊 Live Sensors</h3>";
  html += "<div class='sensor-grid'>";
  html += "<div>🌡️ Temperature<br><strong>" + String(sensors.temperature, 1) + "°C</strong></div>";
  html += "<div>📏 Distance<br><strong>" + String(sensors.ultrasonic, 1) + "cm</strong></div>";
  html += "<div>🔋 Battery<br><strong>" + String(sensors.battery, 1) + "%</strong></div>";
  html += "<div>🎯 Status<br><strong>" + sensors.status + "</strong></div>";
  html += "</div></div>";
  
  // JavaScript
  html += "<script>";
  html += "function sendCommand(endpoint) { fetch(endpoint).then(r=>r.text()).then(console.log); }";
  html += "function emergencyStop() { fetch('/emergency-stop', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({stop:true})}).then(r=>r.text()).then(alert); }";
  html += "function setMode(mode) { fetch('/set-mode', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({mode:mode})}).then(r=>r.text()).then(alert); }";
  html += "function startDelivery() { fetch('/start-delivery', {method:'POST'}).then(r=>r.text()).then(alert); }";
  html += "function completeDelivery() { fetch('/complete-delivery', {method:'POST'}).then(r=>r.text()).then(alert); }";
  html += "setInterval(() => location.reload(), 10000);"; // Auto-refresh every 10 seconds
  html += "document.addEventListener('keydown', function(e) {";
  html += "  switch(e.key) {";
  html += "    case 'ArrowUp': sendCommand('/forward'); break;";
  html += "    case 'ArrowDown': sendCommand('/backward'); break;";
  html += "    case 'ArrowLeft': sendCommand('/left'); break;";
  html += "    case 'ArrowRight': sendCommand('/right'); break;";
  html += "    case ' ': sendCommand('/stop'); break;";
  html += "  }";
  html += "});";
  html += "</script>";
  
  html += "<p><small>💡 Use arrow keys or touch controls • Auto-refresh every 10s</small></p>";
  html += "</body></html>";
  
  return html;
}

void handleStatus() {
  DynamicJsonDocument doc(1024);
  SensorData sensors = readAllSensors();
  
  doc["isConnected"] = WiFi.status() == WL_CONNECTED;
  doc["battery"] = sensors.battery;
  doc["mode"] = robotMode;
  doc["emergencyStop"] = emergencyStop;
  doc["isDelivering"] = isDelivering;
  doc["currentDeliveryId"] = currentDeliveryId;
  doc["ip"] = WiFi.localIP().toString();
  
  // Sensor data
  JsonObject sensorObj = doc.createNestedObject("sensors");
  sensorObj["temperature"] = sensors.temperature;
  sensorObj["ultrasonic"] = sensors.ultrasonic;
  sensorObj["battery"] = sensors.battery;
  sensorObj["status"] = sensors.status;
  
  // Location (static for now, could add GPS later)
  JsonObject location = doc.createNestedObject("location");
  location["lat"] = 6.9271;
  location["lng"] = 79.8612;
  
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleEmergencyStop() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    
    bool stop = doc["stop"] | false;
    emergencyStop = stop;
    
    if (emergencyStop) {
      stopAllMotors();
      robotMode = "emergency";
      digitalWrite(deliveryLedPin, HIGH); // Turn on delivery LED as warning
      Serial.println("🚨 EMERGENCY STOP ACTIVATED");
    } else {
      robotMode = "manual";
      digitalWrite(deliveryLedPin, LOW);
      Serial.println("✓ Emergency stop deactivated");
    }
    
    sendResponse(emergencyStop ? "Emergency stop activated" : "Emergency stop deactivated");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleSetMode() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    
    String mode = doc["mode"] | "manual";
    robotMode = mode;
    
    if (mode == "emergency") {
      emergencyStop = true;
      stopAllMotors();
    } else if (mode == "manual") {
      emergencyStop = false;
    }
    
    Serial.printf("Mode changed to: %s\n", mode.c_str());
    sendResponse("Mode changed to " + mode);
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleStartDelivery() {
  if (!emergencyStop) {
    isDelivering = true;
    currentDeliveryId = "DEL-" + String(millis());
    digitalWrite(deliveryLedPin, HIGH); // Turn on delivery LED
    
    Serial.printf("📦 Started delivery: %s\n", currentDeliveryId.c_str());
    sendResponse("Delivery started: " + currentDeliveryId);
  } else {
    server.send(400, "text/plain", "Cannot start delivery: Emergency stop active");
  }
}

void handleCompleteDelivery() {
  if (isDelivering) {
    String completedId = currentDeliveryId;
    isDelivering = false;
    currentDeliveryId = "";
    digitalWrite(deliveryLedPin, LOW); // Turn off delivery LED
    
    Serial.printf("✅ Completed delivery: %s\n", completedId.c_str());
    sendResponse("Delivery completed: " + completedId);
  } else {
    server.send(400, "text/plain", "No active delivery");
  }
}

void handleSensors() {
  SensorData sensors = readAllSensors();
  
  DynamicJsonDocument doc(512);
  doc["temperature"] = sensors.temperature;
  doc["ultrasonic"] = sensors.ultrasonic;
  doc["battery"] = sensors.battery;
  doc["isMoving"] = sensors.isMoving;
  doc["status"] = sensors.status;
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleVoiceCommand() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    
    String command = doc["command"] | "";
    command.toLowerCase();
    
    if (processVoiceCommand(command)) {
      sendResponse("Voice command executed: " + command);
    } else {
      server.send(400, "text/plain", "Voice command not recognized: " + command);
    }
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleCameraStream() {
  // Placeholder for camera stream (for future camera module)
  server.send(501, "text/plain", "Camera stream not implemented yet");
}

void handleCameraCapture() {
  // Placeholder for camera capture (for future camera module)
  server.send(501, "text/plain", "Camera capture not implemented yet");
}

bool processVoiceCommand(String command) {
  command.trim();
  command.toLowerCase();
  
  if (emergencyStop && command != "stop" && command != "emergency") {
    return false; // Only allow stop commands during emergency
  }
  
  // Movement commands
  if (command.indexOf("forward") >= 0 || command.indexOf("move forward") >= 0) {
    moveForward();
    return true;
  } else if (command.indexOf("backward") >= 0 || command.indexOf("back") >= 0) {
    moveBackward();
    return true;
  } else if (command.indexOf("left") >= 0) {
    turnLeft();
    return true;
  } else if (command.indexOf("right") >= 0) {
    turnRight();
    return true;
  } else if (command.indexOf("stop") >= 0 || command.indexOf("halt") >= 0) {
    stopAllMotors();
    return true;
  }
  
  // Mode commands
  else if (command.indexOf("autonomous") >= 0 || command.indexOf("auto") >= 0) {
    robotMode = "autonomous";
    return true;
  } else if (command.indexOf("manual") >= 0) {
    robotMode = "manual";
    emergencyStop = false;
    return true;
  } else if (command.indexOf("emergency") >= 0) {
    emergencyStop = true;
    stopAllMotors();
    robotMode = "emergency";
    return true;
  }
  
  return false; // Command not recognized
}

SensorData readAllSensors() {
  SensorData data;
  
  // Read temperature (simulated - replace with actual sensor)
  int tempReading = analogRead(temperatureSensorPin);
  data.temperature = 20.0 + (tempReading / 4095.0) * 15.0; // 20-35°C range
  
  // Read ultrasonic distance
  data.ultrasonic = readUltrasonicDistance();
  
  // Read battery level
  int batteryReading = analogRead(batteryPin);
  data.battery = (batteryReading / 4095.0) * 100.0; // Convert to percentage
  batteryLevel = data.battery; // Update global battery level
  
  // Determine if moving (check if any motor is active)
  data.isMoving = digitalRead(leftMotorPin1) || digitalRead(leftMotorPin2) || 
                  digitalRead(rightMotorPin1) || digitalRead(rightMotorPin2);
  
  // Set status
  if (emergencyStop) {
    data.status = "EMERGENCY";
  } else if (isDelivering) {
    data.status = "DELIVERING";
  } else if (data.isMoving) {
    data.status = "MOVING";
  } else {
    data.status = "IDLE";
  }
  
  return data;
}

float readUltrasonicDistance() {
  digitalWrite(ultrasonicTrigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(ultrasonicTrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(ultrasonicTrigPin, LOW);
  
  long duration = pulseIn(ultrasonicEchoPin, HIGH, 30000); // 30ms timeout
  if (duration == 0) {
    return 999.0; // Return max distance if no echo
  }
  
  float distance = (duration * 0.034) / 2; // Convert to cm
  return constrain(distance, 0, 400); // HC-SR04 range: 2-400cm
}

void updateSensors() {
  SensorData sensors = readAllSensors();
  
  // Update battery level
  batteryLevel = sensors.battery;
  
  // Log sensor readings periodically
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 30000) { // Log every 30 seconds
    Serial.printf("📊 Sensors - Temp: %.1f°C, Distance: %.1fcm, Battery: %.1f%%\n", 
                  sensors.temperature, sensors.ultrasonic, sensors.battery);
    lastLog = millis();
  }
}

void sendStatusToBackend() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  http.begin(String("http://") + backendHost + ":" + backendPort + "/api/robot/status");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(1024);
  SensorData sensors = readAllSensors();
  
  doc["robotId"] = WiFi.macAddress();
  doc["ip"] = WiFi.localIP().toString();
  doc["battery"] = sensors.battery;
  doc["mode"] = robotMode;
  doc["emergencyStop"] = emergencyStop;
  doc["isDelivering"] = isDelivering;
  doc["currentDeliveryId"] = currentDeliveryId;
  doc["sensors"]["temperature"] = sensors.temperature;
  doc["sensors"]["ultrasonic"] = sensors.ultrasonic;
  doc["sensors"]["status"] = sensors.status;
  doc["timestamp"] = millis();
  
  String payload;
  serializeJson(doc, payload);
  
  int httpResponseCode = http.POST(payload);
  if (httpResponseCode > 0) {
    Serial.printf("✓ Status sent to backend: %d\n", httpResponseCode);
  } else {
    Serial.printf("✗ Failed to send status: %d\n", httpResponseCode);
  }
  
  http.end();
}

void performSafetyChecks() {
  SensorData sensors = readAllSensors();
  
  // Obstacle detection
  if (sensors.ultrasonic < 10.0 && sensors.isMoving) {
    stopAllMotors();
    Serial.println("🚨 Obstacle detected! Stopping motors.");
  }
  
  // Low battery warning
  if (sensors.battery < 20.0) {
    static unsigned long lastWarning = 0;
    if (millis() - lastWarning > 60000) { // Warn every minute
      Serial.printf("⚠️  Low battery warning: %.1f%%\n", sensors.battery);
      blinkStatusLed(5); // Quick blinks for low battery
      lastWarning = millis();
    }
  }
  
  // Critical battery - emergency stop
  if (sensors.battery < 10.0 && !emergencyStop) {
    emergencyStop = true;
    robotMode = "emergency";
    stopAllMotors();
    Serial.println("🚨 CRITICAL BATTERY - EMERGENCY STOP ACTIVATED");
  }
  
  // Temperature check
  if (sensors.temperature > 50.0) {
    Serial.printf("🌡️  High temperature warning: %.1f°C\n", sensors.temperature);
  }
}

void blinkStatusLed(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(statusLedPin, LOW);
    delay(200);
    digitalWrite(statusLedPin, HIGH);
    delay(200);
  }
}

void sendResponse(String message) {
  DynamicJsonDocument doc(256);
  doc["message"] = message;
  doc["status"] = "success";
  doc["timestamp"] = millis();
  doc["robotMode"] = robotMode;
  doc["emergencyStop"] = emergencyStop;
  doc["isDelivering"] = isDelivering;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
  
  Serial.println("📤 " + message);
}

// Motor Control Functions
void leftMotorForward() {
  if (!emergencyStop) {
    digitalWrite(leftMotorPin1, HIGH);
    digitalWrite(leftMotorPin2, LOW);
  }
}

void leftMotorBackward() {
  if (!emergencyStop) {
    digitalWrite(leftMotorPin1, LOW);
    digitalWrite(leftMotorPin2, HIGH);
  }
}

void leftMotorStop() {
  digitalWrite(leftMotorPin1, LOW);
  digitalWrite(leftMotorPin2, LOW);
}

void rightMotorForward() {
  if (!emergencyStop) {
    digitalWrite(rightMotorPin1, HIGH);
    digitalWrite(rightMotorPin2, LOW);
  }
}

void rightMotorBackward() {
  if (!emergencyStop) {
    digitalWrite(rightMotorPin1, LOW);
    digitalWrite(rightMotorPin2, HIGH);
  }
}

void rightMotorStop() {
  digitalWrite(rightMotorPin1, LOW);
  digitalWrite(rightMotorPin2, LOW);
}

void moveForward() {
  if (!emergencyStop) {
    leftMotorForward();
    rightMotorForward();
    Serial.println("🔄 Moving Forward");
  }
}

void moveBackward() {
  if (!emergencyStop) {
    leftMotorBackward();
    rightMotorBackward();
    Serial.println("🔄 Moving Backward");
  }
}

void turnLeft() {
  if (!emergencyStop) {
    leftMotorBackward();
    rightMotorForward();
    Serial.println("🔄 Turning Left");
  }
}

void turnRight() {
  if (!emergencyStop) {
    leftMotorForward();
    rightMotorBackward();
    Serial.println("🔄 Turning Right");
  }
}

void stopAllMotors() {
  leftMotorStop();
  rightMotorStop();
  Serial.println("⏹️  All Motors Stopped");
}