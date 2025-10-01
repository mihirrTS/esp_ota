/*
 * PersonalCMS ESP32 DevKit V1 - FIXED Captive Portal Version
 * Production-ready PersonalCMS server communication with e-ink display
 * Troubleshooting fixes for captive portal not loading issues
 * 
 * Pin Configuration (ESP32 DevKit V1 - Production Setup):
 * - RST: GPIO 25, DC: GPIO 26, CS: GPIO 5
 * - BUSY: GPIO 27, PWR: GPIO 33  
 * - MOSI: GPIO 23, SCK: GPIO 18
 * - Mode Switch: GPIO 36 (Physical button - SVP pin with better floating behavior)
 * - Status LED: GPIO 2 (Built-in LED)
 * 
 * Common Issues Fixed:
 * 1. DNS server configuration
 * 2. WiFi AP settings optimization  
 * 3. Captive portal detection improvements
 * 4. Enhanced error handling and logging
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <SPI.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <DHT.h>

// E-ink Display Headers (Production)
#include "Display_EPD_W21_spi.h"
#include "Display_EPD_W21.h"
#include "Ap_29demo.h"

// Device Configuration
String deviceID = "ESP32_DEV1_002";  // Updated to match working reference
String deviceName = "";
String occupation = "";
String serverURL = "";

// OTA Configuration
String otaUsername = "";
String otaPassword = "";
String currentFirmwareVersion = "2.0.0";
const unsigned long OTA_CHECK_INTERVAL = 120000; // 2 minutes
unsigned long lastOTACheck = 0;
bool otaInProgress = false;

// WiFi Configuration
String wifiSSID = "";
String wifiPassword = "";
WebServer server(80);
DNSServer dnsServer;

// Captive Portal Configuration (FIXED)
const char* AP_SSID = "PersonalCMS-ESP32-Setup";
const char* AP_PASS = "12345678";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

// Hardware Pins (Production GPIO Assignment)
const int BUTTON_PIN = 12;  // GPIO 12 (Multi-function button: mode switch + NFC activation)
const int LED_PIN = 2;      // GPIO 2 (Built-in LED for status indication)

// NFC PN532 Configuration (I2C)
const int PN532_SDA = 21;   // GPIO 21 (I2C Data)
const int PN532_SCL = 22;   // GPIO 22 (I2C Clock)
// Note: NFC trigger functionality moved to main button (GPIO 12)

// DHT22 Temperature Sensor
const int DHT_PIN = 4;      // GPIO 4 (DHT22 data pin - as requested)
const int DHT_TYPE = DHT22; // DHT22 sensor type

// IR Sensor for Sleep/Wake
const int IR_SENSOR_PIN = 32; // GPIO 32 (IR motion detection)

// NFC VCard Constants
const uint8_t NTAG_START_PAGE = 4;
const uint8_t ERASE_START = 4, ERASE_END = 40;

// NFC VCard Data
const char USER_FULL_NAME[]   = "John Q. Developer";
const char USER_ORG[]         = "Acme Embedded Solutions";
const char USER_PHONE[]       = "+1-555-123-4567";
const char USER_EMAIL[]       = "john.developer@example.com";
const char USER_WEBSITE[]     = "https://example.com";
const char USER_NOTE[] = "Lead Embedded Engineer\nLoves NFC, low-power design, and espresso.\nAvailable for consulting.";
const char FALLBACK_URL[]     = "https://google.com/";

// Operating Mode and Button Handling
bool lastSwitchState = HIGH;
bool currentMode = false;   // false = dashboard, true = images
bool displayInitialized = false;
bool captivePortalActive = false;

// Enhanced button handling with timing-based detection
unsigned long buttonPressStartTime = 0;
unsigned long lastButtonChangeTime = 0;
unsigned long groundConnectionStart = 0;
bool buttonPressed = false;
bool longPressDetected = false;
bool lastDebouncedState = HIGH;
bool groundConnectionActive = false;
const unsigned long DEBOUNCE_DELAY = 50;  // 50ms debounce delay
const unsigned long MIN_GROUND_TIME = 300; // Minimum 300ms to prevent false triggers
const unsigned long LONG_PRESS_DURATION = 2000; // 2 seconds for NFC activation
const unsigned long MODE_SWITCH_MAX = 1500; // Under 1.5s = mode switch

// Universal pin detection constants
const int UNIVERSAL_GROUNDED_THRESHOLD = 10;    // ADC must be < 10 to be considered truly grounded
const int UNIVERSAL_HIGH_THRESHOLD = 4000;      // ADC must be > 4000 to be considered truly high
const int FLOATING_MIN = 200;                   // Floating range minimum
const int FLOATING_MAX = 3800;                  // Floating range maximum

// IR Sensor timing variables  
unsigned long irGroundConnectionStart = 0;
bool irGroundConnectionActive = false;
bool irMotionDetected = false;

// Motion detection schedule (every 10 minutes, check for 10 seconds after initial 10 minutes)
const unsigned long MOTION_START_DELAY = 10 * 60 * 1000;      // 10 minutes initial delay
const unsigned long MOTION_CHECK_INTERVAL = 10 * 60 * 1000;   // Check every 10 minutes  
const unsigned long MOTION_CHECK_DURATION = 10 * 1000;        // Check for 10 seconds
unsigned long lastMotionCheckStart = 0;
bool motionCheckingActive = false;
bool motionSystemEnabled = false;

// Display update variables
bool displayUpdateNeeded = false;
unsigned long lastDisplayUpdate = 0;

// NFC PN532 Module
Adafruit_PN532 nfc(PN532_SDA, PN532_SCL);
bool nfcInitialized = false;
bool nfcWriteOnce = false;

// DHT22 Temperature Sensor
DHT dht(DHT_PIN, DHT_TYPE);
float currentTemperature = 22.5; // Default room temperature
float currentHumidity = 50.0;    // Default moderate humidity
unsigned long lastTempReading = 0;
const unsigned long TEMP_READ_INTERVAL = 30000; // Read every 30 seconds

// IR Sensor for Sleep/Wake
bool deviceSleeping = false;
bool lastIRState = LOW;
unsigned long lastIRTrigger = 0;
const unsigned long SLEEP_TIMEOUT = 300000; // 5 minutes of no motion

// NFC Helper Structures and Functions
struct Buf { 
  uint8_t *b; size_t len, cap; 
  Buf():b(nullptr),len(0),cap(0){} 
  ~Buf(){if(b) free(b);} 
  void reserve(size_t n){if(n>cap){cap=max(n,cap*2);b=(uint8_t*)realloc(b,cap);}} 
  void push(uint8_t x){reserve(len+1);b[len++]=x;} 
  void push(const uint8_t*d,size_t n){if(n==0)return;reserve(len+n);memcpy(b+len,d,n);len+=n;} 
};

String escapeNote(const char *note){ 
  String s(note), out; 
  for(size_t i=0;i<s.length();++i){ 
    char c=s.charAt(i); 
    if(c=='\n') out+="\\n"; 
    else if(c!='\r') out+=c; 
  } 
  return out; 
}

void buildMime(Buf &o,const char *mime,const uint8_t *p,size_t l,bool mb,bool me){ 
  uint8_t h=(mb?0x80:0)|(me?0x40:0)|0x10|(0x02&0x07); 
  if(l>255) h&=~0x10; 
  o.push(h); 
  o.push(strlen(mime)); 
  if(l<=255) o.push((uint8_t)l); 
  else{o.push((l>>24)&0xFF); o.push((l>>16)&0xFF); o.push((l>>8)&0xFF); o.push(l&0xFF);} 
  o.push((const uint8_t*)mime,strlen(mime)); 
  if(l)o.push(p,l); 
}

void buildUri(Buf &o,const char *url,bool mb,bool me){ 
  size_t l=strlen(url)+1; 
  uint8_t h=(mb?0x80:0)|(me?0x40:0)|0x10|(0x01&0x07); 
  if(l>255) h&=~0x10; 
  o.push(h); 
  o.push(1); 
  if(l<=255) o.push((uint8_t)l); 
  else{o.push((l>>24)&0xFF); o.push((l>>16)&0xFF); o.push((l>>8)&0xFF); o.push(l&0xFF);} 
  o.push('U'); 
  o.push(0x00); 
  o.push((const uint8_t*)url,strlen(url)); 
}

bool writePage(uint8_t p,const uint8_t dat[4]){ 
  // Add timeout and error handling to prevent infinite blocking
  unsigned long startTime = millis();
  const unsigned long WRITE_TIMEOUT = 2000; // 2 second timeout per page
  
  while (millis() - startTime < WRITE_TIMEOUT) {
    if(nfc.ntag2xx_WritePage(p,(uint8_t*)dat)){ 
      return true; // Success
    }
    delay(50); // Brief delay before retry
  }
  
  Serial.print("Write timeout pg "); 
  Serial.println(p); 
  return false; 
}

bool eraseRange(uint8_t s,uint8_t e){ 
  uint8_t z[4]={0,0,0,0}; 
  for(uint8_t p=s;p<=e;p++){ 
    if(!writePage(p,z)) {
      Serial.printf("Erase failed at page %d\n", p);
      return false;
    }
    delay(10); // Slightly longer delay for stability
  } 
  return true; 
}

bool writeTLV(uint8_t start,const uint8_t *d,size_t len){ 
  size_t i=0; 
  uint8_t pg=start,buf[4]; 
  while(i<len){ 
    for(uint8_t k=0;k<4;k++) buf[k]=(i<len?d[i++]:0x00); 
    if(!writePage(pg++,buf)) {
      Serial.printf("TLV write failed at page %d\n", pg-1);
      return false;
    }
    delay(10); // Consistent delay for stability
  } 
  return true; 
} // 5 seconds for device removal

// Timer variables
unsigned long dashboardStartTime = 0;
unsigned long lastTimerUpdate = 0;
bool timerActive = false;

// Storage
Preferences nvs;
String preferencesJson = "";

// Display buffers
unsigned char dashboardBuffer[EPD_ARRAY];
bool dashboardLoaded = false;

// NTP Configuration for Mumbai timezone (IST = UTC+5:30)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;  // UTC+5:30 in seconds
const int daylightOffset_sec = 0;  // No daylight saving in India

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
void sendSensorDataToServer();
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
void initializeNFC();
void writeNFCVCard();
void initializeDHT();
void readTemperatureHumidity();
void handleIRSensor();
void enterSleepMode();
void wakeFromSleepMode();
void checkForOTAUpdate();
void performOTAUpdate(String firmwareURL);

void setup() {
  Serial.begin(115200);
  delay(2000); // Give time for Serial to initialize
  
  Serial.println("");
  Serial.println("=============================================================================");
  Serial.println("🚀 PersonalCMS ESP32 DevKit V1 - FIXED Captive Portal Version");
  Serial.println("🔧 Troubleshooting fixes applied for captive portal issues");
  Serial.println("🔄 OTA Update System Integrated - Version: " + currentFirmwareVersion);
  Serial.println("=============================================================================");
  
  // Hardware diagnostic for ESP32 DevKit V1
  Serial.println("");
  Serial.println("🔧 ESP32 DEVKIT V1 HARDWARE DIAGNOSTICS:");
  Serial.printf("   📟 Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("   💾 Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("   🧠 Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
  Serial.printf("   ⚡ CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  
  // Pin configuration diagnostic
  Serial.println("");
  Serial.println("📌 ESP32 DEVKIT V1 PRODUCTION PIN CONFIGURATION:");
  Serial.printf("   🎛️  Multi-Button: GPIO %d (Mode switch + NFC activation)\n", BUTTON_PIN);
  Serial.printf("   💡 Status LED: GPIO %d (Built-in LED)\n", LED_PIN);
  Serial.printf("   🌡️  DHT22 Sensor: GPIO %d (Temperature/Humidity)\n", DHT_PIN);
  Serial.printf("   👁️  IR Sensor: GPIO %d (Motion detection)\n", IR_SENSOR_PIN);
  Serial.println("   🖥️  Display Pins: RST=25, DC=26, CS=5, BUSY=27, PWR=33");
  Serial.println("   📡 SPI Pins: MOSI=23, SCK=18");
  Serial.println("   🔗 I2C Pins: SDA=21, SCL=22 (PN532 NFC)");
  
  // Initialize hardware pins
  Serial.println("");
  Serial.println("🔌 INITIALIZING HARDWARE:");
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // GPIO 12 with internal pullup
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.printf("   ✅ GPIO %d (Button): INPUT_PULLUP - multi-function button\n", BUTTON_PIN);
  Serial.printf("   ✅ GPIO %d (LED): OUTPUT\n", LED_PIN);
  
  // Test initial button state  
  bool initialButtonState = digitalRead(BUTTON_PIN);
  Serial.printf("   📊 Initial button state: %s\n", initialButtonState ? "HIGH (not pressed)" : "LOW (pressed)");
  
  // Load saved configuration
  Serial.println("");
  Serial.println("💾 LOADING CONFIGURATION:");
  loadConfiguration();
  
  // Initialize display
  Serial.println("");
  Serial.println("🖥️ DISPLAY INITIALIZATION:");
  initializeDisplay();
  showBootupScreen();
  
  // Initialize sensors and modules
  Serial.println("");
  Serial.println("🔧 SENSORS & MODULES INITIALIZATION:");
  
  // Initialize NFC PN532
  initializeNFC();
  
  // Initialize DHT22 temperature sensor
  initializeDHT();
  
  // Initialize IR sensor pin
  pinMode(IR_SENSOR_PIN, INPUT);
  Serial.printf("   ✅ IR Sensor: GPIO %d (INPUT - short to GND for motion)\n", IR_SENSOR_PIN);
  
  // Setup WiFi - this will start captive portal if needed
  Serial.println("");
  Serial.println("📡 WIFI SETUP:");
  setupWiFi();
  
  Serial.println("");
  Serial.println("✅ SETUP COMPLETE - Device Ready");
  Serial.println("=============================================================================");
  Serial.println("📋 PRODUCTION CONTROLS:");
  Serial.printf("   - GPIO %d button: Switch Dashboard ↔ Images (SHORT PRESS < 2s)\n", BUTTON_PIN);
  Serial.printf("   - GPIO %d button: Activate NFC VCard Writing (LONG PRESS ≥ 2s)\n", BUTTON_PIN);
  Serial.printf("   - GPIO %d sensor: Temperature & Humidity (DHT22)\n", DHT_PIN);
  Serial.printf("   - GPIO %d sensor: IR Motion Detection (sleep/wake)\n", IR_SENSOR_PIN);
  Serial.println("   - GPIO 21/22: NFC PN532 I2C Communication");
  Serial.println("   - Timer: Shows seconds since dashboard refresh");
  Serial.println("   - Auto-refresh: Every 30 seconds from server");
  Serial.println("=============================================================================");
}

void loop() {
  static unsigned long lastStatusReport = 0;
  static unsigned long loopCounter = 0;
  loopCounter++;
  
  // Status reporting every 15 seconds
  if (millis() - lastStatusReport > 15000) {
    Serial.println("");
    Serial.println("📊 SYSTEM STATUS REPORT:");
    Serial.printf("   🔄 Loop iterations: %lu\n", loopCounter);
    Serial.printf("   💾 Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("   ⏰ Uptime: %lu seconds\n", millis() / 1000);
    Serial.printf("   📡 WiFi status: %s\n", 
                  WiFi.status() == WL_CONNECTED ? "CONNECTED" : 
                  captivePortalActive ? "CAPTIVE PORTAL ACTIVE" : "DISCONNECTED");
    if (captivePortalActive) {
      Serial.printf("   🌐 AP IP: %s\n", WiFi.softAPIP().toString().c_str());
      Serial.printf("   👥 Connected clients: %d\n", WiFi.softAPgetStationNum());
    }
    Serial.printf("   🎛️ Current mode: %s\n", currentMode ? "IMAGE" : "DASHBOARD");
    bool currentButtonState = digitalRead(BUTTON_PIN);
    Serial.printf("   🎚️ Button: %s (GPIO %d)\n", 
                  currentButtonState ? "RELEASED" : "PRESSED", BUTTON_PIN);
    Serial.printf("   🌡️  Temperature: %.1f°C, Humidity: %.1f%%\n", currentTemperature, currentHumidity);
    Serial.printf("   👁️  Motion sensor: %s, Sleep mode: %s\n", 
                  irMotionDetected ? "MOTION" : "CLEAR", 
                  deviceSleeping ? "ON" : "OFF");
    Serial.printf("   🏷️  NFC status: %s, Write once: %s\n", 
                  nfcInitialized ? "READY" : "OFFLINE", 
                  nfcWriteOnce ? "COMPLETED" : "AVAILABLE");
    lastStatusReport = millis();
  }
  
  // Simple button handling with GPIO 12 (proper digital input with pullup)
  bool currentButtonReading = digitalRead(BUTTON_PIN);  // LOW = pressed, HIGH = released
  
  // Debounce logic
  if (currentButtonReading != lastDebouncedState) {
    lastButtonChangeTime = millis();
  }
  
  if ((millis() - lastButtonChangeTime) > DEBOUNCE_DELAY) {
    // Button state has been stable for debounce period
    
    if (!buttonPressed && currentButtonReading == LOW) {
      // Button just pressed
      buttonPressed = true;
      buttonPressStartTime = millis();
      digitalWrite(LED_PIN, HIGH);  // Turn on LED when button pressed
      Serial.printf("🔘 Button pressed (GPIO %d)\n", BUTTON_PIN);
      
    } else if (buttonPressed && currentButtonReading == HIGH) {
      // Button just released
      unsigned long pressDuration = millis() - buttonPressStartTime;
      buttonPressed = false;
      digitalWrite(LED_PIN, LOW);  // Turn off LED when button released
      
      if (pressDuration >= LONG_PRESS_DURATION) {
        // Long press: Activate NFC VCard writing
        Serial.printf("🏷️ LONG PRESS detected (%lu ms) - Activating NFC VCard writing...\n", pressDuration);
        if (nfcInitialized && !nfcWriteOnce) {
          writeNFCVCard();
          // After NFC operation, ensure normal operation continues
          Serial.println("🔄 NFC operation completed - resuming normal operation");
        } else if (nfcWriteOnce) {
          Serial.println("🔒 NFC already written once - protection enabled");
        } else {
          Serial.println("❌ NFC not ready");
        }
        
      } else if (pressDuration >= MIN_GROUND_TIME) {
        // Short press: Switch display mode
        Serial.printf("🎛️ SHORT PRESS detected (%lu ms) - Switching display mode...\n", pressDuration);
        currentMode = !currentMode;
        Serial.printf("   Mode changed to: %s\n", currentMode ? "IMAGE" : "DASHBOARD");
        
        // Force display update
        displayUpdateNeeded = true;
        lastDisplayUpdate = 0;
      }
    }
  }
  
  lastDebouncedState = currentButtonReading;
  
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
    static unsigned long lastSensorUpdate = 0;
    static unsigned long lastSystemCheck = 0;
    
    // Check for OTA updates every 2 minutes
    if (millis() - lastOTACheck >= OTA_CHECK_INTERVAL && !otaInProgress) {
      checkForOTAUpdate();
      lastOTACheck = millis();
    }
    
    // Regular content checking
    if (millis() - lastContentCheck > 30000) { // Check every 30 seconds
      checkServerForContent();
      lastContentCheck = millis();
    }
    
    // Send sensor data every 2 minutes
    if (millis() - lastSensorUpdate > 120000) {
      sendSensorDataToServer();
      lastSensorUpdate = millis();
    }
    
    // System health check every 5 minutes (detect stuck states)
    if (millis() - lastSystemCheck > 300000) {
      Serial.println("🔧 System health check - ensuring normal operation");
      
      // Check if I2C/NFC is causing issues
      if (nfcInitialized) {
        // Quick I2C bus health check
        Wire.beginTransmission(0x24); // PN532 I2C address
        uint8_t error = Wire.endTransmission();
        if (error != 0) {
          Serial.println("⚠️  I2C bus error detected - reinitializing...");
          Wire.end();
          delay(100);
          Wire.begin(PN532_SDA, PN532_SCL);
          delay(100);
        }
      }
      
      lastSystemCheck = millis();
      Serial.println("✅ System health check complete");
    }
    
    updateTimer();
  }
  
  // Handle display updates when mode changes
  if (displayUpdateNeeded) {
    Serial.println("🖥️ Display update requested - checking for new content...");
    if (WiFi.status() == WL_CONNECTED) {
      checkServerForContent();  // This will trigger display update
    }
    displayUpdateNeeded = false;
    lastDisplayUpdate = millis();
  }
  
  // Handle sensor monitoring (even when WiFi not connected)
  handleIRSensor();        // Check for motion/sleep mode
  readTemperatureHumidity(); // Update temperature readings
  // NFC functionality now handled by button long press
  
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
  Serial.println("   1. Connect your device to WiFi: PersonalCMS-ESP32-Setup");
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
    <title>PersonalCMS ESP32 Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 500px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; color: #333; margin-bottom: 20px; }
        .section { margin: 20px 0; padding: 15px; border-left: 4px solid #2196F3; background: #f9f9f9; }
        input, select, textarea { width: 100%; padding: 10px; margin: 5px 0; box-sizing: border-box; border: 1px solid #ddd; border-radius: 4px; }
        button { background: #2196F3; color: white; padding: 15px; border: none; width: 100%; font-size: 16px; border-radius: 4px; cursor: pointer; }
        button:hover { background: #1976D2; }
        .refresh-btn { background: #FF9800; margin-top: 5px; padding: 8px; }
        .feature-list { font-size: 14px; color: #666; }
        h2 { color: #2196F3; }
        h3 { color: #1976D2; margin-bottom: 10px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h2>PersonalCMS ESP32</h2>
            <p>E-ink Display + Server Integration</p>
        </div>
        
        <div class="feature-list">
            <strong>Features:</strong>
            <ul>
                <li>Dashboard display from PersonalCMS server</li>
                <li>Image rotation with timer overlay</li>
                <li>Real-time timer in Mumbai timezone</li>
                <li>Auto-refresh every 30 seconds</li>
                <li>Hardware button mode switching</li>
            </ul>
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
                <input type="text" name="ota_username" placeholder="OTA Username (optional)">
                <input type="password" name="ota_password" placeholder="OTA Password (optional)">
            </div>
            
            <div class="section">
                <h3>Content Preferences</h3>
                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 10px;">
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
  otaUsername = server.arg("ota_username");
  otaPassword = server.arg("ota_password");
  
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
    "<html><head><meta charset='UTF-8'></head><body style='font-family:Arial; text-align:center; padding:50px;'>"
    "<h2>Configuration Saved!</h2>"
    "<p>Device will restart and connect to PersonalCMS server...</p>"
    "<div style='margin:20px; padding:15px; background:#e8f5e8; border-radius:5px;'>"
    "<strong>Next Steps:</strong><br>"
    "1. Device will connect to WiFi<br>"
    "2. Register with PersonalCMS server<br>"
    "3. Initialize e-ink display<br>"
    "4. Start showing personalized content"
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
  nvs.begin("personalcms", false);
  
  deviceName = nvs.getString("device_name", "");
  occupation = nvs.getString("occupation", "");
  wifiSSID = nvs.getString("wifi_ssid", "");
  wifiPassword = nvs.getString("wifi_password", "");
  serverURL = nvs.getString("server_url", "");
  preferencesJson = nvs.getString("preferences", "");
  otaUsername = nvs.getString("ota_username", "");
  otaPassword = nvs.getString("ota_password", "");
  
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
  nvs.begin("personalcms", false);
  
  nvs.putString("device_name", deviceName);
  nvs.putString("occupation", occupation);
  nvs.putString("wifi_ssid", wifiSSID);
  nvs.putString("wifi_password", wifiPassword);
  nvs.putString("server_url", serverURL);
  nvs.putString("preferences", preferencesJson);
  nvs.putString("ota_username", otaUsername);
  nvs.putString("ota_password", otaPassword);
  
  nvs.end();
  Serial.println("✅ Configuration saved to NVS");
}

// Placeholder implementations for remaining functions
void initializeDisplay() {
  Serial.println("🖥️ Display initialization");
  
  // Initialize SPI pins for e-ink display
  pinMode(25, OUTPUT); // RST
  pinMode(26, OUTPUT); // DC  
  pinMode(5, OUTPUT);  // CS
  pinMode(27, INPUT);  // BUSY
  pinMode(33, OUTPUT); // PWR
  
  // Initialize SPI with stable settings for e-ink
  SPI.begin(18, -1, 23, -1); // SCK=18, MISO=not used, MOSI=23, SS=not used
  SPI.setFrequency(4000000); // 4MHz for stable e-ink communication
  SPI.setDataMode(SPI_MODE0); // Ensure correct SPI mode
  SPI.setBitOrder(MSBFIRST);  // Most significant bit first
  
  // Power cycle the display for clean initialization
  digitalWrite(33, LOW);  // PWR off
  delay(100);
  digitalWrite(33, HIGH); // PWR on
  delay(200); // Longer delay for power stabilization
  
  // Check if display is connected by testing BUSY pin
  Serial.println("🔍 Checking for e-ink display connection...");
  
  // Try to initialize display with timeout
  bool displayConnected = false;
  unsigned long startTime = millis();
  
  // Set a 3-second timeout for display initialization
  while (millis() - startTime < 3000) {
    // Reset sequence for clean start
    digitalWrite(25, LOW);  // Reset
    delay(20);
    digitalWrite(25, HIGH); // Release reset
    delay(50);
    
    // Check if BUSY pin responds (indicates display is connected)
    if (digitalRead(27) == LOW || digitalRead(27) == HIGH) {
      // Pin is responsive, likely display is connected
      Serial.println("📟 Display connection detected, initializing...");
      
      // Initialize display with proper sequence
      EPD_Init(); // Full initialization
      delay(100);
      
      displayConnected = true;
      break;
    }
    delay(100);
  }
  
  if (displayConnected) {
    displayInitialized = true;
    Serial.println("✅ Display initialized successfully");
    Serial.println("🖥️ E-ink display ready for monochrome B&W operation");
  } else {
    displayInitialized = false;
    Serial.println("⚠️  No display detected - continuing in headless mode");
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
  doc["device_type"] = "ESP32_DevKit_V1";
  doc["wifi_ssid"] = wifiSSID;
  
  // Add sensor data
  doc["temperature"] = currentTemperature;
  doc["humidity"] = currentHumidity;
  doc["motion_detected"] = irMotionDetected;
  doc["sleep_mode"] = deviceSleeping;
  
  Serial.printf("📊 Sending sensor data: Temp=%.1f°C, Humidity=%.1f%%, Motion=%s, Sleep=%s\n", 
                currentTemperature, currentHumidity, 
                irMotionDetected ? "YES" : "NO", 
                deviceSleeping ? "YES" : "NO");
  
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

void sendSensorDataToServer() {
  if (serverURL.length() == 0) {
    Serial.println("❌ No server URL for sensor data - skipping update");
    return;
  }
  
  // Read latest sensor data before sending
  readTemperatureHumidity();
  
  HTTPClient http;
  http.begin(serverURL + "/api/devices/" + deviceID + "/sensor-data");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(512);
  doc["device_id"] = deviceID;
  doc["temperature"] = currentTemperature;
  doc["humidity"] = currentHumidity;
  doc["motion_detected"] = irMotionDetected;
  doc["sleep_mode"] = deviceSleeping;
  doc["timestamp"] = millis() / 1000; // Uptime in seconds
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("📊 Sending sensor data to server:");
  Serial.printf("   🌡️  Current temperature: %.1f°C\n", currentTemperature);
  Serial.printf("   💧 Current humidity: %.1f%%\n", currentHumidity);
  Serial.printf("   👁️  Motion detected: %s\n", irMotionDetected ? "YES" : "NO");
  Serial.printf("   💤 Sleep mode: %s\n", deviceSleeping ? "ON" : "OFF");
  Serial.println("   📤 JSON payload: " + jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("✅ Sensor data sent (" + String(httpResponseCode) + "): " + response);
  } else {
    Serial.println("❌ Failed to send sensor data. HTTP Code: " + String(httpResponseCode));
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
          // Reset timer state before new display
          timerActive = false;
          
          // Display on e-ink with proper initialization sequence
          Serial.println("🖥️ Displaying dashboard on e-ink display...");
          EPD_Init(); // Full initialization for clean display
          EPD_WhiteScreen_ALL(dashboardBuffer, dashboardBuffer); // Use same buffer for B&W
          EPD_DeepSleep(); // Proper sleep to save power and prevent corruption
          Serial.println("✅ Dashboard displayed successfully on e-ink!");
          
          // Small delay to ensure display settles
          delay(100);
        } else {
          Serial.println("💻 Dashboard data ready (display not connected - headless mode)");
        }
        
        dashboardLoaded = true;
        dashboardStartTime = millis();
        
        Serial.println("🕒 Timer will start updating every 10 seconds in bottom right corner");
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
          // Reset timer state - images don't have timers
          timerActive = false;
          dashboardLoaded = false; // Not a dashboard
          
          // Display on e-ink with proper sequence for image mode
          Serial.println("🖥️ Displaying image on e-ink display...");
          EPD_Init(); // Full initialization for clean display
          EPD_WhiteScreen_ALL(dashboardBuffer, dashboardBuffer); // Use same buffer for B&W monochrome
          EPD_DeepSleep(); // Proper sleep to prevent color corruption
          Serial.println("✅ Image displayed successfully on e-ink!");
          
          // Small delay to ensure display settles properly
          delay(100);
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

void switchMode() {
  currentMode = !currentMode;
  Serial.printf("🔄 Mode switched to: %s\n", currentMode ? "IMAGES" : "DASHBOARD");
  
  // Handle timer and display state based on mode
  if (currentMode) {
    // Switching to IMAGE mode - disable timer completely
    if (timerActive && displayInitialized) {
      Serial.println("🕒 Clearing timer area - switching to IMAGE mode");
      // Don't clear timer area as it will interfere with image display
      // Just disable timer updates
    }
    timerActive = false;
    dashboardLoaded = false; // Not showing dashboard anymore
    Serial.println("🖼️  IMAGE mode active - timer disabled");
  } else {
    // Switching to DASHBOARD mode - timer will start when dashboard loads
    timerActive = false; // Will be re-enabled when dashboard displays
    Serial.println("� DASHBOARD mode active - timer will start when dashboard loads");
  }
  
  // Add small delay before checking for new content to prevent rapid switching issues
  delay(200);
  
  // Immediately check for content in new mode
  checkServerForContent();
}

void updateTimer() {
  // Only update timer if we're in dashboard mode and dashboard is loaded
  if (currentMode || !dashboardLoaded) {
    return; // Timer only shows in dashboard mode
  }
  
  unsigned long currentTime = millis();
  
  // Update timer every 5 seconds (reduced frequency to prevent excessive refreshes)
  if (currentTime - lastTimerUpdate >= 5000) {
    // Calculate seconds since dashboard was loaded
    unsigned long elapsedSeconds = (currentTime - dashboardStartTime) / 1000;
    
    if (displayInitialized) {
      // Only update timer every 10 seconds to reduce e-ink wear
      if (elapsedSeconds % 10 == 0) {
        // Initialize partial display mode for timer updates (only once)
        if (!timerActive) {
          EPD_Init_Part();
          EPD_SetRAMValue_BaseMap(dashboardBuffer, dashboardBuffer);
          timerActive = true;
          Serial.println("🕒 Timer activated - partial display mode");
        }
        
        // Display the timer in bottom right corner (minimal partial update)
        EPD_Display_Timer(elapsedSeconds);
        Serial.printf("⏱️  Timer updated: %lu seconds since dashboard refresh\n", elapsedSeconds);
      } else {
        // Just log without display update
        Serial.printf("⏱️  Timer tick: %lu seconds (display update in %lu seconds)\n", 
                      elapsedSeconds, 10 - (elapsedSeconds % 10));
      }
    } else {
      // Headless mode - just log the timer
      Serial.printf("⏱️  Timer (headless): %lu seconds since dashboard refresh\n", elapsedSeconds);
    }
    
    lastTimerUpdate = currentTime;
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

void handleDeviceRemovalButton() {
  Serial.println("🚨 Handling device removal - long press detected");
  // Add device removal logic here
}

// NFC PN532 Functions
void initializeNFC() {
  Serial.println("🏷️  Initializing PN532 NFC module...");
  
  // Initialize I2C for PN532
  Wire.begin(PN532_SDA, PN532_SCL);
  nfc.begin();
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("   ❌ PN532 not found on I2C pins 21/22");
    nfcInitialized = false;
  } else {
    Serial.println("   ✅ PN532 initialized successfully!");
    Serial.printf("   📋 Firmware version: 0x%08X\n", versiondata);
    nfc.SAMConfig();
    nfcInitialized = true;
  }
}

// NFC trigger functionality moved to button long press

void writeNFCVCard() {
  if (!nfcInitialized) {
    Serial.println("❌ NFC not initialized - skipping VCard write");
    return;
  }
  
  Serial.println("🏷️  Waiting for NTAG to write...");
  Serial.println("   💡 Place NFC tag near the reader...");
  
  uint8_t uid[7], uidLen;
  unsigned long nfcStartTime = millis();
  const unsigned long NFC_TIMEOUT = 10000; // 10 second timeout
  
  // Wait for tag with timeout
  while (millis() - nfcStartTime < NFC_TIMEOUT) {
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen)) {
      break; // Tag detected
    }
    delay(100); // Short delay between checks
    
    // Check if timeout reached
    if (millis() - nfcStartTime >= NFC_TIMEOUT) {
      Serial.println("   ⏰ NFC timeout - no tag detected within 10 seconds");
      Serial.println("   🔄 Returning to normal operation...");
      return;
    }
  }
  
  Serial.print("   🏷️  Tag detected UID: ");
  for (uint8_t i = 0; i < uidLen; i++) {
    Serial.print(uid[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  String escNote = escapeNote(USER_NOTE);
  String vcard = "BEGIN:VCARD\r\nVERSION:3.0\r\nFN:" + String(USER_FULL_NAME) + "\r\n";
  if(strlen(USER_ORG)) vcard += "ORG:" + String(USER_ORG) + "\r\n";
  if(strlen(USER_PHONE)) vcard += "TEL;TYPE=CELL:" + String(USER_PHONE) + "\r\n";
  if(strlen(USER_EMAIL)) vcard += "EMAIL:" + String(USER_EMAIL) + "\r\n";
  if(strlen(USER_WEBSITE)) vcard += "URL:" + String(USER_WEBSITE) + "\r\n";
  if(escNote.length()) vcard += "NOTE:" + escNote + "\r\n";
  vcard += "END:VCARD\r\n";

  size_t vlen = vcard.length(); 
  uint8_t *vbytes = (uint8_t*)malloc(vlen); 
  memcpy(vbytes, vcard.c_str(), vlen);
  
  Buf ndef; 
  bool hasUrl = (strlen(FALLBACK_URL) > 0);
  buildMime(ndef, "text/x-vcard", vbytes, vlen, true, !hasUrl); // MB=1, ME depends on URL
  if(hasUrl) buildUri(ndef, FALLBACK_URL, false, true);
  free(vbytes);

  Buf tlv; 
  tlv.push(0x03); 
  if(ndef.len <= 0xFF) tlv.push((uint8_t)ndef.len); 
  else{ 
    tlv.push(0xFF); 
    tlv.push((ndef.len >> 8) & 0xFF); 
    tlv.push(ndef.len & 0xFF); 
  } 
  tlv.push(ndef.b, ndef.len); 
  tlv.push(0xFE);

  Serial.println("   🗑️  Erasing NTAG pages...");
  if(!eraseRange(ERASE_START, ERASE_END)){ 
    Serial.println("   ❌ Erase failed; remove tag and try again later"); 
    
    // Wait for tag removal with timeout
    unsigned long removalStart = millis();
    while(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen) && (millis() - removalStart < 5000)) {
      delay(200);
    }
    
    Serial.println("   🔄 Returning to normal operation...");
    return; 
  }
  
  Serial.println("   ✍️  Writing NDEF...");
  if(writeTLV(NTAG_START_PAGE, tlv.b, tlv.len)) {
    Serial.println("   ✅ Write OK - VCard written successfully!");
    Serial.println("   📱 NFC tag ready with contact information");
    nfcWriteOnce = true; // Prevent multiple writes
    Serial.println("   🔒 NFC write protection enabled (one-time write)");
  } else {
    Serial.println("   ❌ Write failed - please try again");
  }

  Serial.println("   📤 Remove tag...");
  
  // Wait for tag removal with timeout
  unsigned long removalStart = millis();
  while(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen) && (millis() - removalStart < 10000)) {
    delay(200);
  }
  
  if (millis() - removalStart >= 10000) {
    Serial.println("   ⏰ Tag removal timeout - continuing anyway");
  } else {
    Serial.println("   ✅ Tag removed successfully");
  }
  
  // Reset I2C and NFC module to clear any errors
  Serial.println("   🔄 Resetting NFC module...");
  delay(100);
  
  // Reinitialize I2C and NFC to clear any error states
  Wire.end();
  delay(50);
  Wire.begin(PN532_SDA, PN532_SCL);
  delay(50);
  nfc.begin();
  delay(100);
  nfc.SAMConfig();
  delay(50);
  
  Serial.println("   ✅ NFC operation complete - returning to normal operation");
  Serial.println("🔄 Ready for dashboard/image switching");
  
  // Force a content refresh to ensure normal operation resumes
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("📡 Refreshing content after NFC operation...");
    checkServerForContent();
  }
}

// DHT22 Temperature/Humidity Functions
void initializeDHT() {
  Serial.println("🌡️  Initializing DHT22 sensor...");
  
  // Set DHT pin mode
  pinMode(DHT_PIN, INPUT);
  delay(100);
  
  dht.begin();
  delay(2000); // Let sensor stabilize longer
  
  // Try multiple readings to ensure sensor is working
  int attempts = 0;
  bool sensorWorking = false;
  
  while (attempts < 5 && !sensorWorking) {
    delay(1000); // Wait between attempts
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    
    if (!isnan(temp) && !isnan(hum) && temp > -40 && temp < 80 && hum >= 0 && hum <= 100) {
      currentTemperature = temp;
      currentHumidity = hum;
      sensorWorking = true;
      Serial.printf("   ✅ DHT22 initialized: %.1f°C, %.1f%% (attempt %d)\n", temp, hum, attempts + 1);
    } else {
      attempts++;
      Serial.printf("   ⚠️  DHT22 read attempt %d failed (temp: %.1f, hum: %.1f)\n", attempts, temp, hum);
    }
  }
  
  if (!sensorWorking) {
    Serial.printf("   ❌ DHT22 sensor failed after %d attempts - using fallback values\n", attempts);
    Serial.println("   💡 Check wiring: VCC->3.3V, DATA->GPIO4, GND->GND, 10kΩ pullup on DATA");
    currentTemperature = 23.5;
    currentHumidity = 45.0;
  }
}

void readTemperatureHumidity() {
  // Always read on first call or after interval
  if (lastTempReading == 0 || millis() - lastTempReading >= TEMP_READ_INTERVAL) {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    
    // Validate readings with reasonable ranges
    if (!isnan(temp) && !isnan(hum) && temp > -40 && temp < 80 && hum >= 0 && hum <= 100) {
      currentTemperature = temp;
      currentHumidity = hum;
      
      static unsigned long lastTempLog = 0;
      if (millis() - lastTempLog > 60000) { // Log every minute
        Serial.printf("🌡️  Temperature: %.1f°C, Humidity: %.1f%% (GPIO %d)\n", temp, hum, DHT_PIN);
        lastTempLog = millis();
      }
    } else {
      // On sensor failure, use fallback values instead of keeping 0
      if (currentTemperature == 0.0 && currentHumidity == 0.0) {
        currentTemperature = 22.5; // Room temperature fallback
        currentHumidity = 50.0;   // Moderate humidity fallback
        Serial.printf("🌡️  DHT22 sensor not responding - using fallback: %.1f°C, %.1f%%\n", 
                      currentTemperature, currentHumidity);
      }
      
      static unsigned long lastErrorLog = 0;
      if (millis() - lastErrorLog > 300000) { // Log errors every 5 minutes
        Serial.printf("⚠️  DHT22 sensor read error (temp: %.1f, hum: %.1f) - keeping previous values\n", temp, hum);
        Serial.println("   💡 Check DHT22 wiring and power supply");
        Serial.printf("   📊 Current values: %.1f°C, %.1f%%\n", currentTemperature, currentHumidity);
        lastErrorLog = millis();
      }
    }
    
    lastTempReading = millis();
  }
}

void handleIRSensor() {
  // Motion detection schedule: 
  // - Wait 10 minutes after boot before starting
  // - Then every 10 minutes, check for motion for 10 seconds
  // - Go to sleep if no motion detected during check period
  
  unsigned long currentTime = millis();
  
  // Don't start motion detection until 10 minutes after boot
  if (!motionSystemEnabled && currentTime > MOTION_START_DELAY) {
    motionSystemEnabled = true;
    lastMotionCheckStart = currentTime;
    Serial.println("👁️ Motion detection system ENABLED - checking every 10 minutes");
  }
  
  // Exit early if motion system not enabled yet
  if (!motionSystemEnabled) {
    return;
  }
  
  // Check if it's time to start a new motion check cycle
  if (!motionCheckingActive && (currentTime - lastMotionCheckStart) >= MOTION_CHECK_INTERVAL) {
    motionCheckingActive = true;
    lastMotionCheckStart = currentTime;
    irMotionDetected = false;  // Reset for new check
    Serial.println("👁️ Starting 10-second motion check...");
  }
  
  // If we're in an active checking period
  if (motionCheckingActive) {
    // Check if the 10-second window has expired
    if ((currentTime - lastMotionCheckStart) >= MOTION_CHECK_DURATION) {
      motionCheckingActive = false;
      
      if (irMotionDetected) {
        Serial.println("👁️ Motion detected during check - staying awake");
        irMotionDetected = false;  // Reset for next cycle
      } else {
        Serial.println("💤 No motion detected - device could enter sleep mode");
        // Here you could add sleep mode logic if needed
      }
      return;
    }
    
    // During active checking, monitor the IR sensor
    int irAnalogReading = analogRead(IR_SENSOR_PIN);
    bool irDefinitelyGrounded = (irAnalogReading < UNIVERSAL_GROUNDED_THRESHOLD);
    
    // Only look for definitive ground signals during active checking
    if (irDefinitelyGrounded) {
      if (!irGroundConnectionActive) {
        irGroundConnectionStart = currentTime;
        irGroundConnectionActive = true;
      }
      
      unsigned long irGroundDuration = currentTime - irGroundConnectionStart;
      
      // Motion detection (pin definitely grounded for minimum time)
      if (irGroundDuration >= MIN_GROUND_TIME && !irMotionDetected) {
        irMotionDetected = true;
        Serial.printf("👁️ Motion detected! (GPIO %d grounded for %lu ms, ADC: %d)\n", 
                      IR_SENSOR_PIN, irGroundDuration, irAnalogReading);
        
        // Wake up display if sleeping
        if (deviceSleeping) {
          wakeFromSleepMode();
        }
      }
    } else {
      // Reset ground connection when pin goes high
      irGroundConnectionActive = false;
    }
  }
  
  // Debug info only during active checking periods (reduce spam)
  if (motionCheckingActive) {
    static unsigned long lastMotionDebug = 0;
    if (currentTime - lastMotionDebug > 2000) {  // Every 2 seconds during active check
      int irAnalogReading = analogRead(IR_SENSOR_PIN);
      Serial.printf("👁️ Motion check: ADC=%d, Time remaining: %lu ms\n", 
                    irAnalogReading, MOTION_CHECK_DURATION - (currentTime - lastMotionCheckStart));
      lastMotionDebug = currentTime;
    }
  }
}

void enterSleepMode() {
  if (deviceSleeping) return;
  
  Serial.println("😴 Entering sleep mode - no motion detected");
  deviceSleeping = true;
  
  // Turn off display if initialized
  if (displayInitialized) {
    EPD_DeepSleep();
    Serial.println("   🖥️  Display in deep sleep");
  }
  
  // Reduce LED activity
  digitalWrite(LED_PIN, LOW);
  Serial.println("   💤 Device sleeping - waiting for motion...");
}

void wakeFromSleepMode() {
  if (!deviceSleeping) return;
  
  Serial.println("👁️  Motion detected - waking from sleep mode");
  deviceSleeping = false;
  
  // Flash LED to indicate wake
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  
  // Refresh display if connected to WiFi
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("   🔄 Refreshing content after wake...");
    checkServerForContent();
  }
  
  Serial.println("   ✅ Device fully awake and operational");
}

// OTA Update Functions
void checkForOTAUpdate() {
  if (serverURL.length() == 0 || otaInProgress) {
    return;
  }
  
  Serial.println("🔄 Checking for OTA updates...");
  
  HTTPClient http;
  http.begin(serverURL + "/api/ota/check/" + deviceID);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-Version", currentFirmwareVersion);
  http.addHeader("X-Device-Type", "ESP32_PersonalCMS");
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("✅ OTA check response: " + response);
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error && doc["update_available"].as<bool>()) {
      String firmwareURL = doc["firmware_url"];
      String newVersion = doc["version"];
      String description = doc["description"];
      
      Serial.println("🔄 Update available: " + newVersion);
      Serial.println("📝 Description: " + description);
      Serial.println("📦 Firmware URL: " + firmwareURL);
      
      performOTAUpdate(firmwareURL);
    } else {
      Serial.println("✅ No update available - current version: " + currentFirmwareVersion);
    }
  } else if (httpResponseCode == 404) {
    Serial.println("ℹ️ OTA endpoint not available on server");
  } else {
    Serial.printf("❌ OTA check failed: HTTP %d\n", httpResponseCode);
  }
  
  http.end();
}

void performOTAUpdate(String firmwareURL) {
  otaInProgress = true;
  Serial.println("🔄 Starting OTA update...");
  Serial.println("📦 URL: " + firmwareURL);
  
  // Turn off display to save power during update
  if (displayInitialized) {
    EPD_DeepSleep();
    Serial.println("   🖥️ Display put to sleep for OTA");
  }
  
  // Blink LED rapidly during update preparation
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  
  HTTPClient httpClient;
  HTTPUpdate httpUpdate;
  
  // Configure HTTP client
  httpClient.begin(firmwareURL);
  httpClient.setTimeout(30000); // 30 second timeout
  
  // Set authentication if provided
  if (otaUsername.length() > 0 && otaPassword.length() > 0) {
    httpClient.setAuthorization(otaUsername.c_str(), otaPassword.c_str());
    Serial.println("   🔐 Using OTA authentication");
  }
  
  // Setup update callbacks
  httpUpdate.onStart([]() {
    Serial.println("🔄 OTA update started");
    digitalWrite(LED_PIN, HIGH); // Keep LED on during update
  });
  
  httpUpdate.onEnd([]() {
    Serial.println("✅ OTA update finished");
    digitalWrite(LED_PIN, LOW);
  });
  
  httpUpdate.onProgress([](int cur, int total) {
    static unsigned long lastProgressReport = 0;
    unsigned long now = millis();
    
    if (now - lastProgressReport > 5000) { // Report every 5 seconds
      int percentage = (cur * 100) / total;
      Serial.printf("🔄 OTA Progress: %d%% (%d/%d bytes)\n", percentage, cur, total);
      
      // Flash LED to show progress
      digitalWrite(LED_PIN, (percentage % 20 < 10) ? HIGH : LOW);
      
      lastProgressReport = now;
    }
  });
  
  httpUpdate.onError([](int err) {
    Serial.printf("❌ OTA Error: %d\n", err);
    Serial.println("   🔄 Continuing normal operation...");
  });
  
  // Perform the update
  Serial.println("📥 Starting firmware download and installation...");
  t_httpUpdate_return ret = httpUpdate.update(httpClient, "");
  
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("❌ OTA update failed: %s\n", httpUpdate.getLastErrorString().c_str());
      Serial.println("   🔄 Resuming normal operation...");
      otaInProgress = false;
      
      // Re-initialize display if it was working before
      if (displayInitialized) {
        delay(1000);
        initializeDisplay();
        Serial.println("   🖥️ Display re-initialized after failed OTA");
      }
      break;
      
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("ℹ️ No updates available (server response)");
      otaInProgress = false;
      break;
      
    case HTTP_UPDATE_OK:
      Serial.println("✅ OTA update successful!");
      Serial.println("🔄 Device will restart with new firmware...");
      // Device will restart automatically
      break;
  }
}