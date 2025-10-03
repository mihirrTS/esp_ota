@echo off
echo =====================================================
echo ESP32 DevKit V1 Firmware Compilation Script
echo Building all firmware variants with heartbeat fixes
echo =====================================================

set ARDUINO_CLI_PATH=C:\Users\mihir\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\4.5.1\esptool.exe
set ARDUINO_CORE_PATH=C:\Users\mihir\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.0.7
set LIBRARIES_PATH=C:\Users\mihir\Documents\Arduino\libraries
set PROJECT_ROOT=%~dp0

echo.
echo Checking Arduino CLI and ESP32 core installation...
if not exist "%ARDUINO_CLI_PATH%" (
    echo ERROR: Arduino CLI esptool not found at %ARDUINO_CLI_PATH%
    echo Please install ESP32 core through Arduino IDE or CLI
    pause
    exit /b 1
)

if not exist "%ARDUINO_CORE_PATH%" (
    echo ERROR: ESP32 core not found at %ARDUINO_CORE_PATH%
    echo Please install ESP32 core version 3.0.7 through Arduino IDE
    pause
    exit /b 1
)

echo ✅ Arduino environment detected
echo.

:: Set compilation variables
set FQBN=esp32:esp32:esp32doit-devkit-v1
set BUILD_PROPERTIES=build.flash_mode=dio,build.flash_size=4MB,build.flash_freq=40m,build.boot=dio

:: Create firmwares directory if it doesn't exist
if not exist "%PROJECT_ROOT%firmwares" (
    mkdir "%PROJECT_ROOT%firmwares"
    echo Created firmwares directory
)

echo.
echo =====================================================
echo 1. Building ESP32_DevKit_V1_PersonalCMS v2.4.0
echo =====================================================

set SKETCH_PATH=%PROJECT_ROOT%custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_PersonalCMS.ino
set OUTPUT_DIR=%PROJECT_ROOT%firmwares\PersonalCMS_build

arduino-cli compile --fqbn %FQBN% --build-path "%OUTPUT_DIR%" --build-property "%BUILD_PROPERTIES%" "%SKETCH_PATH%"

if %errorlevel% equ 0 (
    echo ✅ PersonalCMS compilation successful
    
    :: Copy binary files
    copy "%OUTPUT_DIR%\ESP32_DevKit_V1_PersonalCMS.ino.bin" "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_PersonalCMS_v2.4.bin"
    copy "%OUTPUT_DIR%\ESP32_DevKit_V1_PersonalCMS.ino.bootloader.bin" "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_PersonalCMS_v2.4.bootloader.bin"
    copy "%OUTPUT_DIR%\ESP32_DevKit_V1_PersonalCMS.ino.partitions.bin" "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_PersonalCMS_v2.4.partitions.bin"
    
    :: Create merged binary using esptool
    "%ARDUINO_CLI_PATH%" --chip esp32 merge_bin -o "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_PersonalCMS_v2.4.merged.bin" --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_PersonalCMS_v2.4.bootloader.bin" 0x8000 "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_PersonalCMS_v2.4.partitions.bin" 0x10000 "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_PersonalCMS_v2.4.bin"
    
    echo ✅ PersonalCMS v2.4.0 firmware created
) else (
    echo ❌ PersonalCMS compilation failed
    set COMPILATION_FAILED=1
)

echo.
echo =====================================================
echo 2. Building ESP32_DevKit_V1_LED_Blink v2.4.0
echo =====================================================

set SKETCH_PATH=%PROJECT_ROOT%custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_LED_Blink\ESP32_DevKit_V1_LED_Blink.ino
set OUTPUT_DIR=%PROJECT_ROOT%firmwares\LED_Blink_build

arduino-cli compile --fqbn %FQBN% --build-path "%OUTPUT_DIR%" --build-property "%BUILD_PROPERTIES%" "%SKETCH_PATH%"

