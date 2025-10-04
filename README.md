# ESP32 OTA PersonalCMS - Complete Device & Content Management System

## 🎯 Overview
ESP32 OTA PersonalCMS is a comprehensive system combining **Over-The-Air (OTA) firmware updates** with **personalized content management** for ESP32 devices. It features e-ink display integration, sensor data collection, WiFi captive portal setup, and web-based device management.

## ✨ Key Features

### 🔄 OTA System
- **WiFi Captive Portal** for initial device setup and credential saving
- **Automatic firmware updates** every 2 minutes with cross-firmware compatibility
- **Three firmware variants**: PersonalCMS (full), OTA Base (minimal), LED Blink (patterns)
- **Persistent configuration** using NVS storage
- **Web-based firmware management** with automatic versioning

### 📱 PersonalCMS Features
- **Per-device content management** with personalized dashboards
- **E-ink display integration** (800x480) with BMP generation
- **Sensor data collection** (temperature, humidity, motion, NFC)
- **Content APIs** with external integrations (jokes, riddles, news)
- **Image processing** with Floyd-Steinberg dithering for e-ink optimization
- **Real-time device monitoring** and status tracking

### 🌐 Web Interface
- **OTA Management Dashboard** for firmware uploads and device assignments
- **Content Administration** with API endpoint configuration
- **Device Management** with real-time status and preferences
- **Image Upload & Processing** with automatic e-ink optimization
- **Mobile-responsive interface** for device configuration

## 🚀 Quick Start

### Prerequisites
- Python 3.8+
- ESP32 DevKit V1 with optional e-ink display and sensors
- Arduino CLI (optional, for firmware building)

### Installation & Startup
```powershell
# Windows - Use the convenient startup script
START_SERVER.bat

# Or manual installation:
python -m pip install -r requirements.txt
python START_PERSONALCMS_OTA.py
```

### Access Points
- **Main Dashboard**: http://localhost:5000/
- **OTA Management**: http://localhost:5000/ota-management
- **Content Admin**: http://localhost:5000/content-admin
- **Image Upload**: http://localhost:5000/upload

## 🏗️ Architecture

### Core Files
- **`unified_cms.py`** - Main Flask application with models and business logic
- **`models.py`** - Database models and shared logic components
- **`ota_manager.py`** - OTA update management and firmware handling
- **`START_PERSONALCMS_OTA.py`** - Main entry point with initialization

### Key API Endpoints

#### Device & Content API
- `POST /api/devices/register` – device registration and setup
- `GET /api/devices/{device_id}/content` – personalized content JSON
- `GET /api/devices/{device_id}/dashboard` – generated dashboard BMP
- `GET /api/devices/{device_id}/images-sequence` – image sequence for device
- `POST /api/devices/{device_id}/heartbeat` – device status ping
- `POST /api/devices/{device_id}/sensor-data` – sensor data upload

#### OTA Management API
- `GET /api/ota/check/{device_id}` – check for firmware updates
- `GET /api/ota/download/{firmware_key}` – download firmware binary
- `POST /api/ota/upload` – upload new firmware
- `DELETE /api/ota/delete/{firmware_key}` – remove firmware
- `GET /api/ota/versions` – list available firmware versions

## 🔧 ESP32 Firmware Variants

### 1. ESP32 PersonalCMS (Full Featured)
- **File**: `ESP32_DevKit_V1_PersonalCMS.ino`
- **Features**: Complete PersonalCMS with e-ink display, sensors, NFC, and OTA
- **Components**: Temperature/humidity sensor, motion sensor, e-ink display, NFC module
- **Memory**: ~1.13MB firmware size
- **Use Case**: Production devices with full functionality

### 2. ESP32 OTA Base (Bare-bones)
- **File**: `ESP32_DevKit_V1_OTA_Base.ino`
- **Features**: Basic WiFi, OTA updates, LED status, dashboard download capability
- **Components**: Built-in LED, WiFi, OTA system
- **Memory**: ~1.13MB firmware size
- **Use Case**: Minimal deployments or recovery firmware

### 3. ESP32 LED Blink (LED Patterns)
- **File**: `ESP32_DevKit_V1_LED_Blink.ino`
- **Features**: Configurable LED patterns with OTA updates
- **Components**: Built-in LED with pattern control
- **Memory**: ~1.13MB firmware size
- **Use Case**: Testing, demonstration, or LED-based projects

## 📡 Device Setup Process

