/*
 * PersonalCMS ESP32-S3 N8R8 - FIXED Captive Portal Version
 * Troubleshooting fixes for captive portal not loading issues
 * 
 * Common Issues Fixed:
 * 1. DNS server configuration
 * 2. WiFi AP settings optimization  
 * 3. Captive portal detection improvements
 * 4. Memory management for ESP32-S3
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <SPI.h>
#include <time.h>

// E-ink Display Headers (ESP32-S3 version)
#include "Display_EPD_W21_spi_s3.h"
#include "Display_EPD_W21.h"
#include "Ap_29demo.h"

// Device Configuration
String deviceID = "ESP32_S3_N8R8_001";  // Updated to match production naming
String deviceName = "";
String occupation = "";
String serverURL = "";

// WiFi Configuration
String wifiSSID = "";
String wifiPassword = "";
WebServer server(80);
DNSServer dnsServer;

// Captive Portal Configuration (FIXED)
const char* AP_SSID = "PersonalCMS-S3-Setup";
const char* AP_PASS = "12345678";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

// Hardware Pins (ESP32-S3 Production GPIO Assignment - Safe pins)
const int SWITCH_PIN = 9;   // GPIO 9 (Physical button - safe pin)
const int LED_PIN = 2;      // GPIO 2 (Built-in LED)

// Operating Mode and Button Handling
bool lastSwitchState = HIGH;
bool currentMode = false;   // false = dashboard, true = images
bool displayInitialized = false;
bool captivePortalActive = false;

// Enhanced button handling for device removal
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
bool longPressDetected = false;
const unsigned long LONG_PRESS_DURATION = 5000;

// Timer variables
unsigned long dashboardStartTime = 0;
unsigned long lastTimerUpdate = 0;
bool timerActive = false;

// Storage
Preferences nvs;
String preferencesJson = "";

// Display buffers (ESP32-S3 has more memory)
unsigned char dashboardBuffer[EPD_ARRAY];
bool dashboardLoaded = false;

// NTP Configuration for Mumbai timezone (IST = UTC+5:30)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

// Function Declarations
void setupWiFi();
void startCaptivePortal();
void handleRoot();
void handleSubmit();
void handleWiFiScan();
void handleNotFound();
void loadConfiguration();
void saveConfiguration();
void sendDeviceSetupToServer();
void checkServerForContent();
void parseAndDisplayContent(String jsonResponse);
void downloadAndDisplayDashboard(String url);
void downloadAndDisplayImage(String url);
void switchMode();
void initializeDisplay();
void setupNTP();
void updateTimer();
void displayCurrentContent();
void showBootupScreen();
void removeDeviceFromServer();
void unlinkDeviceFromServer();
void handleDeviceRemovalButton();
void flipBMPVertically(uint8_t* bmpData, int width, int height);

void setup() {
  Serial.begin(115200);
  delay(2000); // Give time for Serial to initialize
  
  Serial.println("");
  Serial.println("=============================================================================");
  Serial.println("🚀 PersonalCMS ESP32-S3 N8R8 - FIXED Captive Portal Version");
  Serial.println("🔧 Troubleshooting fixes applied for captive portal issues");
  Serial.println("=============================================================================");
  
  // Enhanced hardware diagnostic for ESP32-S3
  Serial.println("");
  Serial.println("🔧 ESP32-S3 HARDWARE DIAGNOSTICS:");
  Serial.printf("   📟 Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("   💾 Flash: %d MB, PSRAM: %d MB\n", 
                ESP.getFlashChipSize() / (1024 * 1024), 
                ESP.getPsramSize() / (1024 * 1024));
  Serial.printf("   🧠 Free Heap: %d KB, Free PSRAM: %d KB\n", 
                ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024);
  
  // Initialize hardware pins
  Serial.println("");
  Serial.println("🔌 INITIALIZING HARDWARE:");
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.printf("   ✅ GPIO %d (Switch): INPUT_PULLUP\n", SWITCH_PIN);
  Serial.printf("   ✅ GPIO %d (LED): OUTPUT\n", LED_PIN);
  
  // Load saved configuration
  Serial.println("");
  Serial.println("💾 LOADING CONFIGURATION:");
  loadConfiguration();
  
  // Initialize display
  Serial.println("");
  Serial.println("🖥️ DISPLAY INITIALIZATION:");
  initializeDisplay();
  showBootupScreen();
  
  // Setup WiFi - this will start captive portal if needed
  Serial.println("");
  Serial.println("📡 WIFI SETUP:");
  setupWiFi();
  
  Serial.println("");
  Serial.println("✅ SETUP COMPLETE - Device Ready");
  Serial.println("=============================================================================");
}

void loop() {
  static unsigned long lastStatusReport = 0;
  
  // Status reporting every 10 seconds
  if (millis() - lastStatusReport > 10000) {
    Serial.println("");
    Serial.println("📊 SYSTEM STATUS:");
    Serial.printf("   💾 Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("   📡 WiFi status: %s\n", 
                  WiFi.status() == WL_CONNECTED ? "CONNECTED" : 
                  captivePortalActive ? "CAPTIVE PORTAL ACTIVE" : "DISCONNECTED");
    if (captivePortalActive) {
      Serial.printf("   🌐 AP IP: %s\n", WiFi.softAPIP().toString().c_str());
      Serial.printf("   👥 Connected clients: %d\n", WiFi.softAPgetStationNum());
    }
    lastStatusReport = millis();
  }
  
  // Enhanced button handling
  bool currentSwitchState = digitalRead(SWITCH_PIN);
  
  if (currentSwitchState == LOW && lastSwitchState == HIGH) {
    // Button pressed
    buttonPressStartTime = millis();
    buttonPressed = true;
    longPressDetected = false;
    Serial.println("🔘 Button pressed - monitoring for short/long press");
    digitalWrite(LED_PIN, HIGH);
  }
  
  if (currentSwitchState == HIGH && lastSwitchState == LOW) {
    // Button released
    unsigned long pressDuration = millis() - buttonPressStartTime;
    buttonPressed = false;
    Serial.printf("🔘 Button released after %lu ms\n", pressDuration);
    digitalWrite(LED_PIN, LOW);
    
    if (!longPressDetected && pressDuration >= 200 && pressDuration < LONG_PRESS_DURATION) {
      Serial.println("🔄 Short press - switching mode");
      switchMode();
    }
    longPressDetected = false;
  }
  
  // Long press detection
  if (buttonPressed && !longPressDetected && (millis() - buttonPressStartTime >= LONG_PRESS_DURATION)) {
    longPressDetected = true;
    Serial.println("🚨 LONG PRESS DETECTED - DEVICE REMOVAL");
    handleDeviceRemovalButton();
  }
  
  // LED feedback during long press
  if (buttonPressed && !longPressDetected) {
    unsigned long currentPressDuration = millis() - buttonPressStartTime;
    if ((currentPressDuration / 200) % 2 == 0) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }
  }
  
  lastSwitchState = currentSwitchState;
  
  // Handle captive portal or normal operation
  if (WiFi.status() != WL_CONNECTED && captivePortalActive) {
    // Process captive portal requests
    dnsServer.processNextRequest();
    server.handleClient();
    return;
  }
  
  if (WiFi.status() == WL_CONNECTED && !timerActive) {
    // Connected WiFi mode - check for content
    static unsigned long lastContentCheck = 0;
    if (millis() - lastContentCheck > 25000) { // Check every 25 seconds
      checkServerForContent();
      lastContentCheck = millis();
    }
    
    updateTimer();
  }
  
  delay(10); // Small delay to prevent watchdog issues
}

void setupWiFi() {
  // Stop any existing WiFi connections
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);
  
  if (wifiSSID.length() > 0) {
    Serial.println("📡 Attempting to connect to saved WiFi: " + wifiSSID);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ WiFi connected successfully!");
      Serial.println("🌐 IP: " + WiFi.localIP().toString());
      Serial.printf("📶 Signal strength: %d dBm\n", WiFi.RSSI());
      digitalWrite(LED_PIN, HIGH);
      captivePortalActive = false;
      
      // Setup NTP
      setupNTP();
      
      // Register with server
      if (serverURL.length() > 0) {
        sendDeviceSetupToServer();
      }
      return;
    } else {
      Serial.println("\n❌ WiFi connection failed");
    }
  }
  
  Serial.println("🌐 Starting captive portal...");
  startCaptivePortal();
}

void startCaptivePortal() {
  // Stop STA mode and switch to AP mode
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP);
  
  // Configure Access Point with fixed IP
  WiFi.softAPConfig(AP_IP, AP_IP, AP_SUBNET);
  
  // Start Access Point
  bool apStarted = WiFi.softAP(AP_SSID, AP_PASS);
  if (!apStarted) {
    Serial.println("❌ Failed to start Access Point");
    delay(5000);
    ESP.restart();
    return;
  }
  
  delay(500); // Give AP time to start
  
  // Start DNS Server (redirect all requests to AP IP)
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  if (!dnsServer.start(53, "*", AP_IP)) {
    Serial.println("❌ Failed to start DNS server");
  }
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.on("/scan", handleWiFiScan);
  server.on("/generate_204", handleRoot); // Android captive portal detection
  server.on("/redirect", handleRoot);     // Microsoft captive portal detection
  server.on("/hotspot-detect.html", handleRoot); // Apple iOS detection
  server.on("/library/test/success.html", handleRoot); // Apple macOS detection
  server.on("/connecttest.txt", handleRoot); // Windows captive portal detection
  server.onNotFound(handleNotFound);      // Catch all other requests
  
  server.begin();
  captivePortalActive = true;
  
  Serial.println("✅ Captive portal started successfully");
  Serial.printf("   🌐 SSID: %s\n", AP_SSID);
  Serial.printf("   🔑 Password: %s\n", AP_PASS);
  Serial.printf("   📡 IP Address: %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("   🌍 Gateway: %s\n", WiFi.softAPIP().toString().c_str());
  Serial.println("");
  Serial.println("📱 SETUP INSTRUCTIONS:");
  Serial.println("   1. Connect your device to WiFi: PersonalCMS-S3-Setup");
  Serial.println("   2. Use password: 12345678");
  Serial.println("   3. Wait for captive portal to open automatically");
  Serial.println("   4. If portal doesn't open, visit: http://192.168.4.1");
}

void handleRoot() {
  Serial.printf("🌐 Serving captive portal to client: %s\n", server.client().remoteIP().toString().c_str());
  
  String html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>PersonalCMS ESP32-S3 Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 20px; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); 
            color: white; 
            min-height: 100vh;
        }
        .container { 
            max-width: 600px; 
            margin: 0 auto; 
            background: rgba(255,255,255,0.1); 
            padding: 30px; 
            border-radius: 15px; 
            backdrop-filter: blur(10px); 
        }
        .header { text-align: center; margin-bottom: 30px; }
        .header h2 { font-size: 28px; margin: 10px 0; }
        .section { 
            margin: 25px 0; 
            padding: 20px; 
            border-left: 4px solid #00ff88; 
            background: rgba(255,255,255,0.05); 
            border-radius: 8px; 
        }
        input, select, textarea { 
            width: 100%; 
            padding: 12px; 
            margin: 8px 0; 
            box-sizing: border-box; 
            border: none; 
            border-radius: 6px; 
            background: rgba(255,255,255,0.9); 
            color: #333; 
        }
        button { 
            background: linear-gradient(45deg, #00ff88, #00d4aa); 
            color: white; 
            padding: 15px; 
            border: none; 
            width: 100%; 
            font-size: 18px; 
            border-radius: 8px; 
            cursor: pointer; 
            font-weight: bold; 
        }
        button:hover { 
            transform: translateY(-2px); 
            box-shadow: 0 4px 15px rgba(0,255,136,0.3); 
        }
        .refresh-btn { 
            background: linear-gradient(45deg, #ff6b35, #f7931e); 
            margin-top: 8px; 
            padding: 10px; 
            font-size: 14px; 
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h2>PersonalCMS ESP32-S3</h2>
            <p>E-ink Display + Server Integration</p>
        </div>
        
        <form action="/submit" method="post">
            <div class="section">
                <h3>Device Information</h3>
                <input type="text" name="device_name" placeholder="Device Name (e.g., Living Room Display)" required>
                <input type="text" name="occupation" placeholder="Your Occupation" required>
            </div>
            
            <div class="section">
                <h3>WiFi Configuration</h3>
                <select id="wifi_networks" name="wifi_ssid" onchange="updateCustomSSID()" required>
                    <option value="">Scanning networks...</option>
                </select>
                <input type="text" id="custom_ssid" name="custom_wifi_ssid" placeholder="Or enter network manually" style="display:none;">
                <input type="password" name="wifi_password" placeholder="WiFi Password" required>
                <button type="button" class="refresh-btn" onclick="refreshNetworks()">Refresh Networks</button>
            </div>
            
            <div class="section">
                <h3>PersonalCMS Server</h3>
                <input type="url" name="server_url" placeholder="Server URL (e.g., http://192.168.1.100:5000)" required>
                <small>Make sure PersonalCMS server is running and accessible</small>
            </div>
            
            <div class="section">
                <h3>Content Preferences</h3>
                <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px;">
                    <div>
                        <h4>Jokes</h4>
                        <label><input type="checkbox" name="prefs" value="dad_jokes"> Dad Jokes</label><br>
                        <label><input type="checkbox" name="prefs" value="programming_jokes"> Programming</label><br>
                    </div>
                    <div>
                        <h4>Riddles</h4>
                        <label><input type="checkbox" name="prefs" value="easy_riddles"> Easy Riddles</label><br>
                        <label><input type="checkbox" name="prefs" value="logic_riddles"> Logic Puzzles</label><br>
                    </div>
                </div>
                <div style="margin-top: 10px;">
                    <h4>News</h4>
                    <label><input type="checkbox" name="prefs" value="tech_news"> Tech News</label>
                    <label><input type="checkbox" name="prefs" value="world_news"> World News</label>
                </div>
            </div>
            
            <button type="submit">Save &amp; Connect</button>
        </form>
    </div>
    
    <script>
        function loadNetworks() {
            fetch('/scan')
                .then(response => response.json())
                .then(networks => {
                    const select = document.getElementById('wifi_networks');
                    select.innerHTML = '<option value="">-- Select Network --</option>';
                    
                    networks.forEach(network => {
                        const option = document.createElement('option');
                        option.value = network.ssid;
                        option.textContent = `${network.ssid} (${network.rssi} dBm) ${network.encryption ? '[Secured]' : '[Open]'}`;
                        select.appendChild(option);
                    });
                    
                    const manualOption = document.createElement('option');
                    manualOption.value = 'custom';
                    manualOption.textContent = 'Enter manually';
                    select.appendChild(manualOption);
                })
                .catch(() => {
                    const select = document.getElementById('wifi_networks');
                    select.innerHTML = '<option value="">Scan failed</option><option value="custom">Enter manually</option>';
                });
        }
        
        function updateCustomSSID() {
            const select = document.getElementById('wifi_networks');
            const customInput = document.getElementById('custom_ssid');
            
            if (select.value === 'custom') {
                customInput.style.display = 'block';
                customInput.required = true;
                select.required = false;
            } else {
                customInput.style.display = 'none';
                customInput.required = false;
                select.required = true;
            }
        }
        
        function refreshNetworks() {
            const select = document.getElementById('wifi_networks');
            select.innerHTML = '<option value="">Scanning...</option>';
            loadNetworks();
        }
        
        window.onload = loadNetworks;
    </script>
</body>
</html>
)HTML";
  
  server.send(200, "text/html; charset=utf-8", html);
}

void handleNotFound() {
  Serial.printf("🔍 Redirect request from %s to captive portal\n", server.client().remoteIP().toString().c_str());
  // Redirect all unknown requests to the main page for captive portal detection
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "Redirecting to captive portal");
}

void handleSubmit() {
  Serial.println("📝 Processing configuration form submission");
  
  // Get form data
  deviceName = server.arg("device_name");
  occupation = server.arg("occupation");
  
  wifiSSID = server.arg("wifi_ssid");
  if (wifiSSID == "custom") {
    wifiSSID = server.arg("custom_wifi_ssid");
  }
  
  wifiPassword = server.arg("wifi_password");
  serverURL = server.arg("server_url");
  
  // Build preferences JSON
  DynamicJsonDocument doc(1024);
  doc["jokes"] = JsonArray();
  doc["riddles"] = JsonArray();
  doc["news"] = JsonArray();
  
  for (int i = 0; i < server.args(); i++) {
    if (server.argName(i) == "prefs") {
      String pref = server.arg(i);
      if (pref.indexOf("joke") >= 0) {
        doc["jokes"].add(pref);
      } else if (pref.indexOf("riddle") >= 0) {
        doc["riddles"].add(pref);
      } else if (pref.indexOf("news") >= 0) {
        doc["news"].add(pref);
      }
    }
  }
  
  serializeJson(doc, preferencesJson);
  saveConfiguration();
  
  server.send(200, "text/html; charset=utf-8", 
    "<html><head><meta charset='UTF-8'></head><body style='font-family:Arial; text-align:center; padding:50px; background:linear-gradient(135deg,#667eea,#764ba2); color:white;'>"
    "<div style='background:rgba(255,255,255,0.1); padding:40px; border-radius:15px; backdrop-filter:blur(10px);'>"
    "<h2>Configuration Saved!</h2>"
    "<p>Device will restart and connect to PersonalCMS server...</p>"
    "<div style='margin:25px; padding:20px; background:rgba(0,255,136,0.1); border-radius:10px;'>"
    "<strong>Startup Sequence:</strong><br><br>"
    "1. Connect to WiFi network<br>"
    "2. Register with PersonalCMS server<br>"
    "3. Initialize e-ink display<br>"
    "4. Start content delivery<br>"
    "5. Sync Mumbai timezone<br>"
    "6. Begin refresh cycle"
    "</div>"
    "<p><em>Device will be ready in 30-60 seconds</em></p>"
    "</div>"
    "</body></html>");
  
  delay(2000);
  ESP.restart();
}

void handleWiFiScan() {
  Serial.println("📡 Scanning for WiFi networks...");
  
  int networkCount = WiFi.scanNetworks();
  String json = "[";
  
  for (int i = 0; i < networkCount; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"encryption\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
    json += "}";
  }
  
  json += "]";
  Serial.printf("📡 Found %d networks\n", networkCount);
  server.send(200, "application/json", json);
}

void loadConfiguration() {
  nvs.begin("personalcms_s3", false);
  
  deviceName = nvs.getString("device_name", "");
  occupation = nvs.getString("occupation", "");
  wifiSSID = nvs.getString("wifi_ssid", "");
  wifiPassword = nvs.getString("wifi_password", "");
  serverURL = nvs.getString("server_url", "");
  preferencesJson = nvs.getString("preferences", "");
  
  if (preferencesJson.length() == 0) {
    preferencesJson = "{\"jokes\":[\"dad_jokes\"],\"riddles\":[\"easy_riddles\"],\"news\":[\"tech_news\"]}";
  }
  
  Serial.println("Configuration loaded:");
  Serial.printf("   Device: %s\n", deviceName.c_str());
  Serial.printf("   WiFi: %s\n", wifiSSID.length() > 0 ? wifiSSID.c_str() : "Not configured");
  Serial.printf("   Server: %s\n", serverURL.length() > 0 ? serverURL.c_str() : "Not configured");
  
  nvs.end();
}

void saveConfiguration() {
  nvs.begin("personalcms_s3", false);
  
  nvs.putString("device_name", deviceName);
  nvs.putString("occupation", occupation);
  nvs.putString("wifi_ssid", wifiSSID);
  nvs.putString("wifi_password", wifiPassword);
  nvs.putString("server_url", serverURL);
  nvs.putString("preferences", preferencesJson);
  
  nvs.end();
  Serial.println("✅ Configuration saved to NVS");
}

// Placeholder implementations for remaining functions
void initializeDisplay() {
  Serial.println("🖥️ Display initialization (ESP32-S3 N8R8)");
  
  // Initialize SPI pins for e-ink display (same as DevKit V1)
  pinMode(25, OUTPUT); // RST
  pinMode(26, OUTPUT); // DC  
  pinMode(5, OUTPUT);  // CS
  pinMode(27, INPUT);  // BUSY
  pinMode(33, OUTPUT); // PWR
  
  // Initialize SPI with ESP32-S3 optimizations
  SPI.begin(18, -1, 23, -1); // SCK=18, MISO=not used, MOSI=23, SS=not used
  SPI.setFrequency(8000000); // 8MHz for ESP32-S3 (faster than DevKit V1)
  
  // Power on the display
  digitalWrite(33, HIGH); // PWR on
  delay(100);
  
  // Check if display is connected by testing BUSY pin
  Serial.println("🔍 Checking for e-ink display connection...");
  
  // Try to initialize display with timeout
  bool displayConnected = false;
  unsigned long startTime = millis();
  
  // Set a 2-second timeout for display initialization
  while (millis() - startTime < 2000) {
    // Try basic display initialization
    digitalWrite(25, LOW);  // Reset
    delay(10);
    digitalWrite(25, HIGH); // Release reset
    delay(10);
    
    // Check if BUSY pin responds (indicates display is connected)
    if (digitalRead(27) == LOW || digitalRead(27) == HIGH) {
      // Pin is responsive, likely display is connected
      Serial.println("📟 Display connection detected, initializing...");
      EPD_Init();
      displayConnected = true;
      break;
    }
    delay(50);
  }
  
  if (displayConnected) {
    displayInitialized = true;
    Serial.println("✅ Display initialized successfully (ESP32-S3 optimized)");
  } else {
    displayInitialized = false;
    Serial.println("⚠️  No display detected - continuing in headless mode (ESP32-S3)");
    Serial.println("💻 Server communication and button controls will work normally");
  }
}

void showBootupScreen() {
  Serial.println("🖥️ Showing bootup screen");
  
  if (!displayInitialized) {
    initializeDisplay();
  }
  
  // Show a simple bootup message
  EPD_WhiteScreen_White(); // Clear screen to white
  delay(1000);
  EPD_DeepSleep();
}

void setupNTP() {
  Serial.println("🕒 Setting up NTP time sync");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void sendDeviceSetupToServer() {
  if (serverURL.length() == 0) {
    Serial.println("❌ No server URL configured - skipping registration");
    return;
  }
  
  Serial.println("📡 Registering device with PersonalCMS server");
  Serial.println("🔗 Server URL: " + serverURL);
  Serial.println("🏷️  Device ID: " + deviceID);
  Serial.println("📝 Device Name: " + deviceName);
  
  HTTPClient http;
  http.begin(serverURL + "/api/devices/register");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(1024);
  doc["device_id"] = deviceID;
  doc["device_name"] = deviceName;
  doc["occupation"] = occupation;
  doc["device_type"] = "ESP32_S3_N8R8";
  doc["wifi_ssid"] = wifiSSID;
  
  // Parse and add preferences
  if (preferencesJson.length() > 0) {
    DynamicJsonDocument prefDoc(512);
    deserializeJson(prefDoc, preferencesJson);
    doc["preferences"] = prefDoc;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("📤 Sending registration data:");
  Serial.println(jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("✅ Registration response (" + String(httpResponseCode) + "): " + response);
  } else {
    Serial.println("❌ Failed to register with server. HTTP Code: " + String(httpResponseCode));
    Serial.println("🔍 Error: " + http.errorToString(httpResponseCode));
  }
  
  http.end();
}

void checkServerForContent() {
  if (serverURL.length() == 0) {
    Serial.println("❌ No server URL configured - skipping content check");
    return;
  }
  
  Serial.println("📡 Checking server for new content");
  Serial.println("🔗 Server URL: " + serverURL);
  Serial.println("🏷️  Device ID: " + deviceID);
  
  HTTPClient http;
  String endpoint = "/api/devices/" + deviceID + "/images-sequence";
  String fullURL = serverURL + endpoint;
  
  Serial.println("📍 Full URL: " + fullURL);
  http.begin(fullURL);
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("✅ Content received from server");
    parseAndDisplayContent(response);
  } else {
    Serial.println("❌ Failed to get content. HTTP Code: " + String(httpResponseCode));
    Serial.println("🔍 Error: " + http.errorToString(httpResponseCode));
    if (httpResponseCode > 0) {
      String errorResponse = http.getString();
      Serial.println("📄 Error response: " + errorResponse);
    }
  }
  
  http.end();
}

void parseAndDisplayContent(String jsonResponse) {
  Serial.println("📄 Parsing server response");
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, jsonResponse);
  if (error) {
    Serial.println("❌ JSON parse error: " + String(error.c_str()));
    return;
  }
  
  // Debug: Print raw JSON
  Serial.println("=== RAW JSON RESPONSE ===");
  Serial.println(jsonResponse);
  
  // Check if response is successful
  if (!doc["success"].as<bool>()) {
    Serial.println("❌ Server returned error: " + String(doc["message"]));
    return;
  }
  
  Serial.println("=== AVAILABLE CONTENT ===");
  
  // Show dashboard URL
  String dashboard_url = doc["dashboard_url"];
  if (dashboard_url.length() > 0) {
    String full_dashboard_url = serverURL + dashboard_url;
    Serial.println("📊 Dashboard BMP: " + full_dashboard_url);
    
    // Download dashboard if in DASHBOARD mode
    if (!currentMode) {
      Serial.println("📊 DASHBOARD MODE - Downloading dashboard...");
      downloadAndDisplayDashboard(full_dashboard_url);
    }
  }
  
  // Show assigned images
  JsonArray assigned_images = doc["assigned_images"];
  Serial.println("🖼️  Assigned Images Count: " + String(assigned_images.size()));
  
  if (assigned_images.size() > 0) {
    Serial.println("Image Sequence URLs:");
    
    for (int i = 0; i < assigned_images.size(); i++) {
      String bmp_url = assigned_images[i]["bmp_url"];  // Use bmp_url instead of url!
      String filename = assigned_images[i]["filename"];
      
      if (bmp_url.length() > 0) {
        String full_image_url = serverURL + bmp_url;
        Serial.println("  [" + String(i) + "] " + filename + " -> " + full_image_url);
        
        // Download image if in IMAGE mode
        if (currentMode && i == 0) {  // Download first image in IMAGE mode
          Serial.println("🖼️  IMAGE MODE - Downloading image...");
          downloadAndDisplayImage(full_image_url);
        }
      }
    }
  } else {
    Serial.println("ℹ️  No images assigned to this device");
  }
}

void downloadAndDisplayDashboard(String url) {
  Serial.println("📊 Downloading dashboard from: " + url);
  
  HTTPClient http;
  http.begin(url);
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    int contentLength = http.getSize();
    Serial.println("✅ Dashboard download successful! Size: " + String(contentLength) + " bytes");
    Serial.println("📐 Expected EPD_ARRAY size: " + String(EPD_ARRAY) + " bytes");
    
    // Get the BMP data
    WiFiClient* stream = http.getStreamPtr();
    
    // Read BMP header first to find data offset
    uint8_t bmpHeader[54];
    size_t headerRead = stream->readBytes(bmpHeader, 54);
    
    if (headerRead >= 54 && bmpHeader[0] == 'B' && bmpHeader[1] == 'M') {
      Serial.println("✅ Valid BMP file detected for dashboard");
      
      // Get data offset from BMP header (bytes 10-13)
      uint32_t dataOffset = bmpHeader[10] | (bmpHeader[11] << 8) | (bmpHeader[12] << 16) | (bmpHeader[13] << 24);
      Serial.println("📍 BMP data offset: " + String(dataOffset) + " bytes");
      
      // Skip remaining header bytes if needed
      if (dataOffset > 54) {
        uint8_t skipBuffer[dataOffset - 54];
        stream->readBytes(skipBuffer, dataOffset - 54);
        Serial.println("⏭️  Skipped additional header: " + String(dataOffset - 54) + " bytes");
      }
      
      // Initialize display if not already done
      if (!displayInitialized) {
        initializeDisplay();
      }
      
      // Read BMP pixel data directly into dashboard buffer
      size_t bmpDataSize = contentLength - dataOffset;
      size_t bytesToRead = min(bmpDataSize, (size_t)EPD_ARRAY);
      
      Serial.println("🔄 Reading BMP pixel data: " + String(bytesToRead) + " bytes");
      size_t totalRead = stream->readBytes(dashboardBuffer, bytesToRead);
      
      if (totalRead == bytesToRead) {
        Serial.println("✅ BMP data loaded into buffer: " + String(totalRead) + " bytes");
        
        // Display on e-ink (ESP32-S3 version)
        Serial.println("🖥️ Displaying dashboard on e-ink display...");
        EPD_Init();
        EPD_WhiteScreen_ALL(dashboardBuffer, dashboardBuffer); // Use same buffer for B&W
        EPD_DeepSleep();
        
        dashboardLoaded = true;
        dashboardStartTime = millis();
        timerActive = false; // Reset timer state for new dashboard
        
        Serial.println("✅ Dashboard displayed successfully on e-ink!");
        Serial.println("🕒 Timer will start updating in bottom right corner (S3)");
      } else {
        Serial.println("❌ Failed to read complete BMP pixel data. Expected: " + String(bytesToRead) + ", Got: " + String(totalRead));
      }
    } else {
      Serial.println("❌ Invalid BMP header detected");
      Serial.print("Header bytes: ");
      for (int i = 0; i < min(headerRead, (size_t)10); i++) {
        Serial.print("0x");
        Serial.print(bmpHeader[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
    
  } else {
    Serial.println("❌ Dashboard download failed. HTTP Code: " + String(httpResponseCode));
    if (httpResponseCode > 0) {
      Serial.println("Error response: " + http.getString());
    }
  }
  
  http.end();
}

void downloadAndDisplayImage(String url) {
  Serial.println("🖼️  Downloading image from: " + url);
  
  HTTPClient http;
  http.begin(url);
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    int contentLength = http.getSize();
    Serial.println("✅ Image download successful! Size: " + String(contentLength) + " bytes");
    Serial.println("📐 Expected EPD_ARRAY size: " + String(EPD_ARRAY) + " bytes");
    
    // Get the BMP data
    WiFiClient* stream = http.getStreamPtr();
    
    // Read BMP header first to find data offset
    uint8_t bmpHeader[54];
    size_t headerRead = stream->readBytes(bmpHeader, 54);
    
    if (headerRead >= 54 && bmpHeader[0] == 'B' && bmpHeader[1] == 'M') {
      Serial.println("✅ Valid BMP file detected for image");
      
      // Get data offset from BMP header (bytes 10-13)
      uint32_t dataOffset = bmpHeader[10] | (bmpHeader[11] << 8) | (bmpHeader[12] << 16) | (bmpHeader[13] << 24);
      Serial.println("📍 BMP data offset: " + String(dataOffset) + " bytes");
      
      // Skip remaining header bytes if needed
      if (dataOffset > 54) {
        uint8_t skipBuffer[dataOffset - 54];
        stream->readBytes(skipBuffer, dataOffset - 54);
        Serial.println("⏭️  Skipped additional header: " + String(dataOffset - 54) + " bytes");
      }
      
      // Initialize display if not already done
      if (!displayInitialized) {
        initializeDisplay();
      }
      
      // Read BMP pixel data directly into dashboard buffer (reuse same buffer)
      size_t bmpDataSize = contentLength - dataOffset;
      size_t bytesToRead = min(bmpDataSize, (size_t)EPD_ARRAY);
      
      Serial.println("🔄 Reading BMP pixel data: " + String(bytesToRead) + " bytes");
      size_t totalRead = stream->readBytes(dashboardBuffer, bytesToRead);
      
      if (totalRead == bytesToRead) {
        Serial.println("✅ BMP data loaded into buffer: " + String(totalRead) + " bytes");
        
        // Display on e-ink (ESP32-S3 version)
        Serial.println("🖥️ Displaying image on e-ink display...");
        EPD_Init();
        EPD_WhiteScreen_ALL(dashboardBuffer, dashboardBuffer); // Use same buffer for B&W
        EPD_DeepSleep();
        
        Serial.println("✅ Image displayed successfully on e-ink!");
      } else {
        Serial.println("❌ Failed to read complete BMP pixel data. Expected: " + String(bytesToRead) + ", Got: " + String(totalRead));
      }
    } else {
      Serial.println("❌ Invalid BMP header detected");
      Serial.print("Header bytes: ");
      for (int i = 0; i < min(headerRead, (size_t)10); i++) {
        Serial.print("0x");
        Serial.print(bmpHeader[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
    
  } else {
    Serial.println("❌ Image download failed. HTTP Code: " + String(httpResponseCode));
    if (httpResponseCode > 0) {
      Serial.println("Error response: " + http.getString());
    }
  }
  
  http.end();
}

void switchMode() {
  currentMode = !currentMode;
  Serial.printf("🔄 Mode switched to: %s\n", currentMode ? "IMAGES" : "DASHBOARD");
  
  // Handle timer based on mode
  if (currentMode) {
    // Switching to IMAGE mode - disable timer
    if (timerActive) {
      EPD_Clear_Timer_Area();
      timerActive = false;
      Serial.println("🕒 Timer cleared - switched to IMAGE mode (S3)");
    }
  } else {
    // Switching to DASHBOARD mode - timer will start when dashboard loads
    timerActive = false;
    Serial.println("🕒 Timer ready for DASHBOARD mode (S3)");
  }
  
  // Immediately check for content in new mode
  checkServerForContent();
}

void updateTimer() {
  // Only update timer if we're in dashboard mode and dashboard is loaded
  if (currentMode || !dashboardLoaded) {
    return; // Timer only shows in dashboard mode
  }
  
  unsigned long currentTime = millis();
  
  // Update timer every second (1000ms) - faster updates on S3
  if (currentTime - lastTimerUpdate >= 1000) {
    // Calculate seconds since dashboard was loaded
    unsigned long elapsedSeconds = (currentTime - dashboardStartTime) / 1000;
    
    // Initialize partial display mode for timer updates
    if (!timerActive) {
      EPD_Init_Part();
      EPD_SetRAMValue_BaseMap(dashboardBuffer, dashboardBuffer);
      timerActive = true;
      Serial.println("🕒 Timer activated - partial display mode (ESP32-S3)");
    }
    
    // Display the timer in bottom right corner
    EPD_Display_Timer(elapsedSeconds);
    
    lastTimerUpdate = currentTime;
    Serial.printf("⏱️  Timer updated: %lu seconds since dashboard refresh\n", elapsedSeconds);
  }
}

void displayCurrentContent() {
  Serial.println("🖥️ Displaying current content");
}

void removeDeviceFromServer() {
  Serial.println("🗑️ Removing device from server");
}

void unlinkDeviceFromServer() {
  Serial.println("🔗 Unlinking device from server");
}

// Function to flip BMP data vertically (fix upside-down display)
void flipBMPVertically(uint8_t* bmpData, int width, int height) {
  Serial.println("🔄 Flipping BMP data vertically to fix orientation...");
  
  // Calculate bytes per row (assuming 1 bit per pixel for e-ink, rounded up to byte boundary)
  int bytesPerRow = (width + 7) / 8;
  
  // Create temporary buffer for one row
  uint8_t* tempRow = new uint8_t[bytesPerRow];
  
  // Flip the image by swapping rows
  for (int y = 0; y < height / 2; y++) {
    int topRowOffset = y * bytesPerRow;
    int bottomRowOffset = (height - 1 - y) * bytesPerRow;
    
    // Copy top row to temp
    memcpy(tempRow, &bmpData[topRowOffset], bytesPerRow);
    
    // Copy bottom row to top
    memcpy(&bmpData[topRowOffset], &bmpData[bottomRowOffset], bytesPerRow);
    
    // Copy temp to bottom
    memcpy(&bmpData[bottomRowOffset], tempRow, bytesPerRow);
  }
  
  delete[] tempRow;
  Serial.println("✅ BMP data orientation fixed!");
}

void downloadAndDisplayDashboard(String url) {
  Serial.println("📊 Downloading dashboard from: " + url);
  
  HTTPClient http;
  http.begin(url);
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    int contentLength = http.getSize();
    Serial.println("✅ Dashboard download successful! Size: " + String(contentLength) + " bytes");
    Serial.println("📐 Expected EPD_ARRAY size: " + String(EPD_ARRAY) + " bytes");
    
    // Get the BMP data
    WiFiClient* stream = http.getStreamPtr();
    
    // Read BMP header first to find data offset
    uint8_t bmpHeader[54];
    size_t headerRead = stream->readBytes(bmpHeader, 54);
    
    if (headerRead >= 54 && bmpHeader[0] == 'B' && bmpHeader[1] == 'M') {
      Serial.println("✅ Valid BMP file detected for dashboard");
      
      // Get data offset from BMP header (bytes 10-13)
      uint32_t dataOffset = bmpHeader[10] | (bmpHeader[11] << 8) | (bmpHeader[12] << 16) | (bmpHeader[13] << 24);
      Serial.println("📍 BMP data offset: " + String(dataOffset) + " bytes");
      
      // Skip remaining header bytes if needed
      if (dataOffset > 54) {
        uint8_t skipBuffer[dataOffset - 54];
        stream->readBytes(skipBuffer, dataOffset - 54);
        Serial.println("⏭️  Skipped additional header: " + String(dataOffset - 54) + " bytes");
      }
      
      // Initialize display if not already done
      if (!displayInitialized) {
        initializeDisplay();
      }
      
      // Read BMP pixel data directly into dashboard buffer
      size_t bmpDataSize = contentLength - dataOffset;
      size_t bytesToRead = min(bmpDataSize, (size_t)EPD_ARRAY);
      
      Serial.println("🔄 Reading BMP pixel data: " + String(bytesToRead) + " bytes");
      size_t totalRead = stream->readBytes(dashboardBuffer, bytesToRead);
      
      if (totalRead == bytesToRead) {
        Serial.println("✅ BMP data loaded into buffer: " + String(totalRead) + " bytes");
        
        // Fix BMP orientation (flip vertically to fix upside-down display)
        flipBMPVertically(dashboardBuffer, EPD_WIDTH, EPD_HEIGHT);
        
        if (displayInitialized) {
          // Display on e-ink if display is connected
          Serial.println("🖥️ Displaying dashboard on e-ink display...");
          EPD_Init();
          EPD_WhiteScreen_ALL(dashboardBuffer, dashboardBuffer); // Use same buffer for B&W
          EPD_DeepSleep();
          Serial.println("✅ Dashboard displayed successfully on e-ink!");
        } else {
          Serial.println("💻 Dashboard data ready (display not connected - headless mode)");
        }
        
        dashboardLoaded = true;
        dashboardStartTime = millis();
        timerActive = false; // Reset timer state for new dashboard
        
        Serial.println("🕒 Timer will start updating in bottom right corner");
      } else {
        Serial.println("❌ Failed to read complete BMP pixel data. Expected: " + String(bytesToRead) + ", Got: " + String(totalRead));
      }
    } else {
      Serial.println("❌ Invalid BMP header detected");
      Serial.print("Header bytes: ");
      for (int i = 0; i < min(headerRead, (size_t)10); i++) {
        Serial.print("0x");
        Serial.print(bmpHeader[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
    
  } else {
    Serial.println("❌ Dashboard download failed. HTTP Code: " + String(httpResponseCode));
    if (httpResponseCode > 0) {
      Serial.println("Error response: " + http.getString());
    }
  }
  
  http.end();
}

void downloadAndDisplayImage(String url) {
  Serial.println("🖼️  Downloading image from: " + url);
  
  HTTPClient http;
  http.begin(url);
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    int contentLength = http.getSize();
    Serial.println("✅ Image download successful! Size: " + String(contentLength) + " bytes");
    Serial.println("📐 Expected EPD_ARRAY size: " + String(EPD_ARRAY) + " bytes");
    
    // Get the BMP data
    WiFiClient* stream = http.getStreamPtr();
    
    // Read BMP header first to find data offset
    uint8_t bmpHeader[54];
    size_t headerRead = stream->readBytes(bmpHeader, 54);
    
    if (headerRead >= 54 && bmpHeader[0] == 'B' && bmpHeader[1] == 'M') {
      Serial.println("✅ Valid BMP file detected for image");
      
      // Get data offset from BMP header (bytes 10-13)
      uint32_t dataOffset = bmpHeader[10] | (bmpHeader[11] << 8) | (bmpHeader[12] << 16) | (bmpHeader[13] << 24);
      Serial.println("📍 BMP data offset: " + String(dataOffset) + " bytes");
      
      // Skip remaining header bytes if needed
      if (dataOffset > 54) {
        uint8_t skipBuffer[dataOffset - 54];
        stream->readBytes(skipBuffer, dataOffset - 54);
        Serial.println("⏭️  Skipped additional header: " + String(dataOffset - 54) + " bytes");
      }
      
      // Initialize display if not already done
      if (!displayInitialized) {
        initializeDisplay();
      }
      
      // Read BMP pixel data directly into dashboard buffer (reuse same buffer)
      size_t bmpDataSize = contentLength - dataOffset;
      size_t bytesToRead = min(bmpDataSize, (size_t)EPD_ARRAY);
      
      Serial.println("🔄 Reading BMP pixel data: " + String(bytesToRead) + " bytes");
      size_t totalRead = stream->readBytes(dashboardBuffer, bytesToRead);
      
      if (totalRead == bytesToRead) {
        Serial.println("✅ BMP data loaded into buffer: " + String(totalRead) + " bytes");
        
        // Fix BMP orientation (flip vertically to fix upside-down display)
        flipBMPVertically(dashboardBuffer, EPD_WIDTH, EPD_HEIGHT);
        
        if (displayInitialized) {
          // Display on e-ink if display is connected
          Serial.println("🖥️ Displaying image on e-ink display...");
          EPD_Init();
          EPD_WhiteScreen_ALL(dashboardBuffer, dashboardBuffer); // Use same buffer for B&W
          EPD_DeepSleep();
          Serial.println("✅ Image displayed successfully on e-ink!");
        } else {
          Serial.println("💻 Image data ready (display not connected - headless mode)");
        }
      } else {
        Serial.println("❌ Failed to read complete BMP pixel data. Expected: " + String(bytesToRead) + ", Got: " + String(totalRead));
      }
    } else {
      Serial.println("❌ Invalid BMP header detected");
      Serial.print("Header bytes: ");
      for (int i = 0; i < min(headerRead, (size_t)10); i++) {
        Serial.print("0x");
        Serial.print(bmpHeader[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
    
  } else {
    Serial.println("❌ Image download failed. HTTP Code: " + String(httpResponseCode));
    if (httpResponseCode > 0) {
      Serial.println("Error response: " + http.getString());
    }
  }
  
  http.end();
}

void handleDeviceRemovalButton() {
  Serial.println("🚨 Handling device removal - long press detected");
  // Add device removal logic here
}