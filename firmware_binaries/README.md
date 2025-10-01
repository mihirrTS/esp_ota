# ESP32 Firmware Binaries for OTA Updates

## 📁 Current Files

This directory contains firmware binary files (.bin) and their metadata (.json) for ESP32 OTA updates.

### Available Firmware:

1. **ESP32_PersonalCMS_Full.bin** - Complete PersonalCMS with all features
   - E-ink display integration
   - Sensor data collection
   - Content management
   - Image display capabilities
   - Size: ~1.2MB when compiled

2. **ESP32_OTA_Base.bin** - Minimal OTA-capable firmware
   - Basic WiFi setup
   - OTA update functionality
   - Status LED indicators
   - Size: ~800KB when compiled

3. **ESP32_LED_Blink.bin** - LED demonstration firmware
   - Multiple blink patterns
   - Web-configurable settings
   - OTA update capability
   - Size: ~700KB when compiled

## 🔨 Creating Actual Binary Files

### Method 1: Using Arduino CLI (Recommended)

1. **Install Arduino CLI:**
   ```bash
   # Download from: https://arduino.github.io/arduino-cli/
   # Or run the provided batch file:
   COMPILE_FIRMWARE.bat
   ```

2. **Setup ESP32 environment:**
   ```bash
   arduino-cli core install esp32:esp32
   ```

3. **Compile firmware:**
   ```bash
   # PersonalCMS Full
   arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir firmware_binaries custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_PersonalCMS.ino
   
   # OTA Base
   arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir firmware_binaries custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_OTA_Base.ino
   
   # LED Blink
   arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir firmware_binaries custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_LED_Blink.ino
   ```

### Method 2: Using Arduino IDE

1. **Install Arduino IDE 2.0+**
2. **Add ESP32 board package:**
   - File → Preferences
   - Additional Board Manager URLs: `https://dl.espressif.com/dl/package_esp32_index.json`
   - Tools → Board → Boards Manager → Search "ESP32" → Install

3. **Open firmware sketch:**
   - Open `.ino` file from `custom_driver/ESP32_DevKit_V1_PersonalCMS/`

4. **Configure board:**
   - Tools → Board → ESP32 Arduino → ESP32 Dev Module

5. **Compile and export:**
   - Sketch → Export Compiled Binary
   - Copy the generated `.bin` file to this directory

### Method 3: Using PlatformIO

1. **Install PlatformIO:**
   ```bash
   pip install platformio
   ```

2. **Create platformio.ini:**
   ```ini
   [env:esp32dev]
   platform = espressif32
   board = esp32dev
   framework = arduino
   ```

3. **Build:**
   ```bash
   pio run
   ```

## 🚀 Using the Binaries

### Upload via Web Interface:
1. Start the PersonalCMS server: `START_SERVER.bat`
2. Open: http://localhost:5000/ota-management
3. Upload your `.bin` files
4. Assign to devices
5. Devices will auto-update on next check

### ESP32 First-Time Setup:
1. Flash any firmware via USB using Arduino IDE or esptool
2. Device will create captive portal "ESP32-Setup-{DEVICE_ID}"
3. Connect and configure WiFi
4. Device will register with server and enable OTA

## 📋 File Structure

```
firmware_binaries/
├── ESP32_PersonalCMS_Full.bin    # Main firmware binary
├── ESP32_PersonalCMS_Full.json   # Firmware metadata
├── ESP32_OTA_Base.bin            # Minimal OTA firmware
├── ESP32_OTA_Base.json           # Metadata
├── ESP32_LED_Blink.bin           # Demo firmware  
├── ESP32_LED_Blink.json          # Metadata
└── README.md                     # This file
```

## ⚠️ Important Notes

- **Current .bin files are placeholders** - Replace with actual compiled binaries
- Always test firmware before deploying via OTA
- Keep backup of working firmware
- Verify device compatibility before upload
- Monitor OTA updates for successful completion

## 🔧 Troubleshooting

### Compilation Issues:
- Ensure all required libraries are installed
- Check ESP32 board package version
- Verify Arduino IDE/CLI setup

### OTA Update Issues:
- Check WiFi connectivity
- Verify server URL in firmware
- Ensure sufficient flash memory
- Monitor serial output during update

## 📞 Support

For issues:
1. Check firmware compilation logs
2. Verify OTA server endpoints
3. Monitor device serial output
4. Test with minimal OTA Base firmware first