### Step 1: Flash Initial Firmware
```powershell
# First-time flash via USB (choose any firmware variant)
esptool.py --port COM3 write_flash 0x1000 custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_OTA_Base\bin\ESP32_DevKit_V1_OTA_Base_v2.0.bin
```

### Step 2: Configure via Captive Portal
1. ESP32 creates WiFi hotspot: `ESP32-OTA-Setup` (password: `12345678`)
2. Connect to the hotspot and navigate to `192.168.4.1`
3. Configure:
   - **Device Name**: Friendly identifier
   - **WiFi Network**: SSID and password
   - **OTA Server**: PersonalCMS server URL (e.g., `http://192.168.1.100:5000`)
   - **Optional**: OTA authentication credentials

### Step 3: Automatic Operation
- Device connects to WiFi and registers with server
- Checks for firmware updates every 2 minutes
- Downloads and displays personalized content (if applicable)
- Sends sensor data and heartbeat signals

## 🔄 OTA Update Workflow

### Cross-Firmware Compatibility
All firmware variants can update to any other variant automatically:
- **PersonalCMS ↔ OTA Base ↔ LED Blink**
- WiFi credentials preserved across firmware types
- Automatic version detection and update delivery

### Update Process
1. **Upload**: Use web interface to upload new firmware (.bin files)
2. **Auto-versioning**: Server generates timestamp-based versions (YYYYMMDD.HHMMSS)
3. **Assignment**: Assign firmware to specific devices or device types
4. **Distribution**: Devices automatically download and install updates
5. **Verification**: Monitor update status through web dashboard

## � Configuration & Hardware

### ESP32 Hardware Requirements
- **Flash**: 4MB minimum (OTA requires ~2MB free space)
- **RAM**: 320KB (512KB recommended for full features)
- **WiFi**: 2.4GHz 802.11 b/g/n

### Pin Configuration (PersonalCMS Full Version)
- **GPIO 2**: Built-in LED (status indicator)
- **GPIO 4**: DHT22 temperature/humidity sensor
- **GPIO 5**: E-ink display CS (Chip Select)
- **GPIO 12**: Multi-function button
- **GPIO 18**: SPI clock
- **GPIO 21/22**: I2C (NFC module communication)
- **GPIO 23**: SPI MOSI
- **GPIO 25-27**: E-ink display control pins
- **GPIO 32**: IR motion sensor
- **GPIO 33**: E-ink display power control

### Environment Configuration
- `DEVICE_ACTIVE_TIMEOUT_SECONDS`: Device activity timeout (default: 300)
- `UPLOAD_FOLDER`: Firmware and image upload directory
- `SECRET_KEY`: Flask application secret key
- `MAX_CONTENT_LENGTH`: Maximum upload file size (16MB)

## � Project Structure
```
epd_server-main/
├── unified_cms.py                      # Main Flask application
├── models.py                           # Database models and shared logic
├── ota_manager.py                      # OTA update management
├── START_PERSONALCMS_OTA.py           # Main entry point and initialization
├── START_SERVER.bat                    # Windows startup script
├── requirements.txt                    # Python dependencies
├── init_db.py                         # Database initialization utility
│
├── custom_driver/                      # ESP32 firmware source code
│   └── ESP32_DevKit_V1_PersonalCMS/
│       ├── ESP32_DevKit_V1_PersonalCMS/    # Full featured firmware
│       ├── ESP32_DevKit_V1_OTA_Base/       # Minimal OTA firmware
│       └── ESP32_DevKit_V1_LED_Blink/      # LED pattern firmware
│
├── firmwares/                          # Compiled firmware binaries
├── data/                              # Server data storage
│   ├── uploads/                       # User uploaded files
│   ├── generated/                     # Generated content
│   ├── device_content/               # Per-device content
│   └── ota/                          # OTA firmware storage
│
├── templates/                         # Web interface templates
├── static/                           # CSS, JavaScript, assets
├── instance/                         # Database files
├── dashboards/                       # Generated dashboard images
└── docs/                            # Documentation files
```

## 🔒 Security Features

### OTA Security
- **HTTP Basic Authentication** for firmware downloads (optional)
- **File validation** (.bin extension, size limits)
- **SHA256 hash verification** for firmware integrity
- **Version string validation** and automatic generation

### Network Security
- **HTTPS support** for OTA downloads
- **WPA2 WiFi encryption** for device connections
- **Captive portal timeout protection**
- **Server access control** and authentication

## 📊 Monitoring & Status

