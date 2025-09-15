#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// WiFi credentials
const char* ssid = "SLT-Fiber-home";
const char* password = "ShiFa4198+*";

WebServer server(80);

// Motor pins
int motor1Pin1 = 27, motor1Pin2 = 26, enable1Pin = 14;
int motor2Pin1 = 25, motor2Pin2 = 33, enable2Pin = 32;
int motor3Pin1 = 19, motor3Pin2 = 18, enable3Pin = 5;
int motor4Pin1 = 12, motor4Pin2 = 13, enable4Pin = 4;
Servo doorServo;
int servomotor = 21;  // pwm pin for servo

int currentSpeed = 200;

// Ultrasonic pins
const int trigPin = 2;
const int echoPin = 15;
long duration;
int distance;
int obstacleThreshold = 20; // cm

// Detection system
bool detectionEnabled = false;
String botState = "Idle";

void setup() {
  Serial.begin(115200);

  // Motor pins setup
  pinMode(motor1Pin1, OUTPUT); pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT); pinMode(motor2Pin2, OUTPUT);
  pinMode(motor3Pin1, OUTPUT); pinMode(motor3Pin2, OUTPUT);
  pinMode(motor4Pin1, OUTPUT); pinMode(motor4Pin2, OUTPUT);
  doorServo.attach(servomotor);

  // Ultrasonic pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/forward", []() { moveForward(); server.send(200, "text/plain", "Moving Forward"); });
  server.on("/backward", []() { moveBackward(); server.send(200, "text/plain", "Moving Backward"); });
  server.on("/left", []() { turnLeft(); server.send(200, "text/plain", "Turning Left"); });
  server.on("/right", []() { turnRight(); server.send(200, "text/plain", "Turning Right"); });
  server.on("/stop", []() { stopAllMotors(); server.send(200, "text/plain", "Stopped"); });
  server.on("/open", []() { opendoor(); server.send(200, "text/plain", "door open"); });
  server.on("/close", []() { closedoor(); server.send(200, "text/plain", "door close"); });
  server.on("/speed", handleSpeed);

  // New detection toggle
  server.on("/detectionOn", []() {
    detectionEnabled = true;
    server.send(200, "text/plain", "Obstacle Detection ON");
  });
  server.on("/detectionOff", []() {
    detectionEnabled = false;
    server.send(200, "text/plain", "Obstacle Detection OFF");
  });

  // Status JSON endpoint
  server.on("/status", []() {
    String json = "{\"detection\":\"" + String(detectionEnabled ? "ON" : "OFF") +
                  "\",\"state\":\"" + botState + "\"}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("Web server started");
  stopAllMotors();
}

void loop() {
  server.handleClient();

  if (detectionEnabled) {
    distance = getDistance();
    if (distance > 0 && distance < obstacleThreshold) {
      stopAllMotors();
      botState = "Stopped: Obstacle Detected";
    } else {
      botState = "Moving";
    }
  } else {
    botState = "Manual Control";
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>Delivery Bot</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:Arial;text-align:center;margin:0;padding:20px;}";
  html += "button{background:#4CAF50;border:none;color:white;padding:15px 32px;text-align:center;";
  html += "font-size:16px;margin:4px 2px;cursor:pointer;border-radius:8px;}";
  html += "button:active{background:#45a049;}</style></head><body>";
  html += "<h1>🤖 Delivery Bot Control</h1>";
  html += "<p>Bot IP: " + WiFi.localIP().toString() + "</p>";
  html += "<button onclick=\"fetch('/forward')\">⬆ FORWARD</button><br>";
  html += "<button onclick=\"fetch('/left')\">⬅ LEFT</button>";
  html += "<button onclick=\"fetch('/stop')\">⏹ STOP</button>";
  html += "<button onclick=\"fetch('/right')\">➡ RIGHT</button><br>";
  html += "<button onclick=\"fetch('/backward')\">⬇ BACKWARD</button><br><br>";
  html += "<button onclick=\"fetch('/open')\">🔓 OPEN DOOR</button>";
  html += "<button onclick=\"fetch('/close')\">🔒 CLOSE DOOR</button><br><br>";

  // New detection buttons
  html += "<button onclick=\"fetch('/detectionOn')\">🟢 Detection ON</button>";
  html += "<button onclick=\"fetch('/detectionOff')\">🔴 Detection OFF</button><br><br>";

  // Status
  html += "<p><b>Detection:</b> <span id='detection'>" + String(detectionEnabled ? "ON" : "OFF") + "</span></p>";
  html += "<p><b>Status:</b> <span id='state'>" + botState + "</span></p>";

  // Speed control
  html += "<p>Speed: <input type='range' min='50' max='255' value='200' ";
  html += "onchange=\"fetch('/speed?value='+this.value)\"> <span id='speedValue'>200</span></p>";

  // Auto-update script
  html += "<script>setInterval(()=>{fetch('/status').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('detection').innerText=d.detection;";
  html += "document.getElementById('state').innerText=d.state;";
  html += "});},1000);</script>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSpeed() {
  if (server.hasArg("value")) {
    currentSpeed = server.arg("value").toInt();
    if (currentSpeed < 50) currentSpeed = 50;
    if (currentSpeed > 255) currentSpeed = 255;
    server.send(200, "text/plain", "Speed set to " + String(currentSpeed));
  }
}

// Ultrasonic distance
int getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH, 30000); // timeout 30ms
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

