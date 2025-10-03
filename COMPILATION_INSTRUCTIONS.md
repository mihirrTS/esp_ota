# Arduino Compilation Instructions for v2.4.0 Firmware

## 🎯 **Important Notice**
The Arduino source code (.ino files) have been updated with heartbeat fixes, but the binary files (.bin) need to be recompiled to include these changes.

## 📋 **Updated Source Files**
- ✅ `ESP32_DevKit_V1_LED_Blink.ino` - Updated with occupation field and heartbeat fixes
- ✅ `ESP32_DevKit_V1_OTA_Base.ino` - Updated with occupation field and heartbeat fixes  
- ✅ `ESP32_DevKit_V1_PersonalCMS.ino` - Updated version number to 2.4.0

## 🔧 **Compilation Steps**

### **Using Arduino IDE:**

1. **Open Arduino IDE 2.x**
2. **Install ESP32 Board Package** (if not already installed):
   - Go to File → Preferences
   - Add to Additional Board Manager URLs: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   - Go to Tools → Board → Boards Manager
   - Search for "esp32" and install "ESP32 by Espressif Systems"

3. **Configure Board Settings:**
   - Board: "ESP32 Dev Module"
   - CPU Frequency: "240MHz (WiFi/BT)"
   - Flash Frequency: "80MHz" 
   - Flash Mode: "DIO"
   - Flash Size: "4MB (32Mb)"
   - Partition Scheme: "Default 4MB with spiffs"

4. **Compile Each Firmware:**

   **a) LED Blink Firmware:**
   ```
   Open: custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_LED_Blink/ESP32_DevKit_V1_LED_Blink.ino
   Click: Sketch → Export Compiled Binary
   Result: Creates ESP32_DevKit_V1_LED_Blink.ino.bin in the same folder
   ```

   **b) OTA Base Firmware:**
   ```
   Open: custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_OTA_Base/ESP32_DevKit_V1_OTA_Base.ino
   Click: Sketch → Export Compiled Binary  
   Result: Creates ESP32_DevKit_V1_OTA_Base.ino.bin in the same folder
   ```

   **c) PersonalCMS Firmware:**
   ```
   Open: custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_PersonalCMS.ino
   Click: Sketch → Export Compiled Binary
   Result: Creates ESP32_DevKit_V1_PersonalCMS.ino.bin in the same folder
   ```

5. **Copy and Rename Binary Files:**
   ```powershell
   # LED Blink
   Copy-Item "ESP32_DevKit_V1_LED_Blink.ino.bin" "bin\ESP32_DevKit_V1_LED_Blink_v2.4.bin"
   
   # OTA Base  
   Copy-Item "ESP32_DevKit_V1_OTA_Base.ino.bin" "bin\ESP32_DevKit_V1_OTA_Base_v2.4.bin"
   
   # PersonalCMS
   Copy-Item "ESP32_DevKit_V1_PersonalCMS.ino.bin" "bin\ESP32_DevKit_V1_PersonalCMS_v2.4.bin"
   ```

6. **Upload to OTA Server:**
   - Use the web interface at http://localhost:5000/upload
   - Or use the Python upload script provided earlier

## ⚠️ **Current Status**
- **Source Code**: ✅ Updated with v2.4.0 heartbeat fixes
- **Binary Files**: ❌ Still contain old compiled code (need recompilation)
- **Server Integration**: ✅ Working (server-side heartbeat fix is active)
- **Device Connectivity**: ✅ Working (ESP32_OTA_LED_001 successfully connected)

## 🔄 **Alternative: Use Pre-compiled v2.3 with Server Fixes**
Since the server-side heartbeat fix is already working and devices are connecting successfully, the current v2.3 binaries are functional. The v2.4.0 source code improvements will take effect when properly compiled.

## 📝 **Note for Future Development**
To avoid this issue in the future, consider setting up Arduino CLI for automated compilation:
```bash
arduino-cli core install esp32:esp32
arduino-cli compile --fqbn esp32:esp32:esp32doit-devkit-v1 sketch.ino
```