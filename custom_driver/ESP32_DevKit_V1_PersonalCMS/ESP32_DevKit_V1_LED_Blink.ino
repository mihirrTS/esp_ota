/*
 * ESP32 DevKit V1 - LED Blink OTA Version
 * Simple LED blinking pattern with OTA update capabilities
 * Includes WiFi captive portal and credential saving
 * 
 * Features:
 * - Configurable LED blinking patterns
 * - WiFi captive portal with credential saving
 * - OTA update check every 2 minutes
 * - Multiple LED modes (heartbeat, fast blink, slow blink, breathing)
 * - Persistent configuration storage
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <time.h>

// Device Configuration
String deviceID = "ESP32_OTA_LED_001";
String deviceName = "";
String serverURL = "";
String otaUsername = "";
String otaPassword = "";

// WiFi Configuration
String wifiSSID = "";
String wifiPassword = "";
WebServer server(80);
DNSServer dnsServer;

// Captive Portal Configuration
const char* AP_SSID = "ESP32-LED-OTA-Setup";
const char* AP_PASS = "12345678";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

// Hardware Pins
const int LED_PIN = 2;        // Built-in LED
const int EXTERNAL_LED = 4;   // External LED pin (optional)

// LED Configuration
enum LEDMode {
  LED_HEARTBEAT,    // Slow heartbeat pattern
  LED_FAST_BLINK,   // Fast blinking
  LED_SLOW_BLINK,   // Slow blinking
  LED_BREATHING,    // Breathing effect (PWM)
  LED_PATTERN       // Custom pattern
};

LEDMode currentLEDMode = LED_HEARTBEAT;
int ledBrightness = 255;      // 0-255 for PWM
int blinkInterval = 1000;     // milliseconds
bool useExternalLED = false;  // Use external LED on GPIO 4

// OTA Configuration
const unsigned long OTA_CHECK_INTERVAL = 120000; // 2 minutes
unsigned long lastOTACheck = 0;
String currentFirmwareVersion = "2.0.0";
bool otaInProgress = false;

// LED timing variables
unsigned long lastLEDUpdate = 0;
bool ledState = false;
int breathingDirection = 1;
int breathingValue = 0;

// Captive portal state
bool captivePortalActive = false;

// Storage
Preferences nvs;

// Function Declarations
void setupWiFi();
void startCaptivePortal();
void handleRoot();
void handleSubmit();
void handleWiFiScan();
void handleNotFound();
void loadConfiguration();
void saveConfiguration();
void checkForOTAUpdate();
void performOTAUpdate(String firmwareURL);
void sendHeartbeat();
void updateLED();
void setLEDMode(LEDMode mode);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("");
  Serial.println("=============================================================================");
  Serial.println("🚀 ESP32 DevKit V1 - LED Blink OTA Version");
  Serial.println("💡 LED patterns with OTA capabilities");
  Serial.println("🔄 Version: " + currentFirmwareVersion);
  Serial.println("=============================================================================");
  
  // Hardware initialization
  pinMode(LED_PIN, OUTPUT);
  pinMode(EXTERNAL_LED, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(EXTERNAL_LED, LOW);
  
  // Load configuration
  loadConfiguration();
  
  // Show startup pattern (rapid blinks)
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    if (useExternalLED) digitalWrite(EXTERNAL_LED, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    if (useExternalLED) digitalWrite(EXTERNAL_LED, LOW);
    delay(100);
  }
  
  // Setup WiFi
  setupWiFi();
  
  Serial.println("✅ Setup complete - LED OTA system ready");
  Serial.println("💡 Current LED mode: " + String(currentLEDMode));
  Serial.println("🔄 OTA checks will run every 2 minutes");
  Serial.println("=============================================================================");
}

void loop() {
  static unsigned long lastStatusReport = 0;
  
  // Handle captive portal if active
  if (captivePortalActive) {
    dnsServer.processNextRequest();
    server.handleClient();
    // Still update LED during setup
    updateLED();
    return;
  }
  
  // Main operation when WiFi connected
  if (WiFi.status() == WL_CONNECTED) {
    // Check for OTA updates every 2 minutes
    if (millis() - lastOTACheck >= OTA_CHECK_INTERVAL) {
      checkForOTAUpdate();
      lastOTACheck = millis();
    }
    
    // Send heartbeat every 30 seconds
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat >= 30000) {
      sendHeartbeat();
      lastHeartbeat = millis();
    }
  }
  
  // Update LED pattern
  updateLED();
  
  // Status reporting every 30 seconds
  if (millis() - lastStatusReport >= 30000) {
    Serial.println("📊 LED OTA Status:");
    Serial.printf("   ⏰ Uptime: %lu seconds\n", millis() / 1000);
    Serial.printf("   📡 WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");
    Serial.printf("   🔄 Version: %s\n", currentFirmwareVersion.c_str());
    Serial.printf("   💡 LED Mode: %d, Brightness: %d\n", currentLEDMode, ledBrightness);
    Serial.printf("   💾 Free heap: %d bytes\n", ESP.getFreeHeap());
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("   🔄 Next OTA check in: %lu seconds\n", (OTA_CHECK_INTERVAL - (millis() - lastOTACheck)) / 1000);
    }
    lastStatusReport = millis();
  }
  
  delay(10); // Small delay for stability
}

void updateLED() {
  unsigned long currentTime = millis();
  
  switch (currentLEDMode) {
    case LED_HEARTBEAT:
      // Heartbeat pattern: quick double blink every 2 seconds
      if (currentTime - lastLEDUpdate >= 2000) {
        digitalWrite(LED_PIN, HIGH);
        if (useExternalLED) analogWrite(EXTERNAL_LED, ledBrightness);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        if (useExternalLED) digitalWrite(EXTERNAL_LED, LOW);
        delay(100);
        digitalWrite(LED_PIN, HIGH);
        if (useExternalLED) analogWrite(EXTERNAL_LED, ledBrightness);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        if (useExternalLED) digitalWrite(EXTERNAL_LED, LOW);
        lastLEDUpdate = currentTime;
      }
      break;
      
    case LED_FAST_BLINK:
      // Fast blinking every 200ms
      if (currentTime - lastLEDUpdate >= 200) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        if (useExternalLED) {
          analogWrite(EXTERNAL_LED, ledState ? ledBrightness : 0);
        }
        lastLEDUpdate = currentTime;
      }
      break;
      
    case LED_SLOW_BLINK:
      // Slow blinking every 1 second
      if (currentTime - lastLEDUpdate >= blinkInterval) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        if (useExternalLED) {
          analogWrite(EXTERNAL_LED, ledState ? ledBrightness : 0);
        }
        lastLEDUpdate = currentTime;
      }
      break;
      
    case LED_BREATHING:
      // Breathing effect with PWM
      if (currentTime - lastLEDUpdate >= 20) {
        breathingValue += breathingDirection * 5;
        if (breathingValue >= 255) {
          breathingValue = 255;
          breathingDirection = -1;
        } else if (breathingValue <= 0) {
          breathingValue = 0;
          breathingDirection = 1;
        }
        
        // Built-in LED is digital only
        digitalWrite(LED_PIN, breathingValue > 128);
        
        // External LED can use PWM
        if (useExternalLED) {
          analogWrite(EXTERNAL_LED, (breathingValue * ledBrightness) / 255);
        }
        
        lastLEDUpdate = currentTime;
      }
      break;
      
    case LED_PATTERN:
      // Custom pattern: 3 fast blinks, pause, repeat
      static int patternStep = 0;
      if (currentTime - lastLEDUpdate >= 150) {
        if (patternStep < 6) {
          // 3 fast blinks (on-off-on-off-on-off)
          ledState = (patternStep % 2 == 0);
          digitalWrite(LED_PIN, ledState);
          if (useExternalLED) {
            analogWrite(EXTERNAL_LED, ledState ? ledBrightness : 0);
          }
          patternStep++;
        } else if (patternStep < 10) {
          // Pause (off)
          digitalWrite(LED_PIN, LOW);
          if (useExternalLED) digitalWrite(EXTERNAL_LED, LOW);
          patternStep++;
        } else {
          patternStep = 0; // Reset pattern
        }
        lastLEDUpdate = currentTime;
      }
      break;
  }
}

void setLEDMode(LEDMode mode) {
  currentLEDMode = mode;
  ledState = false;
  breathingValue = 0;
  breathingDirection = 1;
  lastLEDUpdate = 0;
  
  // Turn off LEDs when changing modes
  digitalWrite(LED_PIN, LOW);
  digitalWrite(EXTERNAL_LED, LOW);
  
  Serial.println("💡 LED mode changed to: " + String(mode));
}

// WiFi and server functions (same as base version)
void setupWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);
  
  if (wifiSSID.length() > 0) {
    Serial.println("📡 Connecting to WiFi: " + wifiSSID);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
      updateLED(); // Keep LED updating during connection
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ WiFi connected!");
      Serial.println("🌐 IP: " + WiFi.localIP().toString());
      
      // Connected pattern: 5 quick blinks
      for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        if (useExternalLED) analogWrite(EXTERNAL_LED, ledBrightness);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        if (useExternalLED) digitalWrite(EXTERNAL_LED, LOW);
        delay(100);
      }
      
      captivePortalActive = false;
      return;
    } else {
      Serial.println("\n❌ WiFi connection failed");
    }
  }
  
  Serial.println("🌐 Starting captive portal...");
  startCaptivePortal();
}

void startCaptivePortal() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP);
  
  WiFi.softAPConfig(AP_IP, AP_IP, AP_SUBNET);
  
  bool apStarted = WiFi.softAP(AP_SSID, AP_PASS);
  if (!apStarted) {
    Serial.println("❌ Failed to start Access Point");
    delay(5000);
    ESP.restart();
    return;
  }
  
  delay(500);
  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  if (!dnsServer.start(53, "*", AP_IP)) {
    Serial.println("❌ Failed to start DNS server");
  }
  
  server.on("/", handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.on("/scan", handleWiFiScan);
  server.on("/led-mode", HTTP_POST, []() {
    String mode = server.arg("mode");
    String brightness = server.arg("brightness");
    String interval = server.arg("interval");
    String external = server.arg("external_led");
    
    if (mode.length() > 0) {
      setLEDMode((LEDMode)mode.toInt());
    }
    if (brightness.length() > 0) {
      ledBrightness = constrain(brightness.toInt(), 0, 255);
    }
    if (interval.length() > 0) {
      blinkInterval = constrain(interval.toInt(), 100, 5000);
    }
    useExternalLED = (external == "true");
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });
  server.onNotFound(handleNotFound);
  
  server.begin();
  captivePortalActive = true;
  
  // Set captive portal LED mode
  setLEDMode(LED_FAST_BLINK);
  
  Serial.println("✅ Captive portal started");
  Serial.printf("   🌐 SSID: %s\n", AP_SSID);
  Serial.printf("   📡 IP: %s\n", WiFi.softAPIP().toString().c_str());
}

void handleRoot() {
  String html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>ESP32 LED OTA Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 500px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
        .header { text-align: center; color: #333; margin-bottom: 20px; }
        .section { margin: 20px 0; padding: 15px; border-left: 4px solid #2196F3; background: #f9f9f9; }
        input, select { width: 100%; padding: 10px; margin: 5px 0; box-sizing: border-box; border: 1px solid #ddd; border-radius: 4px; }
        button { background: #2196F3; color: white; padding: 15px; border: none; width: 100%; font-size: 16px; border-radius: 4px; cursor: pointer; }
        button:hover { background: #1976D2; }
        .refresh-btn { background: #FF9800; margin-top: 5px; padding: 8px; }
        .led-controls { background: #e8f5e8; }
        .led-button { background: #4CAF50; margin: 5px; padding: 10px; width: auto; display: inline-block; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h2>ESP32 LED OTA</h2>
            <p>LED Patterns + Firmware Updates</p>
        </div>
        
        <div class="section led-controls">
            <h3>LED Controls</h3>
            <button class="led-button" onclick="setLEDMode(0)">Heartbeat</button>
            <button class="led-button" onclick="setLEDMode(1)">Fast Blink</button>
            <button class="led-button" onclick="setLEDMode(2)">Slow Blink</button>
            <button class="led-button" onclick="setLEDMode(3)">Breathing</button>
            <button class="led-button" onclick="setLEDMode(4)">Pattern</button>
            <br><br>
            <label>Brightness: <input type="range" id="brightness" min="0" max="255" value="255" onchange="updateLED()"></label>
            <label>Blink Interval (ms): <input type="number" id="interval" min="100" max="5000" value="1000" onchange="updateLED()"></label>
            <label><input type="checkbox" id="external_led" onchange="updateLED()"> Use External LED (GPIO 4)</label>
        </div>
        
        <form action="/submit" method="post">
            <div class="section">
                <h3>Device Information</h3>
                <input type="text" name="device_name" placeholder="Device Name" required>
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
                <h3>OTA Server Configuration</h3>
                <input type="url" name="server_url" placeholder="Server URL (e.g., http://192.168.1.100:5000)" required>
                <input type="text" name="ota_username" placeholder="OTA Username (optional)">
                <input type="password" name="ota_password" placeholder="OTA Password (optional)">
            </div>
            
            <button type="submit">Save &amp; Connect</button>
        </form>
    </div>
    
    <script>
        function setLEDMode(mode) {
            fetch('/led-mode', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'mode=' + mode + '&brightness=' + document.getElementById('brightness').value + 
                      '&interval=' + document.getElementById('interval').value +
                      '&external_led=' + document.getElementById('external_led').checked
            });
        }
        
        function updateLED() {
            const brightness = document.getElementById('brightness').value;
            const interval = document.getElementById('interval').value;
            const external = document.getElementById('external_led').checked;
            
            fetch('/led-mode', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'brightness=' + brightness + '&interval=' + interval + '&external_led=' + external
            });
        }
        
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

void handleSubmit() {
  deviceName = server.arg("device_name");
  
  wifiSSID = server.arg("wifi_ssid");
  if (wifiSSID == "custom") {
    wifiSSID = server.arg("custom_wifi_ssid");
  }
  
  wifiPassword = server.arg("wifi_password");
  serverURL = server.arg("server_url");
  otaUsername = server.arg("ota_username");
  otaPassword = server.arg("ota_password");
  
  saveConfiguration();
  
  server.send(200, "text/html; charset=utf-8", 
    "<html><body style='font-family:Arial; text-align:center; padding:50px;'>"
    "<h2>Configuration Saved!</h2>"
    "<p>Device will restart and begin LED patterns with OTA monitoring...</p>"
    "</body></html>");
  
  delay(2000);
  ESP.restart();
}

void handleWiFiScan() {
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
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "Redirecting to captive portal");
}

void loadConfiguration() {
  nvs.begin("esp32led", false);
  
  deviceName = nvs.getString("device_name", "");
  wifiSSID = nvs.getString("wifi_ssid", "");
  wifiPassword = nvs.getString("wifi_password", "");
  serverURL = nvs.getString("server_url", "");
  otaUsername = nvs.getString("ota_username", "");
  otaPassword = nvs.getString("ota_password", "");
  
  // Load LED settings
  currentLEDMode = (LEDMode)nvs.getInt("led_mode", LED_HEARTBEAT);
  ledBrightness = nvs.getInt("led_brightness", 255);
  blinkInterval = nvs.getInt("blink_interval", 1000);
  useExternalLED = nvs.getBool("external_led", false);
  
  Serial.println("Configuration loaded:");
  Serial.printf("   Device: %s\n", deviceName.c_str());
  Serial.printf("   WiFi: %s\n", wifiSSID.length() > 0 ? wifiSSID.c_str() : "Not configured");
  Serial.printf("   Server: %s\n", serverURL.length() > 0 ? serverURL.c_str() : "Not configured");
  Serial.printf("   LED Mode: %d, Brightness: %d\n", currentLEDMode, ledBrightness);
  
  nvs.end();
}

void saveConfiguration() {
  nvs.begin("esp32led", false);
  
  nvs.putString("device_name", deviceName);
  nvs.putString("wifi_ssid", wifiSSID);
  nvs.putString("wifi_password", wifiPassword);
  nvs.putString("server_url", serverURL);
  nvs.putString("ota_username", otaUsername);
  nvs.putString("ota_password", otaPassword);
  
  // Save LED settings
  nvs.putInt("led_mode", currentLEDMode);
  nvs.putInt("led_brightness", ledBrightness);
  nvs.putInt("blink_interval", blinkInterval);
  nvs.putBool("external_led", useExternalLED);
  
  nvs.end();
  Serial.println("✅ Configuration saved");
}

void checkForOTAUpdate() {
  if (serverURL.length() == 0 || otaInProgress) {
    return;
  }
  
  Serial.println("🔄 Checking for OTA updates...");
  
  HTTPClient http;
  http.begin(serverURL + "/api/ota/check/" + deviceID);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-Version", currentFirmwareVersion);
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("✅ OTA check response: " + response);
    
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);
    
    if (doc["update_available"].as<bool>()) {
      String firmwareURL = doc["firmware_url"];
      String newVersion = doc["version"];
      
      Serial.println("🔄 Update available: " + newVersion);
      Serial.println("📦 Firmware URL: " + firmwareURL);
      
      performOTAUpdate(firmwareURL);
    } else {
      Serial.println("✅ No update available - current version: " + currentFirmwareVersion);
    }
  } else {
    Serial.printf("❌ OTA check failed: HTTP %d\n", httpResponseCode);
  }
  
  http.end();
}

void performOTAUpdate(String firmwareURL) {
  otaInProgress = true;
  Serial.println("🔄 Starting OTA update...");
  Serial.println("📦 URL: " + firmwareURL);
  
  // Special OTA update LED pattern
  LEDMode oldMode = currentLEDMode;
  setLEDMode(LED_FAST_BLINK);
  
  WiFiClient client;
  HTTPClient httpClient;
  
  // Configure HTTP client
  httpClient.begin(client, firmwareURL);
  httpClient.setTimeout(30000); // 30 second timeout
  
  if (otaUsername.length() > 0 && otaPassword.length() > 0) {
    httpClient.setAuthorization(otaUsername.c_str(), otaPassword.c_str());
  }
  
  // Configure HTTPUpdate with the HTTP client
  HTTPUpdate httpUpdate;
  httpUpdate.setLedPin(LED_PIN, LOW); // LED on during update
  
  httpUpdate.onStart([]() {
    Serial.println("🔄 OTA update started");
  });
  
  httpUpdate.onEnd([]() {
    Serial.println("✅ OTA update finished");
  });
  
  httpUpdate.onProgress([](int cur, int total) {
    Serial.printf("🔄 OTA Progress: %d%%\n", (cur * 100) / total);
    // Flash LED to show progress
    static unsigned long lastFlash = 0;
    if (millis() - lastFlash > 100) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastFlash = millis();
    }
  });
  
  httpUpdate.onError([](int err) {
    Serial.printf("❌ OTA Error: %d\n", err);
  });
  
  // Use the correct ESP32 Core 3.x API
  HTTPUpdateResult ret = httpUpdate.update(httpClient);
  
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("❌ OTA failed: Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      otaInProgress = false;
      setLEDMode(oldMode); // Restore previous mode
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("ℹ️ No updates available");
      otaInProgress = false;
      setLEDMode(oldMode); // Restore previous mode
      break;
    case HTTP_UPDATE_OK:
      Serial.println("✅ OTA update successful - rebooting...");
      ESP.restart();
      break;
  }
  
  httpClient.end();
}

void sendHeartbeat() {
  if (serverURL.length() == 0) {
    return;
  }
  
  HTTPClient http;
  http.begin(serverURL + "/api/devices/" + deviceID + "/heartbeat");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(512);
  doc["device_id"] = deviceID;
  doc["device_name"] = deviceName;
  doc["version"] = currentFirmwareVersion;
  doc["uptime"] = millis() / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["led_mode"] = currentLEDMode;
  doc["led_brightness"] = ledBrightness;
  doc["external_led"] = useExternalLED;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    Serial.printf("💓 Heartbeat sent (HTTP %d)\n", httpResponseCode);
  } else {
    Serial.printf("❌ Heartbeat failed (HTTP %d)\n", httpResponseCode);
  }
  
  http.end();
}