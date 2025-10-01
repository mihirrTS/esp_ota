# ESP32 OTA System - Troubleshooting & Setup Guide

## 🔍 Issues Identified

### 1. **Server URL Mismatch**
- **Problem**: ESP32 is trying to connect to `http://192.168.1.102:5000`
- **Server Running On**: `http://192.168.1.107:5000`
- **Solution**: Update ESP32 configuration with correct server URL

### 2. **Device Not Registered**
- **Problem**: ESP32 device `ESP32_OTA_BASE_001` not found in database
- **Error**: `HTTP 404` when checking for OTA updates
- **Solution**: Device needs to register first

### 3. **No Firmware Files**
- **Problem**: No OTA firmware files uploaded to server
- **Solution**: Upload firmware files via OTA management interface

## ✅ Quick Fix Steps

### Step 1: Configure ESP32 with Correct Server URL
1. **Connect to ESP32 Captive Portal**:
   - Look for WiFi network: `ESP32-Setup-XXXXXX`
   - Connect and open browser (should auto-redirect)

2. **Update Server Configuration**:
   - Server URL: `http://192.168.1.107:5000`
   - Keep other settings as configured

### Step 2: Upload Firmware Files
1. **Access OTA Management**: http://192.168.1.107:5000/ota-management
2. **Upload Firmware Files**: Use the existing firmware binaries from `/firmware_binaries/` folder
3. **Available Files**:
   - `ESP32_PersonalCMS_Full.bin` (1.2MB)
   - `ESP32_OTA_Base.bin` (800KB) 
   - `ESP32_LED_Blink.bin` (700KB)

### Step 3: Verify Registration
After correct server URL configuration:
1. ESP32 should automatically register when WiFi connects
2. Check dashboard: http://192.168.1.107:5000/
3. Device should appear in "Registered Devices" section

## 🔧 Technical Details

### Device Registration Process
```cpp
// ESP32 automatically calls on WiFi connect:
registerDevice() -> POST /api/devices/register
{
  "device_id": "ESP32_OTA_BASE_001",
  "device_name": "ESP32 OTA Base",
  "occupation": "OTA Update Base",
  "device_type": "ESP32_DevKit_V1",
  "wifi_ssid": "YourNetwork",
  "firmware_version": "1.0.0"
}
```

### OTA Check Process
```cpp
// ESP32 calls every 2 minutes:
checkForOTAUpdate() -> GET /api/ota/check/ESP32_OTA_BASE_001
Headers: X-Device-Version: 1.0.0, X-Device-Type: ESP32_OTA_Base

// Server response (if update available):
{
  "update_available": true,
  "firmware_url": "http://192.168.1.107:5000/api/ota/download/firmware_id",
  "version": "1.1.0",
  "size": 800000
}
```

### Heartbeat Process
```cpp
// ESP32 calls every 30 seconds:
sendHeartbeat() -> POST /api/devices/ESP32_OTA_BASE_001/heartbeat
{
  "device_id": "ESP32_OTA_BASE_001",
  "version": "1.0.0",
  "uptime": 1234,
  "free_heap": 200000,
  "wifi_rssi": -45
}
```

## 🚀 Expected Results After Fix

1. **Dashboard**: Should show 1 registered device
2. **ESP32 Serial**: Should show successful heartbeats and OTA checks
3. **OTA Management**: Should list uploaded firmware files
4. **Update Process**: Should work automatically when new firmware uploaded

## 📱 Current Configuration Links

- **Server Dashboard**: http://192.168.1.107:5000/
- **OTA Management**: http://192.168.1.107:5000/ota-management
- **ESP32 Config Portal**: Connect to ESP32 WiFi network

The ESP32 firmware already includes the device registration function we added. The main issue is just the IP address configuration mismatch.