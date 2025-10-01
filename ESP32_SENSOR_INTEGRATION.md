# ESP32 DevKit V1 PersonalCMS - Sensor Integration

## 🔧 **New Hardware Integration Summary**

Successfully integrated **PN532 NFC**, **DHT22 Temperature/Humidity**, and **IR Motion Sensor** into the ESP32 DevKit V1 PersonalCMS project.

## 📌 **Updated Pin Configuration**

### **Changed Pin Assignments:**
- **Mode Switch**: Moved from GPIO 4 → **GPIO 2** (shared with built-in LED)
- **DHT22 Sensor**: Now uses **GPIO 4** (previously switch pin)

### **New Pin Assignments:**
- **PN532 NFC (I2C)**: GPIO 21 (SDA), GPIO 22 (SCL)
- **NFC Trigger**: GPIO 15 (LOW = activate NFC write)
- **DHT22 Sensor**: GPIO 4 (temperature/humidity data)
- **IR Motion Sensor**: GPIO 32 (motion detection for sleep/wake)

### **Complete Pin Map:**
```
ESP32 DevKit V1 Production Pin Assignment:
├── Display (SPI): GPIO 5, 18, 23, 25, 26, 27, 33
├── NFC PN532 (I2C): GPIO 21, 22
├── Controls: GPIO 2 (Switch + LED), GPIO 15 (NFC Trigger)
├── Sensors: GPIO 4 (DHT22), GPIO 32 (IR Motion)
└── Available: 0, 1, 12, 13, 14, 16, 17, 19, 34, 35, 36, 39
```

## 🚀 **New Features Added**

### **1. PN532 NFC Module**
- **Functionality**: Writes VCard with device info to NTAG tags
- **Trigger**: GPIO 15 pulled LOW activates NFC write mode
- **Write-Once Protection**: Prevents multiple writes to same tag
- **VCard Content**: 
  - Device name and server URL
  - Current temperature and humidity readings
  - PersonalCMS device information

### **2. DHT22 Temperature/Humidity Sensor**
- **Readings**: Temperature (°C) and Humidity (%)
- **Update Interval**: Every 30 seconds
- **Status Display**: Included in system status reports
- **Integration**: Data included in NFC VCard and dashboard

### **3. IR Motion Sensor (Sleep/Wake)**
- **Sleep Mode**: Activates after 5 minutes of no motion
- **Deep Sleep**: Puts display and device into low-power mode
- **Wake Trigger**: Any motion detection
- **Auto-Refresh**: Refreshes content automatically on wake

## 🔄 **Enhanced Status Monitoring**

### **System Status Reports Now Include:**
- Temperature and humidity readings
- Motion sensor state (MOTION/CLEAR)
- Sleep mode status (ON/OFF)
- NFC module status (READY/OFFLINE)
- NFC write protection status (COMPLETED/AVAILABLE)

## 📱 **NFC VCard Integration**

### **VCard Content Example:**
```vcard
BEGIN:VCARD
VERSION:3.0
FN:Living Room Display
ORG:PersonalCMS Device
NOTE:# ESP32 Sensor Integration with E-ink Dashboard

## Overview
Complete integration of DHT22 temperature/humidity sensor data into the PersonalCMS e-ink dashboard display system.

## ESP32 Device Changes

### 1. Hardware Configuration
```cpp
// Pin Assignments (ESP32 DevKit V1)
const int SWITCH_PIN = 2;          // Mode switch (moved from GPIO 4)
const int DHT_PIN = 4;             // DHT22 sensor (moved to GPIO 4)
const int NFC_TRIGGER_PIN = 15;    // NFC activation trigger
const int IR_SENSOR_PIN = 32;      // IR motion sensor
const int PN532_SDA = 21;          // NFC I2C Data
const int PN532_SCL = 22;          // NFC I2C Clock
```

### 2. Data Transmission
The ESP32 now sends sensor data to the PersonalCMS server via two methods:

**Device Registration (every startup):**
```json
{
  "device_id": "ESP32_DEV1_002",
  "device_name": "Living Room Display",
  "temperature": 23.5,
  "humidity": 45.2,
  "motion_detected": true,
  "sleep_mode": false
}
```

**Periodic Sensor Updates (every 2 minutes):**
```json
{
  "device_id": "ESP32_DEV1_002",
  "temperature": 23.5,
  "humidity": 45.2,
  "motion_detected": false,
  "sleep_mode": false,
  "timestamp": 1234567890
}
```

### 3. Reading Schedule
- **Temperature/Humidity**: Every 30 seconds
- **Serial Logging**: Every 60 seconds
- **Server Updates**: Every 2 minutes
- **Status Reports**: Every 15 seconds

## PersonalCMS Server Changes

### 1. Database Schema Updates
Added new fields to the `Device` model:
```python
temperature = db.Column(db.Float, default=0.0)
humidity = db.Column(db.Float, default=0.0)
motion_detected = db.Column(db.Boolean, default=False)
sleep_mode = db.Column(db.Boolean, default=False)
sensor_last_update = db.Column(db.DateTime)
```

### 2. API Endpoints

**Enhanced Device Registration:**
- `POST /api/devices/register`
- Now accepts and stores sensor data

**New Sensor Data Endpoint:**
- `POST /api/devices/{device_id}/sensor-data`
- Dedicated endpoint for sensor updates
- Updates device status and timestamp

### 3. Dashboard Generation
The dashboard now includes a **SENSOR DATA** section with:
- Temperature reading (°C)
- Humidity percentage (%)
- Motion detection status
- Sleep mode indicator
- Last sensor update time

## E-ink Display Output

### Dashboard Layout:
```
PersonalCMS - Living Room Display
Device: ESP32_DEV1_002
Occupation: Home Monitoring

