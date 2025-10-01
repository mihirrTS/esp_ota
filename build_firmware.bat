@echo off
echo ============================================================================
echo PersonalCMS ESP32 OTA Firmware Build Script
echo ============================================================================
echo.

REM Check if Arduino CLI is available
arduino-cli version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Arduino CLI not found. Please install Arduino CLI first.
    echo Download from: https://arduino.github.io/arduino-cli/
    pause
    exit /b 1
)

echo 1. Setting up Arduino CLI environment...
arduino-cli core update-index
arduino-cli core install esp32:esp32

echo.
echo 2. Installing required libraries...
arduino-cli lib install "ArduinoJson"
arduino-cli lib install "HTTPUpdate"
arduino-cli lib install "Adafruit PN532"
arduino-cli lib install "DHT sensor library"

REM Create output directory for binaries
if not exist "firmware_binaries" mkdir firmware_binaries

echo.
echo 3. Building ESP32 PersonalCMS (Full Version)...
cd custom_driver\ESP32_DevKit_V1_PersonalCMS
arduino-cli compile --fqbn esp32:esp32:esp32 ESP32_DevKit_V1_PersonalCMS.ino
if exist ESP32_DevKit_V1_PersonalCMS.ino.esp32.esp32.esp32.bin (
    copy ESP32_DevKit_V1_PersonalCMS.ino.esp32.esp32.esp32.bin ..\..\firmware_binaries\ESP32_PersonalCMS_2.0.0.bin
    echo ✅ Built: ESP32_PersonalCMS_2.0.0.bin
) else (
    echo ❌ Failed to build ESP32 PersonalCMS
)
cd ..\..

echo.
echo 4. Building ESP32 OTA Base (Bare-bones Version)...
cd custom_driver\ESP32_DevKit_V1_PersonalCMS
arduino-cli compile --fqbn esp32:esp32:esp32 ESP32_DevKit_V1_OTA_Base.ino
if exist ESP32_DevKit_V1_OTA_Base.ino.esp32.esp32.esp32.bin (
    copy ESP32_DevKit_V1_OTA_Base.ino.esp32.esp32.esp32.bin ..\..\firmware_binaries\ESP32_OTA_Base_1.0.0.bin
    echo ✅ Built: ESP32_OTA_Base_1.0.0.bin
) else (
    echo ❌ Failed to build ESP32 OTA Base
)
cd ..\..

echo.
echo 5. Building ESP32 LED Blink (LED Pattern Version)...
cd custom_driver\ESP32_DevKit_V1_PersonalCMS
arduino-cli compile --fqbn esp32:esp32:esp32 ESP32_DevKit_V1_LED_Blink.ino
if exist ESP32_DevKit_V1_LED_Blink.ino.esp32.esp32.esp32.bin (
    copy ESP32_DevKit_V1_LED_Blink.ino.esp32.esp32.esp32.bin ..\..\firmware_binaries\ESP32_LED_Blink_1.0.0.bin
    echo ✅ Built: ESP32_LED_Blink_1.0.0.bin
) else (
    echo ❌ Failed to build ESP32 LED Blink
)
cd ..\..

echo.
echo ============================================================================
echo Build Summary:
echo ============================================================================
if exist firmware_binaries\ESP32_PersonalCMS_2.0.0.bin echo ✅ ESP32_PersonalCMS_2.0.0.bin - Full featured PersonalCMS
if exist firmware_binaries\ESP32_OTA_Base_1.0.0.bin echo ✅ ESP32_OTA_Base_1.0.0.bin - Bare-bones OTA capable
if exist firmware_binaries\ESP32_LED_Blink_1.0.0.bin echo ✅ ESP32_LED_Blink_1.0.0.bin - LED patterns with OTA

echo.
echo Firmware binaries are ready in the 'firmware_binaries' folder.
echo You can now upload these .bin files through the PersonalCMS OTA interface.
echo.
echo Access OTA Management at: http://localhost:5000/ota-management
echo ============================================================================
echo.

pause