### Device Monitoring
- **Real-time status**: Online/offline based on heartbeat
- **Firmware tracking**: Current version and update history
- **Hardware metrics**: Temperature, humidity, motion detection
- **Network status**: WiFi signal strength (RSSI)
- **System metrics**: Uptime, free heap memory

### System Analytics
- **Device type distribution** and firmware version stats
- **Update success/failure rates** and download metrics
- **Content delivery statistics** and API performance
- **Error tracking** and system health monitoring

## 🎉 System Features & Capabilities

### ✅ OTA Management
- Cross-firmware updates (PersonalCMS ↔ OTA Base ↔ LED Blink)
- Automatic timestamp-based versioning (YYYYMMDD.HHMMSS)
- WiFi credential persistence across firmware updates
- Web-based firmware upload and device assignment
- Real-time update monitoring and status tracking

### ✅ PersonalCMS Features
- Per-device personalized content dashboards
- E-ink display integration with BMP generation
- Floyd-Steinberg dithering for optimal e-ink display
- Content API integration (jokes, riddles, news)
- Image upload and automatic processing
- Sensor data collection and monitoring

### ✅ Device Management
- WiFi captive portal for easy setup
- Real-time device status and health monitoring
- Automatic device registration and heartbeat system
- Per-device preferences and configuration
- Remote device control and management

### ✅ Web Interface
- Responsive Bootstrap-based admin dashboard
- OTA firmware management interface
- Content administration and API configuration
- Image upload and processing tools
- Device monitoring and analytics

## 🚨 Troubleshooting

### Common Issues

**Device not connecting to WiFi**
- Verify SSID/password in captive portal (192.168.4.1)
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Check signal strength and network interference

**OTA updates failing**
- Verify server URL is accessible from device network
- Check firmware file size (must fit in available flash)
- Ensure stable WiFi connection during update process
- Monitor server logs for detailed error information

**Captive portal not appearing**
- Manually connect to `ESP32-OTA-Setup` WiFi network
- Navigate directly to `192.168.4.1` in browser
- Clear device WiFi cache and restart ESP32

**Firmware upload errors**
- Verify .bin file format and valid Arduino compilation
- Check version string format and automatic generation
- Ensure sufficient server disk space for firmware storage

### Recovery Methods

**Serial Recovery (USB)**
```powershell
# Erase flash and install recovery firmware
esptool.py --port COM3 erase_flash
esptool.py --port COM3 write_flash 0x1000 custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_OTA_Base\bin\ESP32_DevKit_V1_OTA_Base_v2.0.bin
```

**Factory Reset via Code**
```cpp
// Reset NVS storage and restart
#include <Preferences.h>
Preferences nvs;
nvs.begin("esp32ota", false);
nvs.clear();
nvs.end();
ESP.restart();
```

## 🌐 Production Deployment

### Server Setup
```powershell
# Install dependencies
pip install -r requirements.txt

# Create Windows service or use task scheduler
# Point to START_PERSONALCMS_OTA.py for automatic startup
```

### Network Configuration
- **Port forwarding**: 5000/tcp for OTA and PersonalCMS server
- **Static IP**: Assign static IP to server for consistent device configuration
- **DNS**: Optional domain setup for easier device configuration
- **SSL/HTTPS**: Use reverse proxy (nginx/Apache) for secure connections

## 💡 Usage Examples

### Upload and Deploy Firmware
1. Navigate to **OTA Management** (http://localhost:5000/ota-management)
2. Click "Upload Firmware" and select .bin file
3. Choose device type and add description
4. Optionally auto-assign to all devices of that type
5. Monitor device updates in real-time

### Configure New Device
1. Flash any firmware variant via USB initially
2. Connect to `ESP32-OTA-Setup` WiFi network (password: `12345678`)
3. Configure WiFi credentials and server URL at `192.168.4.1`
4. Device automatically registers and begins receiving updates

### Content Management
1. Access **Content Admin** for API configuration
2. Upload images via **Image Upload** with automatic processing
3. Monitor device content and sensor data in main dashboard
4. Customize per-device preferences and content delivery

## 🚀 Future Enhancements

- **Staged rollouts**: Gradual firmware deployment to device groups
- **Rollback capability**: Automatic revert on update failure detection
- **Firmware signing**: Cryptographic verification for enhanced security
- **Delta updates**: Incremental patches to reduce bandwidth usage
- **Scheduled updates**: Time-based deployment and maintenance windows
- **Device grouping**: Organized firmware and content management
- **Mobile app**: Native mobile application for device management

---

**ESP32 OTA PersonalCMS** - Complete IoT device and content management platform 🚀
