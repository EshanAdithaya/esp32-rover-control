// ESP32-CAM Module for Smart Delivery Bot
// Handles video streaming, image capture, and communication with main ESP32
// Use this code for the ESP32-CAM board (secondary board)

#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

// WiFi credentials - SAME AS MAIN ESP32
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Main ESP32 communication
const char* mainEsp32Host = "192.168.1.100"; // IP of main ESP32
const int mainEsp32Port = 80;

WebServer server(81); // Different port from main ESP32
HTTPClient http;

// Camera pin definitions for ESP32-CAM (AI-Thinker model)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Flash LED for illumination (optional)
#define FLASH_LED_PIN      4

// Camera settings
bool cameraInitialized = false;
bool streamingActive = false;
unsigned long lastHeartbeat = 0;
unsigned long imageCounter = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize flash LED
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);
  
  // Connect to WiFi
  connectToWiFi();
  
  // Initialize camera
  initializeCamera();
  
  // Setup web server
  setupWebServer();
  
  Serial.println("📸 ESP32-CAM Module Ready!");
  Serial.printf("Camera IP: %s:81\n", WiFi.localIP().toString().c_str());
  
  // Signal ready to main ESP32
  sendHeartbeatToMain();
}

void loop() {
  server.handleClient();
  
  // Send heartbeat to main ESP32 every 30 seconds
  if (millis() - lastHeartbeat > 30000) {
    sendHeartbeatToMain();
    lastHeartbeat = millis();
  }
  
  delay(100);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Camera module connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    // Blink flash LED while connecting
    digitalWrite(FLASH_LED_PIN, !digitalRead(FLASH_LED_PIN));
  }
  
  digitalWrite(FLASH_LED_PIN, LOW);
  Serial.println();
  Serial.println("✓ Camera WiFi connected!");
  Serial.printf("Camera IP: %s\n", WiFi.localIP().toString().c_str());
}

void initializeCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Frame size and quality settings
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // 1600x1200
    config.jpeg_quality = 10; // Lower number = higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA; // 800x600
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Camera init failed with error 0x%x\n", err);
    cameraInitialized = false;
    return;
  }
  
  // Camera settings optimization
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 0); // 0 to 6 (0-No Effect)
    s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled
    s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    s->set_ae_level(s, 0);       // -2 to 2
    s->set_aec_value(s, 300);    // 0 to 1200
    s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    s->set_agc_gain(s, 0);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    s->set_vflip(s, 0);          // 0 = disable , 1 = enable
    s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
  }
  
  cameraInitialized = true;
  Serial.println("✓ Camera initialized successfully");
}

void setupWebServer() {
  // Camera endpoints
  server.on("/", handleRoot);
  server.on("/stream", HTTP_GET, handleStream);
  server.on("/capture", HTTP_GET, handleCapture);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/settings", HTTP_POST, handleSettings);
  server.on("/flash", HTTP_POST, handleFlash);
  
  // Start server
  server.begin();
  Serial.println("✓ Camera web server started on port 81");
}

void handleRoot() {
  String html = generateCameraInterface();
  server.send(200, "text/html", html);
}

String generateCameraInterface() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>📸 Delivery Bot Camera</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;margin:0;padding:20px;background:linear-gradient(135deg,#2c3e50 0%,#3498db 100%);color:white;}";
  html += "h1{margin-bottom:30px;text-shadow:2px 2px 4px rgba(0,0,0,0.5);}";
  html += ".card{background:rgba(255,255,255,0.1);backdrop-filter:blur(10px);border-radius:15px;padding:20px;margin:10px;border:1px solid rgba(255,255,255,0.2);}";
  html += "button{background:linear-gradient(45deg,#3498db,#2980b9);border:none;color:white;padding:12px 24px;font-size:14px;margin:8px;cursor:pointer;border-radius:20px;box-shadow:0 4px 8px rgba(0,0,0,0.2);transition:all 0.3s;}";
  html += "button:hover{transform:translateY(-2px);box-shadow:0 6px 12px rgba(0,0,0,0.3);}";
  html += "img{max-width:100%;border-radius:10px;box-shadow:0 4px 8px rgba(0,0,0,0.3);}";
  html += ".status{text-align:left;font-size:14px;}";
  html += "</style></head><body>";
  
  html += "<h1>📸 Delivery Bot Camera Module</h1>";
  html += "<p><strong>Camera IP:</strong> " + WiFi.localIP().toString() + ":81</p>";
  
  // Status card
  html += "<div class='card'>";
  html += "<h3>📊 Camera Status</h3>";
  html += "<div class='status'>";
  html += "<p><strong>Camera:</strong> " + String(cameraInitialized ? "READY" : "ERROR") + "</p>";
  html += "<p><strong>Streaming:</strong> " + String(streamingActive ? "ACTIVE" : "INACTIVE") + "</p>";
  html += "<p><strong>Images Captured:</strong> " + String(imageCounter) + "</p>";
  html += "<p><strong>WiFi Signal:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
  html += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
  html += "</div></div>";
  
  // Live stream
  if (cameraInitialized) {
    html += "<div class='card'>";
    html += "<h3>📹 Live Stream</h3>";
    html += "<img id='stream' src='/stream' style='width:100%;max-width:640px;'>";
    html += "<br><button onclick=\"toggleStream()\">🔄 Refresh Stream</button>";
    html += "</div>";
  }
  
  // Controls
  html += "<div class='card'>";
  html += "<h3>🎮 Camera Controls</h3>";
  html += "<button onclick=\"captureImage()\">📷 Capture Image</button>";
  html += "<button onclick=\"toggleFlash()\">💡 Flash On/Off</button>";
  html += "<button onclick=\"checkStatus()\">📊 Refresh Status</button>";
  html += "</div>";
  
  // Recent capture display
  html += "<div class='card'>";
  html += "<h3>📸 Last Capture</h3>";
  html += "<img id='lastCapture' src='/capture' style='width:100%;max-width:400px;' onerror=\"this.style.display='none'\">";
  html += "<p><small>Click 'Capture Image' to take a new photo</small></p>";
  html += "</div>";
  
  // JavaScript
  html += "<script>";
  html += "function captureImage() { fetch('/capture').then(r => { document.getElementById('lastCapture').src = '/capture?' + Date.now(); alert('Image captured!'); }); }";
  html += "function toggleFlash() { fetch('/flash', {method:'POST'}).then(r => r.text()).then(alert); }";
  html += "function toggleStream() { document.getElementById('stream').src = '/stream?' + Date.now(); }";
  html += "function checkStatus() { location.reload(); }";
  html += "setInterval(() => document.getElementById('stream').src = '/stream?' + Date.now(), 5000);"; // Refresh stream every 5 seconds
  html += "</script>";
  
  html += "<p><small>📱 Optimized for mobile • Auto-refresh stream every 5s</small></p>";
  html += "</body></html>";
  
  return html;
}

