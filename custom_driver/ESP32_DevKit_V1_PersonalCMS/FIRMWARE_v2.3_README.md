# ESP32 Firmware v2.3 - Custom Driver Package

## Overview
This package contains the latest v2.3 firmware binaries for ESP32 DevKit V1 devices with unified NVS namespace support and cross-firmware compatibility.

## Firmware Variants

### 1. PersonalCMS Firmware v2.3
**Location:** `ESP32_DevKit_V1_PersonalCMS/bin/`
- **Files:** 
  - `ESP32_DevKit_V1_PersonalCMS_v2.3.bin` (Application binary)
  - `ESP32_DevKit_V1_PersonalCMS_v2.3.merged.bin` (Complete firmware with bootloader)
  - `ESP32_DevKit_V1_PersonalCMS_v2.3.json` (Metadata)

**Features:**
- Full PersonalCMS server integration
- E-ink display support (2.9" Waveshare)
- Sensor data collection (DHT22, BMP280)
- OTA firmware updates
- WiFi credential persistence
- Cross-firmware compatibility

### 2. LED Blink Firmware v2.3
**Location:** `ESP32_DevKit_V1_LED_Blink/bin/`
- **Files:**
  - `ESP32_DevKit_V1_LED_Blink_v2.3.bin` (Application binary)
  - `ESP32_DevKit_V1_LED_Blink_v2.3.merged.bin` (Complete firmware with bootloader)
  - `ESP32_DevKit_V1_LED_Blink_v2.3.json` (Metadata)

**Features:**
- LED pattern demonstrations
- OTA firmware updates
- WiFi credential persistence
- Development testing support
- Cross-firmware compatibility

### 3. OTA Base Firmware v2.3
**Location:** `ESP32_DevKit_V1_OTA_Base/bin/`
- **Files:**
  - `ESP32_DevKit_V1_OTA_Base_v2.3.bin` (Application binary)
  - `ESP32_DevKit_V1_OTA_Base_v2.3.merged.bin` (Complete firmware with bootloader)
  - `ESP32_DevKit_V1_OTA_Base_v2.3.json` (Metadata)

**Features:**
- Minimal system recovery mode
- OTA firmware updates
- WiFi credential persistence
- Emergency recovery capabilities
- Cross-firmware compatibility

## Key Improvements in v2.3

### Unified NVS Namespace
- All firmware variants use the same NVS namespace: `"esp32ota"`
- WiFi credentials persist across firmware types
- No need to re-enter WiFi settings when switching firmware

### Cross-Firmware Compatibility
- Update from any firmware type to any other type
- LED Blink ↔ PersonalCMS ↔ OTA Base seamless switching
- Automatic device type detection via HTTP headers

### Enhanced OTA System
- Automatic version management with timestamp-based versioning
- Smart duplicate prevention
- HTTP 404 error fixes with proper device type headers
- Background update checks every 2 minutes

### Device Type Headers
- Fixed missing `X-Device-Type` headers in all firmware
- Proper server-side device mapping
- Eliminated HTTP 404 OTA check errors

## Hardware Compatibility
- **Target Device:** ESP32 DevKit V1
- **Display:** 2.9" Waveshare E-ink (for PersonalCMS)
- **Sensors:** DHT22, BMP280 (for PersonalCMS)
- **LED:** Built-in GPIO 2 (for LED Blink demo)

## Installation Instructions

### Via OTA (Recommended)
1. Upload desired firmware via web interface
2. Device will automatically detect and install update
3. WiFi credentials persist across updates

### Via USB (Initial Setup)
1. Use ESP32 flash tool or Arduino IDE
2. Flash the `.merged.bin` file for complete setup
3. Or flash `.bin` file if bootloader already present

## Version History
- **v2.3:** Unified NVS namespace, cross-firmware compatibility, automatic versioning
- **v2.2:** HTTP 404 fixes, device type headers, WiFi credential persistence
- **v2.1:** Initial OTA implementation
- **v2.0:** Base firmware variants

## Compilation Date
October 3, 2025

## Support
- All firmware variants support OTA updates
- Unified configuration management
- Cross-firmware update capability
- WiFi credential preservation