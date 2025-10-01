# ESP32 OTA Update System for PersonalCMS

This document describes the Over-The-Air (OTA) update system integrated into the PersonalCMS project for ESP32 DevKit V1 devices.

## 📋 Overview

The OTA system allows you to remotely update ESP32 firmware without physical access to the device. It includes:

- **WiFi Captive Portal** for initial setup and credential saving
- **Automatic update checks** every 2 minutes
- **Persistent configuration** storage using NVS
- **Three firmware variants** for different use cases
- **Web-based firmware management** through PersonalCMS server

## 🔧 Firmware Variants

### 1. ESP32 PersonalCMS (Full Version)
- **File**: `ESP32_DevKit_V1_PersonalCMS.ino`
- **Features**: Complete PersonalCMS with e-ink display, sensors, NFC, and OTA
- **Target**: Production devices with full functionality
- **Memory**: ~1.2MB firmware size

### 2. ESP32 OTA Base (Bare-bones)
- **File**: `ESP32_DevKit_V1_OTA_Base.ino`
- **Features**: Basic WiFi, OTA updates, LED status only
- **Target**: Minimal deployments or recovery firmware
- **Memory**: ~800KB firmware size

### 3. ESP32 LED Blink (LED Patterns)
- **File**: `ESP32_DevKit_V1_LED_Blink.ino`
- **Features**: Configurable LED patterns with OTA updates
- **Target**: Testing, demonstration, or LED-based projects
- **Memory**: ~850KB firmware size

## 🚀 Quick Start

### Step 1: Build Firmware
```bash
# On Windows
build_firmware.bat

# On Linux/Mac (manual)
arduino-cli compile --fqbn esp32:esp32:esp32 ESP32_DevKit_V1_PersonalCMS.ino
```

### Step 2: Start PersonalCMS Server
```bash
python unified_cms.py
```

### Step 3: Upload Firmware via Web Interface
1. Go to http://localhost:5000/ota-management
2. Click "Upload Firmware"
3. Select your .bin file and configure version
4. Upload and assign to devices

### Step 4: Flash Initial Firmware to ESP32
```bash
# First-time flash via USB
esptool.py --port COM3 write_flash 0x10000 ESP32_PersonalCMS_2.0.0.bin
```

### Step 5: Configure Device
1. Device creates WiFi hotspot: `ESP32-OTA-Setup` (password: `12345678`)
2. Connect and configure WiFi + OTA server URL
3. Device connects and checks for updates automatically

## 📡 OTA Update Process

### Device Side (ESP32)
1. **Startup**: Load saved WiFi credentials from NVS
2. **Connect**: Establish WiFi connection or start captive portal
3. **Check**: Query server for updates every 2 minutes
4. **Download**: If update available, download and install
5. **Restart**: Automatically restart with new firmware

### Server Side (PersonalCMS)
1. **Receive**: Accept firmware uploads via web interface
2. **Store**: Save .bin files with version metadata
3. **Serve**: Provide update information via REST API
4. **Manage**: Track devices and firmware assignments

## 🔗 API Endpoints

### Device API (used by ESP32)
- `GET /api/ota/check/{device_id}` - Check for updates
- `GET /api/ota/download/{firmware_key}` - Download firmware
- `POST /api/devices/{device_id}/heartbeat` - Send status

### Management API (web interface)
- `POST /api/ota/upload` - Upload new firmware
- `GET /api/ota/versions` - List firmware versions
- `DELETE /api/ota/delete/{firmware_key}` - Delete firmware
- `GET /api/ota/stats` - Get update statistics

## 📱 Captive Portal Configuration

When the ESP32 can't connect to saved WiFi, it creates a setup portal:

### Network Details
- **SSID**: `ESP32-OTA-Setup` (or variant-specific name)
- **Password**: `12345678`
- **IP**: `192.168.4.1`

### Configuration Options
- **Device Name**: Friendly identifier
- **WiFi Network**: SSID and password
- **OTA Server**: PersonalCMS server URL
- **OTA Credentials**: Optional username/password for secure updates

## 💾 Persistent Storage

Device configuration is stored in ESP32 NVS (Non-Volatile Storage):

```cpp
// Stored configuration
String deviceName;        // Device identifier
String wifiSSID;         // WiFi network name
String wifiPassword;     // WiFi password
String serverURL;        // PersonalCMS server URL
String otaUsername;      // OTA authentication (optional)
String otaPassword;      // OTA authentication (optional)
```

## 🔒 Security Features

### Authentication (Optional)
- HTTP Basic Auth for firmware downloads
- Configurable per device or globally

### Validation
- File type validation (.bin files only)
- Firmware size limits
- SHA256 hash verification
- Version string validation

