@echo off
echo.
echo ================================================================================
echo                    ESP32 Firmware Compilation Script
echo ================================================================================
echo.

REM Change to the script directory
cd /d "%~dp0"

echo [INFO] Checking for Arduino CLI...

REM Check if Arduino CLI exists
arduino-cli version >nul 2>&1
if errorlevel 1 (
    echo [WARNING] Arduino CLI not found. Installing...
    
    REM Create tools directory
    if not exist "tools" mkdir tools
    cd tools
    
    REM Download Arduino CLI
    echo [INFO] Downloading Arduino CLI...
    powershell -Command "Invoke-WebRequest -Uri 'https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip' -OutFile 'arduino-cli.zip'"
    
    if exist "arduino-cli.zip" (
        echo [INFO] Extracting Arduino CLI...
        powershell -Command "Expand-Archive -Path 'arduino-cli.zip' -DestinationPath '.' -Force"
        del arduino-cli.zip
        
        REM Add to PATH for this session
        set PATH=%PATH%;%cd%
        cd ..
        
        echo [SUCCESS] Arduino CLI installed!
    ) else (
        echo [ERROR] Failed to download Arduino CLI!
        echo Please install manually from: https://arduino.github.io/arduino-cli/
        pause
        exit /b 1
    )
) else (
    echo [SUCCESS] Arduino CLI found!
)

echo.
echo [INFO] Setting up Arduino CLI...

REM Initialize Arduino CLI
arduino-cli config init --overwrite

REM Update core index
echo [INFO] Updating board index...
arduino-cli core update-index

REM Install ESP32 core
echo [INFO] Installing ESP32 board package...
arduino-cli core install esp32:esp32

REM Install required libraries
echo [INFO] Installing required libraries...
arduino-cli lib install "ArduinoJson"
arduino-cli lib install "WiFi"
arduino-cli lib install "WebServer" 
arduino-cli lib install "HTTPClient"
arduino-cli lib install "HTTPUpdate"
arduino-cli lib install "Preferences"

echo.
echo [INFO] Creating firmware binaries directory...
if not exist "firmware_binaries" mkdir firmware_binaries

echo.
echo [INFO] Compiling firmware projects...

REM Compile PersonalCMS Full
echo [INFO] Compiling ESP32_PersonalCMS_Full...
arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir firmware_binaries custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_PersonalCMS.ino
if exist "firmware_binaries\ESP32_DevKit_V1_PersonalCMS.ino.bin" (
    ren "firmware_binaries\ESP32_DevKit_V1_PersonalCMS.ino.bin" "ESP32_PersonalCMS_Full.bin"
    echo    ✅ ESP32_PersonalCMS_Full.bin created
) else (
    echo    ❌ Failed to compile ESP32_PersonalCMS_Full
)

REM Compile OTA Base
echo [INFO] Compiling ESP32_OTA_Base...
arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir firmware_binaries custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_OTA_Base.ino
if exist "firmware_binaries\ESP32_DevKit_V1_OTA_Base.ino.bin" (
    ren "firmware_binaries\ESP32_DevKit_V1_OTA_Base.ino.bin" "ESP32_OTA_Base.bin"
    echo    ✅ ESP32_OTA_Base.bin created
) else (
    echo    ❌ Failed to compile ESP32_OTA_Base
)

REM Compile LED Blink
echo [INFO] Compiling ESP32_LED_Blink...
arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir firmware_binaries custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_LED_Blink.ino
if exist "firmware_binaries\ESP32_DevKit_V1_LED_Blink.ino.bin" (
    ren "firmware_binaries\ESP32_DevKit_V1_LED_Blink.ino.bin" "ESP32_LED_Blink.bin"
    echo    ✅ ESP32_LED_Blink.bin created
) else (
    echo    ❌ Failed to compile ESP32_LED_Blink
)

echo.
echo [INFO] Compilation Summary:
echo ================================================================================

if exist "firmware_binaries\ESP32_PersonalCMS_Full.bin" (
    for %%A in ("firmware_binaries\ESP32_PersonalCMS_Full.bin") do echo    📄 ESP32_PersonalCMS_Full.bin - %%~zA bytes
)

if exist "firmware_binaries\ESP32_OTA_Base.bin" (
    for %%A in ("firmware_binaries\ESP32_OTA_Base.bin") do echo    📄 ESP32_OTA_Base.bin - %%~zA bytes
)

if exist "firmware_binaries\ESP32_LED_Blink.bin" (
    for %%A in ("firmware_binaries\ESP32_LED_Blink.bin") do echo    📄 ESP32_LED_Blink.bin - %%~zA bytes
)

echo.
echo ✅ Firmware compilation complete!
echo    Binary files are available in firmware_binaries/ directory
echo    Upload them via OTA Management interface at: http://localhost:5000/ota-management
echo.
pause