// Motor Control Functions
void forwardMotor(int pin1, int pin2, int enPin, int speed) {
  digitalWrite(pin1, LOW);
  digitalWrite(pin2, HIGH);
  analogWrite(enPin, speed);
}

void backwardMotor(int pin1, int pin2, int enPin, int speed) {
  digitalWrite(pin1, HIGH);
  digitalWrite(pin2, LOW);
  analogWrite(enPin, speed);
}

void stopMotor(int pin1, int pin2, int enPin) {
  digitalWrite(pin1, LOW);
  digitalWrite(pin2, LOW);
  analogWrite(enPin, 0);
}

void opendoor() {
  doorServo.write(90);
  Serial.println("Door Opened");
}

void closedoor() {
  doorServo.write(0);
  Serial.println("Door Closed");
}

void moveForward() {
  forwardMotor(motor1Pin1, motor1Pin2, enable1Pin, currentSpeed);
  forwardMotor(motor2Pin1, motor2Pin2, enable2Pin, currentSpeed);
  forwardMotor(motor3Pin1, motor3Pin2, enable3Pin, currentSpeed);
  forwardMotor(motor4Pin1, motor4Pin2, enable4Pin, currentSpeed);
  Serial.println("Moving Forward");
}

void moveBackward() {
  backwardMotor(motor1Pin1, motor1Pin2, enable1Pin, currentSpeed);
  backwardMotor(motor2Pin1, motor2Pin2, enable2Pin, currentSpeed);
  backwardMotor(motor3Pin1, motor3Pin2, enable3Pin, currentSpeed);
  backwardMotor(motor4Pin1, motor4Pin2, enable4Pin, currentSpeed);
  Serial.println("Moving Backward");
}

void turnLeft() {
  forwardMotor(motor1Pin1, motor1Pin2, enable1Pin, currentSpeed/3);
  forwardMotor(motor2Pin1, motor2Pin2, enable2Pin, currentSpeed/3);
  forwardMotor(motor3Pin1, motor3Pin2, enable3Pin, currentSpeed);
  forwardMotor(motor4Pin1, motor4Pin2, enable4Pin, currentSpeed);
  Serial.println("Turning Left");
}

void turnRight() {
  forwardMotor(motor1Pin1, motor1Pin2, enable1Pin, currentSpeed);
  forwardMotor(motor2Pin1, motor2Pin2, enable2Pin, currentSpeed);
  forwardMotor(motor3Pin1, motor3Pin2, enable3Pin, currentSpeed/3);
  forwardMotor(motor4Pin1, motor4Pin2, enable4Pin, currentSpeed/3);
  Serial.println("Turning Right");
}

void stopAllMotors() {
  stopMotor(motor1Pin1, motor1Pin2, enable1Pin);
  stopMotor(motor2Pin1, motor2Pin2, enable2Pin);
  stopMotor(motor3Pin1, motor3Pin2, enable3Pin);
  stopMotor(motor4Pin1, motor4Pin2, enable4Pin);
  Serial.println("Stopped");
}