void handleStream() {
  if (!cameraInitialized) {
    server.send(503, "text/plain", "Camera not initialized");
    return;
  }
  
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(503, "text/plain", "Camera capture failed");
    return;
  }
  
  streamingActive = true;
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  
  esp_camera_fb_return(fb);
  
  // Update streaming status after a delay
  delay(100);
  streamingActive = false;
}

void handleCapture() {
  if (!cameraInitialized) {
    server.send(503, "text/plain", "Camera not initialized");
    return;
  }
  
  // Turn on flash for better image quality
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(100); // Brief flash
  
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    digitalWrite(FLASH_LED_PIN, LOW);
    server.send(503, "text/plain", "Camera capture failed");
    return;
  }
  
  imageCounter++;
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  
  esp_camera_fb_return(fb);
  digitalWrite(FLASH_LED_PIN, LOW);
  
  Serial.printf("📸 Image captured #%lu (Size: %d bytes)\n", imageCounter, fb->len);
  
  // Notify main ESP32 of image capture
  notifyMainEsp32("image_captured", String(imageCounter));
}

void handleStatus() {
  DynamicJsonDocument doc(512);
  
  doc["cameraInitialized"] = cameraInitialized;
  doc["streamingActive"] = streamingActive;
  doc["imageCounter"] = imageCounter;
  doc["ip"] = WiFi.localIP().toString();
  doc["port"] = 81;
  doc["wifiSignal"] = WiFi.RSSI();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["flashLedStatus"] = digitalRead(FLASH_LED_PIN);
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSettings() {
  if (!cameraInitialized) {
    server.send(503, "text/plain", "Camera not initialized");
    return;
  }
  
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, server.arg("plain"));
    
    sensor_t * s = esp_camera_sensor_get();
    if (s) {
      if (doc.containsKey("brightness")) s->set_brightness(s, doc["brightness"]);
      if (doc.containsKey("contrast")) s->set_contrast(s, doc["contrast"]);
      if (doc.containsKey("saturation")) s->set_saturation(s, doc["saturation"]);
      if (doc.containsKey("quality")) s->set_quality(s, doc["quality"]);
      
      server.send(200, "text/plain", "Camera settings updated");
    } else {
      server.send(500, "text/plain", "Failed to get camera sensor");
    }
  } else {
    server.send(400, "text/plain", "No settings provided");
  }
}

void handleFlash() {
  bool currentState = digitalRead(FLASH_LED_PIN);
  digitalWrite(FLASH_LED_PIN, !currentState);
  
  String status = digitalRead(FLASH_LED_PIN) ? "ON" : "OFF";
  server.send(200, "text/plain", "Flash LED: " + status);
  
  Serial.println("💡 Flash LED: " + status);
}

void sendHeartbeatToMain() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  http.begin(String("http://") + mainEsp32Host + ":" + mainEsp32Port + "/camera-heartbeat");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(256);
  doc["cameraModule"] = "online";
  doc["ip"] = WiFi.localIP().toString();
  doc["port"] = 81;
  doc["cameraReady"] = cameraInitialized;
  doc["imageCount"] = imageCounter;
  doc["timestamp"] = millis();
  
  String payload;
  serializeJson(doc, payload);
  
  int httpResponseCode = http.POST(payload);
  if (httpResponseCode > 0) {
    Serial.printf("📡 Heartbeat sent to main ESP32: %d\n", httpResponseCode);
  } else {
    Serial.printf("📡 Heartbeat failed: %d\n", httpResponseCode);
  }
  
  http.end();
}

void notifyMainEsp32(String event, String data) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  http.begin(String("http://") + mainEsp32Host + ":" + mainEsp32Port + "/camera-event");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(256);
  doc["event"] = event;
  doc["data"] = data;
  doc["cameraIp"] = WiFi.localIP().toString();
  doc["timestamp"] = millis();
  
  String payload;
  serializeJson(doc, payload);
  
  int httpResponseCode = http.POST(payload);
  if (httpResponseCode > 0) {
    Serial.printf("📤 Event sent to main ESP32: %s\n", event.c_str());
  }
  
  http.end();
}