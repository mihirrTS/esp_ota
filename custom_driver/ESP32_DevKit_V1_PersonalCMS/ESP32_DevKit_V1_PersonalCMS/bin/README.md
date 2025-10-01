# ESP32 DevKit V1 PersonalCMS Firmware v2.0

## Overview
Full-featured PersonalCMS firmware with complete e-ink display, sensor integration, and OTA capabilities.

## Features
- ✅ **E-ink Display**: 2.9" Waveshare e-Paper display support
- ✅ **Sensors**: BMP280 temperature/humidity + IR motion detection  
- ✅ **NFC**: PN532 NFC module for VCard writing
- ✅ **OTA Updates**: Automatic updates every 2 minutes
- ✅ **WiFi Portal**: Captive portal for easy configuration
- ✅ **Content API**: Dynamic content management
- ✅ **HTTPUpdate Fix**: Compatible with ESP32 Core 3.x

## Hardware Requirements
- ESP32 DevKit V1 board
- Waveshare 2.9inch e-Paper Module
- BMP280 sensor (temperature/humidity)
- IR motion sensor
- PN532 NFC module
- 4MB Flash memory minimum

## Installation
1. Flash this firmware to ESP32 via USB (first time only)
2. Connect to WiFi captive portal: `ESP32-PersonalCMS-XXXXXX`
3. Configure server URL: `http://192.168.1.107:5000`
4. Device will auto-register and receive OTA updates

## Configuration
- **Device ID**: `ESP32_DEV1_001` (or custom)
- **OTA Interval**: 2 minutes
- **Heartbeat**: 30 seconds
- **API Endpoints**: Full PersonalCMS API support

## Version 2.0 Changes
- Fixed HTTPUpdate API compatibility with ESP32 Core 3.x
- Enhanced device registration with occupation field
- Improved error handling and logging
- Better OTA reliability and recovery
- Updated sensor integration
- Enhanced content management

## File Details
- **Binary**: `ESP32_DevKit_V1_PersonalCMS_v2.0.bin` (1.2MB)
- **Compiled**: October 1, 2025
- **ESP32 Core**: 3.3.1
- **Arduino**: 2.3.2

## Usage
This firmware provides the complete PersonalCMS experience with all features enabled. Perfect for production deployments where full functionality is required.