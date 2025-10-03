# ESP32 DevKit V1 Firmware Collection - v2.1 🚀

## ✅ **UNIFIED NVS NAMESPACE SOLUTION**
All firmware now uses the **unified "esp32ota" namespace** for seamless WiFi credential persistence across OTA updates between different firmware types.

## 📦 **Available Firmware**

### 1. **OTA Base Firmware** (`ESP32_DevKit_V1_OTA_Base/`)
- **Source:** `ESP32_DevKit_V1_OTA_Base.ino`
- **Binary:** `bin/ESP32_DevKit_V1_OTA_Base_v2.1.bin` (1.13MB)
- **Full Image:** `bin/ESP32_DevKit_V1_OTA_Base_v2.1.merged.bin` (4MB)
- **Description:** Base OTA firmware with PersonalCMS integration
- **Features:** WiFi setup, OTA updates, device registration, heartbeat

### 2. **LED Blink Firmware** (`ESP32_DevKit_V1_LED_Blink/`)
- **Source:** `ESP32_DevKit_V1_LED_Blink.ino`
- **Binary:** `bin/ESP32_DevKit_V1_LED_Blink_v2.1.bin` (1.14MB)
- **Full Image:** `bin/ESP32_DevKit_V1_LED_Blink_v2.1.merged.bin` (4MB)
- **Description:** LED pattern firmware with OTA capabilities
- **Features:** Multiple LED patterns, WiFi setup, OTA updates

### 3. **PersonalCMS Full Firmware** (`ESP32_DevKit_V1_PersonalCMS/`)
- **Source:** `ESP32_DevKit_V1_PersonalCMS.ino`
- **Binary:** `bin/ESP32_DevKit_V1_PersonalCMS_v2.1.bin` (1.20MB)
- **Full Image:** `bin/ESP32_DevKit_V1_PersonalCMS_v2.1.merged.bin` (4MB)
- **Description:** Complete PersonalCMS with display and sensors
- **Features:** E-Paper display, DHT22, BMP280, PN532, full CMS

## 🔧 **NVS Namespace Unification**

### **Problem Solved:**
- ✅ WiFi credentials now persist across ALL firmware types
- ✅ No need to re-enter WiFi settings after OTA updates
- ✅ Seamless switching between Base ↔ LED ↔ PersonalCMS firmware

### **Technical Details:**
```cpp
// All firmware now uses:
nvs.begin("esp32ota", false);  // Unified namespace

// Shared persistent data:
- wifiSSID
- wifiPassword  
- serverURL
- otaUsername
- otaPassword
- deviceName
```

## 📊 **Compilation Details**
- **Date:** 2025-01-21
- **Arduino CLI:** v1.2.0
- **ESP32 Core:** 3.3.1
- **Board:** esp32:esp32:esp32doit-devkit-v1
- **Status:** FRESHLY_COMPILED_SUCCESS

## 🔄 **OTA Update Flow**
1. Flash any firmware (Base/LED/PersonalCMS) initially
2. Configure WiFi via captive portal (one time only)
3. Use OTA system to switch between any firmware types
4. **WiFi credentials automatically preserved!**

## 📁 **File Types**
- **`.ino`** - Arduino source code with unified NVS
- **`_v2.1.bin`** - Application binary for OTA updates  
- **`_v2.1.merged.bin`** - Complete firmware image for initial flashing
- **`_v2.1.json`** - Firmware metadata and configuration

## 🎯 **Quick Access**
Each subdirectory contains:
- Source code (`.ino` file)
- Binary files in `bin/` folder
- Metadata and documentation
- Ready for development or deployment

**No more WiFi re-entry hassles! Switch firmware types seamlessly! 🎉**