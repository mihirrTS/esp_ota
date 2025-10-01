#!/usr/bin/env python3
"""
PersonalCMS with OTA Support
A unified CMS system for ESP32 devices with over-the-air update capabilities.

Initializes database, creates sample data, and starts the server
Single entry point for the entire PersonalCMS system
"""

import os
import sys
import subprocess
from datetime import datetime, timezone

def install_dependencies():
    """Install required Python packages"""
    print("📦 Installing Python dependencies...")
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "requirements.txt"])
        print("   ✅ Dependencies installed successfully")
        return True
    except subprocess.CalledProcessError:
        print("   ❌ Failed to install dependencies")
        return False
    except FileNotFoundError:
        print("   ⚠️  requirements.txt not found, creating one...")
        create_requirements_file()
        return install_dependencies()

def create_requirements_file():
    """Create requirements.txt if it doesn't exist"""
    requirements_content = """Flask==3.0.0
Flask-SQLAlchemy==3.1.1
Pillow==10.4.0
requests==2.31.0
feedparser==6.0.10
jsonpath-ng==1.6.1
Werkzeug==3.0.1"""
    
    with open("requirements.txt", "w") as f:
        f.write(requirements_content)
    print("   ✅ requirements.txt created")

def check_arduino_cli():
    """Check if Arduino CLI is available for firmware building"""
    try:
        result = subprocess.run(["arduino-cli", "version"], capture_output=True, text=True)
        if result.returncode == 0:
            print("   ✅ Arduino CLI available for firmware building")
            return True
    except FileNotFoundError:
        pass
    
    print("   ℹ️  Arduino CLI not found - firmware building not available")
    print("      Install from: https://arduino.github.io/arduino-cli/")
    return False

def initialize_database(app):
    """Initialize database with tables and sample data"""
    print("🔧 Initializing database...")
    
    with app.app_context():
        from unified_cms import db
        from models import Device, ContentAPI, DeviceContent
        
        # Drop existing tables if schema needs updating
        try:
            # Test if current schema is compatible
            db.session.execute(db.text("SELECT firmware_version FROM devices LIMIT 1"))
            print("   ✅ Database schema is up to date")
        except Exception:
            print("   🔄 Updating database schema...")
            db.drop_all()
            db.create_all()
            print("   ✅ Database schema updated")
        
        # Create all tables
        db.create_all()
        print("   ✅ Database tables ready")
        
        # Check if we need to add sample data
        try:
            if Device.query.count() == 0:
                print("   📝 Adding sample data...")
                
                # Create sample device
                sample_device = Device(
                    device_id="ESP32_DEV1_001",
                    device_name="Development Device 1",
                    occupation="Software Developer",
                    device_type="ESP32_DevKit_V1",
                    wifi_ssid="",
                    nickname="Dev Board",
                    is_active=True,
                    is_connected=False,
                    created_at=datetime.now(timezone.utc),
                    firmware_version="1.0.0"
                )
                db.session.add(sample_device)
                
                # Create sample content API (use correct field names)
                sample_content = ContentAPI(
                    category="Default",
                    subcategory="Welcome",
                    api_url="http://localhost:5000/api/default",
                    response_path="$.content",
                    is_active=True,
                    created_at=datetime.now(timezone.utc)
                )
                db.session.add(sample_content)
                
                db.session.commit()
                print("   ✅ Sample data created")
            else:
                print("   ✅ Database already contains data")
                
        except Exception as e:
            print(f"   ⚠️  Warning: Could not add sample data: {e}")
            db.session.rollback()

def check_dependencies():
    """Check if all required packages are installed"""
    required_packages = {
        'flask': 'flask',
        'flask_sqlalchemy': 'flask_sqlalchemy', 
        'PIL': 'PIL',
        'requests': 'requests',
        'feedparser': 'feedparser',
        'jsonpath_ng': 'jsonpath_ng',
        'werkzeug': 'werkzeug'
    }
    
    missing_packages = []
    for package_name, import_name in required_packages.items():
        try:
            __import__(import_name)
        except ImportError:
            missing_packages.append(package_name)
    
    if missing_packages:
        print(f"   ❌ Missing packages: {', '.join(missing_packages)}")
        return False
    else:
        print("   ✅ All required packages are available")
        return True

def main():
    """Main application entry point"""
    print("\n" + "=" * 80)
    print("🚀 PersonalCMS with OTA Support - Starting Up")
    print("=" * 80)
    
    # Check dependencies first
    print("\n📦 Checking dependencies...")
    if not check_dependencies():
        print("\n📥 Installing missing dependencies...")
        if not install_dependencies():
            print("\n❌ Failed to install dependencies. Please run:")
            print("   pip install -r requirements.txt")
            return 1
    
    # Import after ensuring dependencies are available
    try:
        from unified_cms import app, db
    except ImportError as e:
        print(f"\n❌ Failed to import unified_cms: {e}")
        print("   Make sure unified_cms.py is in the current directory")
        return 1
    
    # Initialize database
    initialize_database(app)
    
    # Ensure required directories exist
    directories = [
        'data', 'data/uploads', 'data/generated', 'data/device_content', 
        'data/ota', 'data/ota/firmware', 'templates', 'static', 
        'dashboards', 'firmware_binaries', 'instance'
    ]
    
    print("\n📁 Creating required directories...")
    for directory in directories:
        os.makedirs(directory, exist_ok=True)
        print(f"   ✅ {directory}")
    
    # Check for Arduino CLI
    print("\n🔧 Checking build tools...")
    check_arduino_cli()
    
    print("\n🌐 Starting PersonalCMS Server...")
    print("   📡 Features enabled:")
    print("      ✅ Per-device content management")
    print("      ✅ E-ink display integration")
    print("      ✅ Sensor data collection")
    print("      ✅ OTA firmware updates")
    print("      ✅ WiFi captive portal")
    print("      ✅ NFC VCard writing")
    print("      ✅ Image management")
    print("      ✅ Content APIs")
    
    print(f"\n🔗 Access points:")
    print(f"   🏠 Main Dashboard: http://localhost:5000/")
    print(f"   🔄 OTA Management: http://localhost:5000/ota-management")
    print(f"   📷 Image Upload: http://localhost:5000/upload")
    print(f"   📝 Content Admin: http://localhost:5000/content-admin")
    
    print(f"\n📡 ESP32 API endpoints:")
    print(f"   🔍 Device registration: POST /api/devices/register")
    print(f"   📊 Sensor data: POST /api/devices/{{device_id}}/sensor-data")
    print(f"   🖼️  Content fetch: GET /api/devices/{{device_id}}/images-sequence")
    print(f"   🔄 OTA check: GET /api/ota/check/{{device_id}}")
    print(f"   💓 Heartbeat: POST /api/devices/{{device_id}}/heartbeat")
    
    print(f"\n💡 Quick Start:")
    print(f"   1. Upload firmware via OTA Management")
    print(f"   2. Flash ESP32 via USB first time")
    print(f"   3. Configure via captive portal")
    print(f"   4. Device will auto-update thereafter")
    
    print("\n" + "=" * 80)
    print("✅ PersonalCMS Ready - Listening on http://0.0.0.0:5000")
    print("=" * 80)
    
    # Start the Flask app
    try:
        app.run(host='0.0.0.0', port=5000, debug=False)  # Set debug=False for production
    except KeyboardInterrupt:
        print("\n\n👋 PersonalCMS shutting down gracefully...")
    except Exception as e:
        print(f"\n❌ Server error: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())