# ESP32 DevKit V1 LED Blink Firmware v2.0

## Overview
LED pattern demonstration firmware with OTA capabilities. Features multiple LED modes and web-based configuration.

## Features
- ✅ **8 LED Modes**: Multiple blinking patterns and effects
- ✅ **Web Interface**: Real-time LED control via browser
- ✅ **WiFi Portal**: Captive portal configuration
- ✅ **OTA Updates**: Automatic updates every 2 minutes
- ✅ **Pattern Engine**: Smooth LED transitions and effects
- ✅ **HTTPUpdate Fix**: Compatible with ESP32 Core 3.x

## LED Modes Available
1. **OFF** - LED disabled
2. **SOLID** - Steady on
3. **SLOW_BLINK** - 1 second intervals  
4. **FAST_BLINK** - 200ms intervals
5. **PULSE** - Smooth fading effect
6. **PATTERN1** - Custom sequence 1
7. **PATTERN2** - Custom sequence 2
8. **RAINBOW** - Color cycling (if RGB LED)

## Hardware Requirements
- ESP32 DevKit V1 board
- Built-in LED (GPIO 2) or external LED
- 2MB Flash memory minimum

## Installation
1. Flash this firmware to ESP32 via USB (first time only)
2. Connect to WiFi captive portal: `ESP32-LED-Setup-XXXXXX`
3. Configure server URL: `http://192.168.1.107:5000`
4. Access web interface for LED control

## Configuration
- **Device ID**: `ESP32_LED_BLINK_001`
- **Captive Portal**: `ESP32-LED-Setup-XXXXXX`
- **LED Pin**: GPIO 2 (configurable)
- **OTA Interval**: 2 minutes
- **Web Port**: 80

## Web Interface
Access the device's IP address in a browser for:
- **LED Mode Selection**: Choose from 8 different patterns
- **Pattern Configuration**: Customize timing and effects
- **WiFi Settings**: Network configuration
- **OTA Settings**: Update server configuration
- **Status Monitoring**: Device health and connectivity

## Version 2.0 Changes
- 🔧 **FIXED**: HTTPUpdate API compatibility with ESP32 Core 3.x
- 🔧 **FIXED**: Enhanced LED pattern engine
- ✨ **NEW**: Improved web interface responsiveness
- ✨ **NEW**: Better pattern transition smoothing
- ✨ **NEW**: Enhanced device registration
- ✨ **NEW**: Real-time pattern switching

## API Endpoints
- `POST /api/devices/register` - Device registration
- `POST /api/devices/{device_id}/heartbeat` - Status reporting
- `GET /api/ota/check/{device_id}` - Update checking

## File Details
- **Binary**: `ESP32_DevKit_V1_LED_Blink_v2.0.bin` (700KB)
- **Compiled**: October 1, 2025
- **ESP32 Core**: 3.3.1
- **Arduino**: 2.3.2

## Usage Examples
- **Demo Purposes**: Showcase ESP32 capabilities
- **Visual Feedback**: Status indication for other systems
- **Testing Platform**: Verify OTA and web functionality
- **Educational**: Learn ESP32 programming concepts

## Pattern Customization
The firmware supports custom LED patterns through the web interface. You can:
- Adjust blink intervals
- Set fade speeds for pulse mode
- Configure custom sequences
- Save preferred patterns to NVS

Perfect for demonstrations, testing, and learning ESP32 development!