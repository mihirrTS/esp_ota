@echo off
echo.
echo ================================================================================
echo                     PersonalCMS - First Time Setup
echo ================================================================================
echo.

REM Change to the script directory
cd /d "%~dp0"

echo [INFO] Setting up Python virtual environment...

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python is not installed or not in PATH!
    echo Please install Python 3.8+ from https://python.org
    echo.
    pause
    exit /b 1
)

REM Create virtual environment if it doesn't exist
if not exist ".venv" (
    echo [INFO] Creating virtual environment...
    python -m venv .venv
    if errorlevel 1 (
        echo [ERROR] Failed to create virtual environment!
        pause
        exit /b 1
    )
)

REM Activate virtual environment and install dependencies
echo [INFO] Installing dependencies...
.venv\Scripts\pip.exe install --upgrade pip
.venv\Scripts\pip.exe install -r requirements.txt

if errorlevel 1 (
    echo [ERROR] Failed to install dependencies!
    pause
    exit /b 1
)

REM Create necessary directories
echo [INFO] Creating project directories...
if not exist "data" mkdir data
if not exist "data\uploads" mkdir data\uploads
if not exist "data\generated" mkdir data\generated
if not exist "data\device_content" mkdir data\device_content
if not exist "data\ota" mkdir data\ota
if not exist "data\ota\firmware" mkdir data\ota\firmware
if not exist "firmware_binaries" mkdir firmware_binaries
if not exist "instance" mkdir instance

echo.
echo ================================================================================
echo                            Setup Complete!
echo ================================================================================
echo.
echo Next steps:
echo   1. Double-click START_SERVER.bat to run the server
echo   2. Open http://localhost:5000 in your browser
echo   3. Upload ESP32 firmware via OTA Management
echo.
echo For ESP32 development:
echo   - Install Arduino IDE or PlatformIO
echo   - Use firmware files in custom_driver/ folder
echo.
pause