if %errorlevel% equ 0 (
    echo ✅ LED Blink compilation successful
    
    :: Copy binary files
    copy "%OUTPUT_DIR%\ESP32_DevKit_V1_LED_Blink.ino.bin" "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_LED_Blink_v2.4.bin"
    copy "%OUTPUT_DIR%\ESP32_DevKit_V1_LED_Blink.ino.bootloader.bin" "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_LED_Blink_v2.4.bootloader.bin"
    copy "%OUTPUT_DIR%\ESP32_DevKit_V1_LED_Blink.ino.partitions.bin" "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_LED_Blink_v2.4.partitions.bin"
    
    :: Create merged binary
    "%ARDUINO_CLI_PATH%" --chip esp32 merge_bin -o "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_LED_Blink_v2.4.merged.bin" --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_LED_Blink_v2.4.bootloader.bin" 0x8000 "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_LED_Blink_v2.4.partitions.bin" 0x10000 "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_LED_Blink_v2.4.bin"
    
    echo ✅ LED Blink v2.4.0 firmware created
) else (
    echo ❌ LED Blink compilation failed
    set COMPILATION_FAILED=1
)

echo.
echo =====================================================
echo 3. Building ESP32_DevKit_V1_OTA_Base v2.4.0
echo =====================================================

set SKETCH_PATH=%PROJECT_ROOT%custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_OTA_Base\ESP32_DevKit_V1_OTA_Base.ino
set OUTPUT_DIR=%PROJECT_ROOT%firmwares\OTA_Base_build

arduino-cli compile --fqbn %FQBN% --build-path "%OUTPUT_DIR%" --build-property "%BUILD_PROPERTIES%" "%SKETCH_PATH%"

if %errorlevel% equ 0 (
    echo ✅ OTA Base compilation successful
    
    :: Copy binary files
    copy "%OUTPUT_DIR%\ESP32_DevKit_V1_OTA_Base.ino.bin" "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_OTA_Base_v2.4.bin"
    copy "%OUTPUT_DIR%\ESP32_DevKit_V1_OTA_Base.ino.bootloader.bin" "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_OTA_Base_v2.4.bootloader.bin"
    copy "%OUTPUT_DIR%\ESP32_DevKit_V1_OTA_Base.ino.partitions.bin" "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_OTA_Base_v2.4.partitions.bin"
    
    :: Create merged binary
    "%ARDUINO_CLI_PATH%" --chip esp32 merge_bin -o "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_OTA_Base_v2.4.merged.bin" --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_OTA_Base_v2.4.bootloader.bin" 0x8000 "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_OTA_Base_v2.4.partitions.bin" 0x10000 "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_OTA_Base_v2.4.bin"
    
    echo ✅ OTA Base v2.4.0 firmware created
) else (
    echo ❌ OTA Base compilation failed
    set COMPILATION_FAILED=1
)

echo.
echo =====================================================
echo 4. Updating custom_driver bin directories
echo =====================================================

:: Update PersonalCMS bin directory
set PERSONAL_CMS_BIN=%PROJECT_ROOT%custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_PersonalCMS\bin
if exist "%PERSONAL_CMS_BIN%" (
    echo Removing old PersonalCMS binaries...
    del /q "%PERSONAL_CMS_BIN%\ESP32_DevKit_V1_PersonalCMS_v2.2.*" 2>nul
    del /q "%PERSONAL_CMS_BIN%\ESP32_DevKit_V1_PersonalCMS_v2.3.*" 2>nul
    
    echo Copying new PersonalCMS v2.4.0 binaries...
    copy "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_PersonalCMS_v2.4.bin" "%PERSONAL_CMS_BIN%\"
    copy "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_PersonalCMS_v2.4.merged.bin" "%PERSONAL_CMS_BIN%\"
    
    :: Create JSON metadata
    echo {"version": "2.4.0", "description": "PersonalCMS firmware v2.4.0 with heartbeat occupation fix", "build_date": "%date%", "device_type": "ESP32_PersonalCMS"} > "%PERSONAL_CMS_BIN%\ESP32_DevKit_V1_PersonalCMS_v2.4.json"
)