### Network Security
- HTTPS support for OTA downloads
- WPA2 WiFi encryption
- Captive portal timeout protection

## 📊 Monitoring & Status

### Device Status
- **Online/Offline**: Based on last heartbeat
- **Firmware Version**: Current running version
- **Uptime**: Time since last restart
- **Free Heap**: Available memory
- **WiFi Signal**: RSSI strength

### Update Statistics
- Total firmware versions
- Download counts per version
- Device type distribution
- Recent update activity

## 🛠️ Hardware Requirements

### ESP32 DevKit V1
- **Flash**: 4MB minimum (OTA requires ~2MB free)
- **RAM**: 320KB (512KB recommended)
- **WiFi**: 2.4GHz 802.11 b/g/n

### Pin Usage (PersonalCMS Full Version)
- **GPIO 2**: Built-in LED (status indicator)
- **GPIO 4**: DHT22 temperature sensor
- **GPIO 5**: E-ink display CS
- **GPIO 12**: Multi-function button
- **GPIO 18**: SPI clock
- **GPIO 21/22**: I2C (NFC module)
- **GPIO 23**: SPI MOSI
- **GPIO 25-27**: E-ink display control
- **GPIO 32**: IR motion sensor
- **GPIO 33**: E-ink display power

## 🚨 Troubleshooting

### Common Issues

**Device not connecting to WiFi**
- Check SSID/password in captive portal
- Verify 2.4GHz network (ESP32 doesn't support 5GHz)
- Check signal strength and interference

**OTA updates failing**
- Verify server URL is accessible from device
- Check firmware file size (must fit in available flash)
- Ensure stable WiFi connection during update

**Captive portal not appearing**
- Try connecting to `ESP32-OTA-Setup` manually
- Navigate to `192.168.4.1` if portal doesn't auto-open
- Clear device WiFi cache and retry

**Firmware upload errors**
- Verify .bin file format
- Check version string format (e.g., "2.1.0")
- Ensure sufficient server disk space

### Recovery Methods

**Serial Recovery**
```bash
# Flash recovery firmware via USB
esptool.py --port COM3 erase_flash
esptool.py --port COM3 write_flash 0x10000 ESP32_OTA_Base_1.0.0.bin
```

**Factory Reset**
```cpp
// In code or via serial monitor
#include <Preferences.h>
Preferences nvs;
nvs.begin("esp32ota", false);
nvs.clear();
nvs.end();
ESP.restart();
```

## 📁 File Structure

```
epd_server-main/
├── custom_driver/ESP32_DevKit_V1_PersonalCMS/
│   ├── ESP32_DevKit_V1_PersonalCMS.ino      # Full featured
│   ├── ESP32_DevKit_V1_OTA_Base.ino         # Bare-bones
│   ├── ESP32_DevKit_V1_LED_Blink.ino        # LED patterns
│   └── Display_EPD_W21_*.h                  # Display drivers
├── data/ota/
│   ├── firmware/                            # Uploaded .bin files
│   └── firmware_registry.json              # Version metadata
├── templates/
│   ├── ota_management.html                  # OTA dashboard
│   └── ota_upload.html                      # Upload interface
├── ota_manager.py                           # OTA backend logic
├── unified_cms.py                           # Main server
└── build_firmware.bat                       # Build script
```

## 🔄 Update Workflow

1. **Development**: Modify ESP32 code
2. **Build**: Compile to .bin using Arduino IDE or CLI
3. **Upload**: Use PersonalCMS web interface to upload firmware
4. **Assign**: Assign firmware to device types or specific devices
5. **Deploy**: Devices automatically check and update
6. **Monitor**: Track update progress and device status

## 🌐 Production Deployment

### Server Setup
```bash
# Install dependencies
pip install flask flask-sqlalchemy pillow requests

# Create systemd service (Linux)
sudo cp personalcms.service /etc/systemd/system/
sudo systemctl enable personalcms
sudo systemctl start personalcms
```

### Network Configuration
- Port forwarding: 5000/tcp for OTA server
- DNS: Point domain to server IP
- SSL: Use reverse proxy (nginx) for HTTPS

### Backup Strategy
- Regular database backups
- Firmware file backups
- Device configuration exports

## 📞 Support

For issues and questions:
1. Check troubleshooting section above
2. Review server logs: `tail -f personalcms.log`
3. Monitor device serial output for debugging
4. Check GitHub issues for known problems

## 🚀 Future Enhancements

- **Staged rollouts**: Gradual firmware deployment
- **Rollback capability**: Automatic revert on failure
- **Firmware signing**: Cryptographic verification
- **Delta updates**: Incremental patches
- **Scheduled updates**: Time-based deployment
- **Device groups**: Organized firmware management