SENSOR DATA:
• Temperature: 23.5°C
• Humidity: 45.2%
• Status: Motion Detected
• Last Update: 14:30

Updated: 2025-09-30 14:30

JOKES:
• Dad Jokes
  Why don't scientists trust atoms?

RIDDLES:
• Logic Riddles
  What has keys but no locks?
```

## Features

### ✅ **Implemented:**
1. **Real-time Sensor Reading**: DHT22 temperature and humidity
2. **Server Integration**: Automatic data transmission to PersonalCMS
3. **Dashboard Display**: Temperature/humidity shown on e-ink screen
4. **Motion Detection**: IR sensor for sleep/wake functionality
5. **NFC VCard Writing**: Professional contact card on NTAG
6. **Error Handling**: Robust sensor failure detection
7. **Power Management**: Sleep mode with motion activation

### 🎯 **Technical Specifications:**
- **Temperature Range**: -40°C to 80°C (±0.5°C accuracy)
- **Humidity Range**: 0-100% RH (±2-5% accuracy)
- **Update Frequency**: 30-second sensor reads, 2-minute server sync
- **Display**: 800x480 monochrome e-ink with sensor data overlay
- **Communication**: I2C (NFC), GPIO (sensors), WiFi (server)

### 📊 **Data Flow:**
```
DHT22 → ESP32 → WiFi → PersonalCMS Server → BMP Generation → ESP32 → E-ink Display
                ↑                              ↓
        Every 2 min                    Every 30 sec
```

## Installation Notes

### Database Migration:
When updating the server, run:
```bash
python init_db.py  # Recreate database with new sensor fields
```

### ESP32 Libraries Required:
- `DHT sensor library` for temperature/humidity
- `Adafruit PN532` for NFC functionality  
- `ArduinoJson` for data serialization
- Standard ESP32 libraries (WiFi, HTTPClient, Wire)

### Hardware Connections:
- DHT22: GPIO 4 (with 4.7kΩ pull-up resistor)
- PN532: GPIO 21 (SDA), GPIO 22 (SCL)
- IR Sensor: GPIO 32
- NFC Trigger: GPIO 15
- Mode Switch: GPIO 2

## Status: ✅ COMPLETE
All sensor integration features are implemented and functional. The ESP32 device now provides comprehensive environmental monitoring with real-time display on the e-ink dashboard.
Temperature: 23.4°C
Humidity: 45.2%
Server: http://192.168.1.100:5000
URL:http://192.168.1.100:5000
END:VCARD
```

### **NFC Usage:**
1. Pull GPIO 15 LOW to activate NFC write mode
2. Place NTAG213/215/216 tag near PN532 module
3. VCard with device info and current sensor data is written
4. Write-once protection prevents accidental overwrites

## ⚡ **Power Management**

### **Sleep Mode Features:**
- **Trigger**: No motion detected for 5 minutes
- **Actions**: Display deep sleep, reduced LED activity
- **Wake**: Immediate on motion detection
- **Recovery**: Auto-refresh content and flash LED on wake

## 🌡️ **Dashboard Integration**

### **Temperature Data Integration:**
- Temperature and humidity readings are now available for dashboard display
- Data updates every 30 seconds automatically
- Values included in system status reports
- Ready for server-side dashboard generation

## 📋 **Updated Controls**

### **GPIO 2 (Mode Switch + LED):**
- **SHORT PRESS**: Switch Dashboard ↔ Images mode
- **LONG PRESS (5+ sec)**: Device removal mode
- **LED**: Status indication and visual feedback

### **GPIO 15 (NFC Trigger):**
- **LOW**: Activate NFC write mode (one-time)
- **HIGH**: Normal operation

### **GPIO 32 (IR Sensor):**
- **HIGH**: Motion detected (wake device)
- **LOW**: No motion (sleep countdown)

## 🔧 **Dependencies Required**

Add these libraries to your Arduino IDE:
```cpp
#include <Wire.h>           // I2C communication
#include <Adafruit_PN532.h> // NFC PN532 module
#include <DHT.h>            // DHT22 temperature sensor
```

## ✅ **Integration Status**

- ✅ **PN532 NFC Module**: Fully integrated with I2C communication
- ✅ **DHT22 Sensor**: Temperature/humidity monitoring active
- ✅ **IR Motion Sensor**: Sleep/wake functionality implemented
- ✅ **Pin Reassignments**: All GPIO conflicts resolved
- ✅ **Status Monitoring**: Enhanced system reporting
- ✅ **Power Management**: Sleep mode with auto-wake
- ✅ **VCard Writing**: One-time NFC tag programming

## 🎯 **Next Steps for Dashboard Integration**

To display temperature data on the dashboard:

1. **Server-Side**: Modify PersonalCMS to accept temperature data from device
2. **API Integration**: Send sensor readings with device status updates
3. **Dashboard Generation**: Include temperature/humidity in BMP generation
4. **Real-Time Updates**: Display live sensor data on e-ink screen

The ESP32 DevKit V1 is now a complete IoT device with NFC capabilities, environmental monitoring, and intelligent power management! 🚀