:: Update LED Blink bin directory
set LED_BLINK_BIN=%PROJECT_ROOT%custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_LED_Blink\bin
if exist "%LED_BLINK_BIN%" (
    echo Removing old LED Blink binaries...
    del /q "%LED_BLINK_BIN%\ESP32_DevKit_V1_LED_Blink_v2.2.*" 2>nul
    del /q "%LED_BLINK_BIN%\ESP32_DevKit_V1_LED_Blink_v2.3.*" 2>nul
    
    echo Copying new LED Blink v2.4.0 binaries...
    copy "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_LED_Blink_v2.4.bin" "%LED_BLINK_BIN%\"
    copy "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_LED_Blink_v2.4.merged.bin" "%LED_BLINK_BIN%\"
    
    :: Create JSON metadata
    echo {"version": "2.4.0", "description": "LED Blink firmware v2.4.0 with heartbeat occupation fix", "build_date": "%date%", "device_type": "ESP32_LED_Blink"} > "%LED_BLINK_BIN%\ESP32_DevKit_V1_LED_Blink_v2.4.json"
)

:: Update OTA Base bin directory  
set OTA_BASE_BIN=%PROJECT_ROOT%custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_OTA_Base\bin
if exist "%OTA_BASE_BIN%" (
    echo Removing old OTA Base binaries...
    del /q "%OTA_BASE_BIN%\ESP32_DevKit_V1_OTA_Base_v2.2.*" 2>nul
    del /q "%OTA_BASE_BIN%\ESP32_DevKit_V1_OTA_Base_v2.3.*" 2>nul
    
    echo Copying new OTA Base v2.4.0 binaries...
    copy "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_OTA_Base_v2.4.bin" "%OTA_BASE_BIN%\"
    copy "%PROJECT_ROOT%firmwares\ESP32_DevKit_V1_OTA_Base_v2.4.merged.bin" "%OTA_BASE_BIN%\"
    
    :: Create JSON metadata
    echo {"version": "2.4.0", "description": "OTA Base firmware v2.4.0 with heartbeat occupation fix", "build_date": "%date%", "device_type": "ESP32_OTA_Base"} > "%OTA_BASE_BIN%\ESP32_DevKit_V1_OTA_Base_v2.4.json"
)

echo.
echo =====================================================
echo 5. Clean up build directories
echo =====================================================

if exist "%PROJECT_ROOT%firmwares\PersonalCMS_build" (
    rd /s /q "%PROJECT_ROOT%firmwares\PersonalCMS_build"
)
if exist "%PROJECT_ROOT%firmwares\LED_Blink_build" (
    rd /s /q "%PROJECT_ROOT%firmwares\LED_Blink_build"
)
if exist "%PROJECT_ROOT%firmwares\OTA_Base_build" (
    rd /s /q "%PROJECT_ROOT%firmwares\OTA_Base_build"
)

echo ✅ Build directories cleaned up

echo.
echo =====================================================
echo COMPILATION SUMMARY
echo =====================================================

if "%COMPILATION_FAILED%"=="1" (
    echo ❌ Some firmware compilation failed
    echo Please check Arduino IDE installation and ESP32 core
    echo Manual compilation may be needed through Arduino IDE
) else (
    echo ✅ All firmware compiled successfully!
    echo.
    echo 📁 Firmware files created in:
    echo    • %PROJECT_ROOT%firmwares\
    echo    • %PROJECT_ROOT%custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_PersonalCMS\bin\
    echo    • %PROJECT_ROOT%custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_LED_Blink\bin\
    echo    • %PROJECT_ROOT%custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_OTA_Base\bin\
    echo.
    echo 🎯 Ready for OTA deployment!
    echo    All firmware includes heartbeat occupation field fixes
    echo    Version 2.4.0 with improved device registration
)

echo.
echo Press any key to exit...
pause >nul