/*
 * ESP32 DevKit V1 - OTA Update Base Version with Dashboard & Image Support
 * Full PersonalCMS functionality with OTA capabilities, WiFi captive portal, and content display
 * 
 * Features:
 * - WiFi captive portal with credential saving
 * - OTA update check every 2 minutes
 * - Dashboard BMP download and display capability
 * - Image assignment download and conversion
 * - Server-side BMP generation support
 * - PersonalCMS content management integration
 * - Simple LED status indicator
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
String deviceID = "ESP32_OTA_BASE_001";
String deviceName = "";
String occupation = "";
String serverURL = "";
String otaUsername = "";
String otaPassword = "";

// WiFi Configuration
String wifiSSID = "";
String wifiPassword = "";
WebServer server(80);
DNSServer dnsServer;

// Captive Portal Configuration
const char* AP_SSID = "ESP32-OTA-Setup";
const char* AP_PASS = "12345678";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

// Hardware Pins
const int LED_PIN = 2;   // Built-in LED
const int BUTTON_PIN = 0; // GPIO0 - Boot button (available on most ESP32 dev boards)

// Display Configuration (for future use - currently headless)
const int EPD_WIDTH = 800;
const int EPD_HEIGHT = 480;
const int EPD_ARRAY = (EPD_WIDTH * EPD_HEIGHT) / 8; // 1 bit per pixel for e-ink

// Button handling
bool lastButtonState = HIGH;
unsigned long buttonPressTime = 0;
unsigned long lastButtonCheck = 0;
const unsigned long BUTTON_DEBOUNCE_DELAY = 50; // 50ms debounce
const unsigned long LONG_PRESS_TIME = 2000; // 2 seconds for long press

// Content Management
bool currentMode = false;   // false = dashboard, true = images
unsigned char* dashboardBuffer = nullptr;
unsigned char* imageBuffer = nullptr;
bool dashboardLoaded = false;
bool imageLoaded = false;
unsigned long dashboardStartTime = 0;
String lastDashboardURL = "";
String lastImageURL = "";

// OTA Configuration
const unsigned long OTA_CHECK_INTERVAL = 120000; // 2 minutes
const unsigned long CONTENT_CHECK_INTERVAL = 30000; // 30 seconds
unsigned long lastOTACheck = 0;
unsigned long lastContentCheck = 0;
String currentFirmwareVersion = "2.4.0";
bool otaInProgress = false;

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
void blinkLED(int times, int delayMs = 200);
void registerDevice();
void checkServerForContent();
void downloadAndDisplayDashboard(String url);
void downloadAndDisplayImage(String url);
void flipBMPVertically(uint8_t* bmpData, int width, int height);
void initializeBuffers();
void switchMode();
void handleButton();
void checkButtonPress();

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("");
  Serial.println("=============================================================================");
  Serial.println("🚀 ESP32 DevKit V1 - OTA Base with PersonalCMS Support");
  Serial.println("📡 Dashboard & Image download capabilities");
  Serial.println("🔄 Version: " + currentFirmwareVersion);
  Serial.println("=============================================================================");
  
  // Hardware initialization
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize button pin with internal pull-up
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.printf("🔘 Button configured on GPIO %d (LOW = pressed)\n", BUTTON_PIN);
  
  // Initialize content buffers
  initializeBuffers();
  
  // Load configuration
  loadConfiguration();
  
  // Show startup indication
  blinkLED(3, 100);
  
  // Setup WiFi
  setupWiFi();
  
  Serial.println("✅ Setup complete - OTA base system with PersonalCMS ready");
  Serial.println("🔄 OTA checks will run every 2 minutes");
  Serial.println("📊 Content checks will run every 30 seconds");
  Serial.printf("🔘 GPIO %d button: Switch Dashboard ↔ Images (SHORT PRESS < 2s)\n", BUTTON_PIN);
  Serial.println("=============================================================================");
}

void loop() {
  static unsigned long lastStatusReport = 0;
  
  // Handle captive portal if active
  if (captivePortalActive) {
    dnsServer.processNextRequest();
    server.handleClient();
    return;
  }
  
  // Main operation when WiFi connected
  if (WiFi.status() == WL_CONNECTED) {
    // Handle button press for mode switching
    handleButton();
    
    // Check for OTA updates every 2 minutes
    if (millis() - lastOTACheck >= OTA_CHECK_INTERVAL) {
      checkForOTAUpdate();
      lastOTACheck = millis();
    }
    
    // Check for content updates every 30 seconds
    if (millis() - lastContentCheck >= CONTENT_CHECK_INTERVAL) {
      checkServerForContent();
      lastContentCheck = millis();
    }
    
    // Send heartbeat every 30 seconds
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat >= 30000) {
      sendHeartbeat();
      lastHeartbeat = millis();
    }
    
    // Status LED heartbeat (quick blink every 5 seconds)
    static unsigned long lastLEDBlink = 0;
    if (millis() - lastLEDBlink >= 5000) {
      digitalWrite(LED_PIN, HIGH);
      delay(50);
      digitalWrite(LED_PIN, LOW);
      lastLEDBlink = millis();
    }
  }
  
  // Status reporting every 30 seconds
  if (millis() - lastStatusReport >= 30000) {
    Serial.println("📊 PersonalCMS OTA Base Status:");
    Serial.printf("   ⏰ Uptime: %lu seconds\n", millis() / 1000);
    Serial.printf("   📡 WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");
    Serial.printf("   🔄 Version: %s\n", currentFirmwareVersion.c_str());
    Serial.printf("   💾 Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("   🔄 Next OTA check in: %lu seconds\n", (OTA_CHECK_INTERVAL - (millis() - lastOTACheck)) / 1000);
    Serial.printf("   📊 Next content check in: %lu seconds\n", (CONTENT_CHECK_INTERVAL - (millis() - lastContentCheck)) / 1000);
    Serial.printf("   🎛️ Current mode: %s\n", currentMode ? "IMAGE" : "DASHBOARD");
    Serial.printf("   📊 Dashboard loaded: %s\n", dashboardLoaded ? "YES" : "NO");
    Serial.printf("   🖼️ Image loaded: %s\n", imageLoaded ? "YES" : "NO");
    lastStatusReport = millis();
  }
  
  delay(100);
}

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
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ WiFi connected!");
      Serial.println("🌐 IP: " + WiFi.localIP().toString());
      digitalWrite(LED_PIN, HIGH);
      captivePortalActive = false;
      
      // Register device with server
      delay(2000); // Wait for connection to stabilize
      registerDevice();
      
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
  server.onNotFound(handleNotFound);
  
  server.begin();
  captivePortalActive = true;
  
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
    <title>ESP32 OTA Base Setup</title>
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
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h2>ESP32 OTA Base</h2>
            <p>Firmware Update System</p>
        </div>
        
        <form action="/submit" method="post">
            <div class="section">
                <h3>Device Information</h3>
                <input type="text" name="device_name" placeholder="Device Name" required>
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
                <h3>OTA Server Configuration</h3>
                <input type="url" name="server_url" placeholder="Server URL (e.g., http://192.168.1.100:5000)" required>
                <input type="text" name="ota_username" placeholder="OTA Username (optional)">
                <input type="password" name="ota_password" placeholder="OTA Password (optional)">
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

void handleSubmit() {
  deviceName = server.arg("device_name");
  occupation = server.arg("occupation");
  
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
    "<p>Device will restart and begin OTA monitoring...</p>"
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
  nvs.begin("esp32ota", false);
  
  deviceName = nvs.getString("device_name", "");
  occupation = nvs.getString("occupation", "");
  wifiSSID = nvs.getString("wifi_ssid", "");
  wifiPassword = nvs.getString("wifi_password", "");
  serverURL = nvs.getString("server_url", "");
  otaUsername = nvs.getString("ota_username", "");
  otaPassword = nvs.getString("ota_password", "");
  
  Serial.println("Configuration loaded:");
  Serial.printf("   Device: %s\n", deviceName.c_str());
  Serial.printf("   WiFi: %s\n", wifiSSID.length() > 0 ? wifiSSID.c_str() : "Not configured");
  Serial.printf("   Server: %s\n", serverURL.length() > 0 ? serverURL.c_str() : "Not configured");
  
  nvs.end();
}

void saveConfiguration() {
  nvs.begin("esp32ota", false);
  
  nvs.putString("device_name", deviceName);
  nvs.putString("occupation", occupation);
  nvs.putString("wifi_ssid", wifiSSID);
  nvs.putString("wifi_password", wifiPassword);
  nvs.putString("server_url", serverURL);
  nvs.putString("ota_username", otaUsername);
  nvs.putString("ota_password", otaPassword);
  
  nvs.end();
  Serial.println("✅ Configuration saved");
}

void registerDevice() {
  if (serverURL.length() == 0) {
    Serial.println("❌ No server URL configured");
    return;
  }
  
  Serial.println("📡 Registering device with PersonalCMS server");
  Serial.println("🔗 Server URL: " + serverURL);
  Serial.println("🏷️  Device ID: " + deviceID);
  
  HTTPClient http;
  http.begin(serverURL + "/api/devices/register");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(1024);
  doc["device_id"] = deviceID;
  doc["device_name"] = deviceName.length() > 0 ? deviceName : "ESP32 PersonalCMS Base";
  doc["occupation"] = "Personal CMS Device";
  doc["device_type"] = "ESP32_DevKit_V1";
  doc["wifi_ssid"] = WiFi.SSID();
  doc["firmware_version"] = currentFirmwareVersion;
  doc["capabilities"] = "dashboard,images,ota,bmp_download";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.printf("✅ Device registration response: %d\n", httpResponseCode);
    if (httpResponseCode == 200 || httpResponseCode == 201) {
      Serial.println("✅ Device registered successfully with PersonalCMS");
    } else {
      Serial.println("⚠️  Device registration returned: " + response);
    }
  } else {
    Serial.printf("❌ Device registration failed: %d\n", httpResponseCode);
  }
  
  http.end();
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
  http.addHeader("X-Device-Type", "ESP32_OTA_Base");
  
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
  
  // Blink LED rapidly during update
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  
  WiFiClient client;
  HTTPClient httpClient;
  
  // Configure HTTP client
  httpClient.begin(client, firmwareURL);
  httpClient.setTimeout(30000); // 30 second timeout
  
  // Set authentication if provided
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
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("ℹ️ No updates available");
      otaInProgress = false;
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
  doc["occupation"] = occupation.length() > 0 ? occupation : "Device User";
  doc["device_type"] = "ESP32_OTA_Base";
  doc["version"] = currentFirmwareVersion;
  doc["uptime"] = millis() / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["current_mode"] = currentMode ? "images" : "dashboard";
  doc["dashboard_loaded"] = dashboardLoaded;
  doc["image_loaded"] = imageLoaded;
  
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

void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}

// PersonalCMS Content Management Functions

void initializeBuffers() {
  Serial.println("🧠 Initializing content buffers...");
  
  // Allocate memory for content buffers
  dashboardBuffer = (unsigned char*)malloc(EPD_ARRAY);
  imageBuffer = (unsigned char*)malloc(EPD_ARRAY);
  
  if (dashboardBuffer == nullptr || imageBuffer == nullptr) {
    Serial.println("❌ Failed to allocate content buffers - low memory!");
    Serial.printf("   Free heap: %d bytes, Required: %d bytes each\n", ESP.getFreeHeap(), EPD_ARRAY);
    Serial.println("   Running in headless mode without content buffers");
  } else {
    Serial.printf("✅ Content buffers allocated: %d bytes each\n", EPD_ARRAY);
    Serial.printf("   📊 Dashboard buffer: %p\n", dashboardBuffer);
    Serial.printf("   🖼️ Image buffer: %p\n", imageBuffer);
    
    // Initialize buffers to white (0xFF for e-ink)
    memset(dashboardBuffer, 0xFF, EPD_ARRAY);
    memset(imageBuffer, 0xFF, EPD_ARRAY);
  }
}

void checkServerForContent() {
  if (serverURL.length() == 0) {
    Serial.println("❌ No server URL configured - skipping content check");
    return;
  }
  
  Serial.println("📊 Checking PersonalCMS server for content updates...");
  
  HTTPClient http;
  http.begin(serverURL + "/api/devices/" + deviceID + "/content");
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("✅ Content check successful");
    
    // Parse JSON response
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
      Serial.printf("❌ JSON parsing failed: %s\n", error.c_str());
      http.end();
      return;
    }
    
    // Show dashboard URL
    String dashboard_url = doc["dashboard_url"];
    if (dashboard_url.length() > 0) {
      String full_dashboard_url = serverURL + dashboard_url;
      Serial.println("📊 Dashboard BMP: " + full_dashboard_url);
      
      // Download dashboard if in dashboard mode and (URL changed or not loaded yet)
      if (!currentMode && (full_dashboard_url != lastDashboardURL || !dashboardLoaded)) {
        lastDashboardURL = full_dashboard_url;
        downloadAndDisplayDashboard(full_dashboard_url);
      } else if (!currentMode) {
        Serial.println("   ℹ️ Dashboard already current (dashboard mode)");
      } else {
        Serial.println("   ℹ️ Dashboard available (image mode active)");
      }
    } else {
      Serial.println("   ℹ️ No dashboard URL provided");
    }
    
    // Show available images
    JsonArray images = doc["images"];
    if (images.size() > 0) {
      Serial.printf("🖼️ Available images: %d\n", images.size());
      
      // Download image if in image mode and (URL changed or not loaded yet)
      JsonObject firstImage = images[0];
      String image_url = firstImage["bmp_url"];
      
      if (image_url.length() > 0) {
        String full_image_url = serverURL + image_url;
        Serial.println("🖼️ Image BMP: " + full_image_url);
        
        // Download image if in image mode and (URL changed or not loaded yet)
        if (currentMode && (full_image_url != lastImageURL || !imageLoaded)) {
          lastImageURL = full_image_url;
          downloadAndDisplayImage(full_image_url);
        } else if (currentMode) {
          Serial.println("   ℹ️ Image already current (image mode)");
        } else {
          Serial.println("   ℹ️ Image available (dashboard mode active)");
        }
      }
    } else {
      Serial.println("   ℹ️ No images available");
    }
    
  } else {
    Serial.printf("❌ Content check failed: HTTP %d\n", httpResponseCode);
    if (httpResponseCode > 0) {
      Serial.println("Error response: " + http.getString());
    }
  }
  
  http.end();
}

void downloadAndDisplayDashboard(String url) {
  Serial.println("📊 Downloading dashboard from: " + url);
  
  if (dashboardBuffer == nullptr) {
    Serial.println("❌ Dashboard buffer not allocated - running headless");
    Serial.println("   📊 Dashboard URL logged but not downloaded");
    return;
  }
  
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
      
      // Read BMP pixel data directly into dashboard buffer
      size_t bmpDataSize = contentLength - dataOffset;
      size_t bytesToRead = min(bmpDataSize, (size_t)EPD_ARRAY);
      
      Serial.println("🔄 Reading BMP pixel data: " + String(bytesToRead) + " bytes");
      size_t totalRead = stream->readBytes(dashboardBuffer, bytesToRead);
      
      if (totalRead == bytesToRead) {
        Serial.println("✅ BMP data loaded into buffer: " + String(totalRead) + " bytes");
        
        // Fix BMP orientation (flip vertically to fix upside-down display)
        flipBMPVertically(dashboardBuffer, EPD_WIDTH, EPD_HEIGHT);
        
        Serial.println("💻 Dashboard data ready (headless mode - no display connected)");
        
        dashboardLoaded = true;
        dashboardStartTime = millis();
        
        Serial.println("✅ Dashboard processing complete");
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
  
  if (imageBuffer == nullptr) {
    Serial.println("❌ Image buffer not allocated - running headless");
    Serial.println("   🖼️ Image URL logged but not downloaded");
    return;
  }
  
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
      
      // Read BMP pixel data directly into image buffer
      size_t bmpDataSize = contentLength - dataOffset;
      size_t bytesToRead = min(bmpDataSize, (size_t)EPD_ARRAY);
      
      Serial.println("🔄 Reading BMP pixel data: " + String(bytesToRead) + " bytes");
      size_t totalRead = stream->readBytes(imageBuffer, bytesToRead);
      
      if (totalRead == bytesToRead) {
        Serial.println("✅ BMP data loaded into buffer: " + String(totalRead) + " bytes");
        
        // Fix BMP orientation (flip vertically to fix upside-down display)
        flipBMPVertically(imageBuffer, EPD_WIDTH, EPD_HEIGHT);
        
        Serial.println("💻 Image data ready (headless mode - no display connected)");
        
        imageLoaded = true;
        
        Serial.println("✅ Image processing complete");
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

void switchMode() {
  currentMode = !currentMode;
  Serial.printf("🔄 Mode switched to: %s\n", currentMode ? "IMAGES" : "DASHBOARD");
  
  // Visual feedback - blink LED to indicate mode change
  blinkLED(currentMode ? 2 : 1, 150); // 2 blinks for images, 1 blink for dashboard
  
  // Force immediate content check to display new mode content
  Serial.println("🔄 Triggering immediate content check for new mode...");
  checkServerForContent();
  
  Serial.println("✅ Mode switch complete - content updated");
}

void handleButton() {
  // Check button state with debouncing
  if (millis() - lastButtonCheck >= BUTTON_DEBOUNCE_DELAY) {
    checkButtonPress();
    lastButtonCheck = millis();
  }
}

void checkButtonPress() {
  bool currentButtonState = digitalRead(BUTTON_PIN);
  
  // Button pressed (LOW due to pull-up)
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    buttonPressTime = millis();
    Serial.println("🔘 Button pressed - monitoring for release...");
  }
  
  // Button released
  else if (currentButtonState == HIGH && lastButtonState == LOW) {
    unsigned long pressDuration = millis() - buttonPressTime;
    
    if (pressDuration >= LONG_PRESS_TIME) {
      // Long press (2+ seconds) - Future use (device reset, etc.)
      Serial.printf("🔘 Long press detected (%lu ms) - Reserved for future use\n", pressDuration);
      Serial.println("   💡 Currently: Long press reserved for advanced functions");
      
      // Visual feedback for long press
      blinkLED(5, 100); // 5 quick blinks
    }
    else if (pressDuration >= 50) {
      // Short press - Switch mode
      Serial.printf("🔘 Short press detected (%lu ms) - Switching mode\n", pressDuration);
      switchMode();
    }
    
    buttonPressTime = 0;
  }
  
  lastButtonState = currentButtonState;
}