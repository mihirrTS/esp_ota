@echo off
echo.
echo ================================================================================
echo                        PersonalCMS with OTA Updates
echo ================================================================================
echo.

REM Change to the script directory
cd /d "%~dp0"

REM Check if virtual environment exists
if not exist ".venv\Scripts\python.exe" (
    echo [ERROR] Virtual environment not found!
    echo Please run the Python setup first or create a virtual environment.
    echo.
    pause
    exit /b 1
)

REM Activate virtual environment and check dependencies
echo [INFO] Checking Python environment...
.venv\Scripts\python.exe -c "import sys; print(f'Python: {sys.version}'); import flask, flask_sqlalchemy, PIL, requests, werkzeug; import feedparser; import jsonpath_ng; print('[SUCCESS] All dependencies are installed')" 2>nul
if errorlevel 1 (
    echo [WARNING] Some dependencies missing. Installing from requirements.txt...
    .venv\Scripts\python.exe -m pip install -r requirements.txt
    if errorlevel 1 (
        echo [ERROR] Failed to install dependencies!
        pause
        exit /b 1
    )
)

REM Create necessary directories
echo [INFO] Creating required directories...
if not exist "data" mkdir data
if not exist "data\uploads" mkdir data\uploads
if not exist "data\generated" mkdir data\generated
if not exist "data\device_content" mkdir data\device_content
if not exist "data\ota" mkdir data\ota
if not exist "data\ota\firmware" mkdir data\ota\firmware
if not exist "firmware_binaries" mkdir firmware_binaries
if not exist "instance" mkdir instance

echo.
echo [INFO] Starting PersonalCMS Server...
echo.
echo ================================================================================
echo   🌐 Server will be available at: http://localhost:5000
echo   🔄 OTA Management: http://localhost:5000/ota-management
echo   📷 Image Upload: http://localhost:5000/upload
echo   📝 Content Admin: http://localhost:5000/content-admin
echo ================================================================================
echo.
echo Press Ctrl+C to stop the server
echo.

REM Start the server
.venv\Scripts\python.exe START_PERSONALCMS_OTA.py

echo.
echo [INFO] Server stopped.
pause