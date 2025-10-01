#!/usr/bin/env python3
"""
Database Initialization Script
Creates the database and tables for the PersonalCMS system
"""

import os
import sys
from pathlib import Path

# Add the project directory to the Python path
project_dir = Path(__file__).parent
sys.path.insert(0, str(project_dir))

try:
    from unified_cms import app, db
    from models import Device, DeviceContent, DefaultContent
    
    def init_database():
        """Initialize database with tables and sample data"""
        print("🔧 Initializing PersonalCMS Database...")
        
        with app.app_context():
            # Create all tables
            db.create_all()
            print("✅ Database tables created")
            
            # Add sample ESP32 devices if they don't exist
            sample_devices = [
                {
                    "device_id": "ESP32_OTA_BASE_001",
                    "device_name": "ESP32 Base Device",
                    "occupation": "OTA Testing",
                    "device_type": "ESP32_DevKit_V1",
                    "firmware_version": "2.0.0",
                    "is_active": True
                },
                {
                    "device_id": "ESP32_OTA_LED_001", 
                    "device_name": "ESP32 LED Device",
                    "occupation": "LED Pattern Testing",
                    "device_type": "ESP32_DevKit_V1",
                    "firmware_version": "2.1.0",
                    "is_active": True
                }
            ]
            
            for device_data in sample_devices:
                existing_device = Device.query.filter_by(device_id=device_data["device_id"]).first()
                if not existing_device:
                    device = Device(
                        device_id=device_data["device_id"],
                        device_name=device_data["device_name"],
                        occupation=device_data["occupation"],
                        device_type=device_data["device_type"],
                        firmware_version=device_data["firmware_version"],
                        is_active=device_data["is_active"]
                    )
                    db.session.add(device)
                    print(f"✅ Added device: {device_data['device_id']}")
                else:
                    print(f"ℹ️ Device already exists: {device_data['device_id']}")
            
            # Create default content if needed
            existing_content = DefaultContent.query.first()
            if not existing_content:
                content = DefaultContent(
                    category="system",
                    subcategory="default",
                    content="Default PersonalCMS Content",
                    title="System Default",
                    is_active=True
                )
                db.session.add(content)
                print("✅ Added default content")
            
            # Commit changes
            db.session.commit()
            print("✅ Database initialization completed")
            
            # Display summary
            device_count = Device.query.count()
            content_count = DefaultContent.query.count()
            print(f"\n📊 Database Summary:")
            print(f"   📱 Devices: {device_count}")
            print(f"   📄 Content: {content_count}")
            
        return True
    
    if __name__ == "__main__":
        try:
            if init_database():
                print("\n🎉 Database initialization successful!")
                sys.exit(0)
            else:
                print("\n❌ Database initialization failed!")
                sys.exit(1)
        except Exception as e:
            print(f"\n❌ Database initialization error: {e}")
            sys.exit(1)

except ImportError as e:
    print(f"❌ Import error: {e}")
    print("Make sure you're running this from the project directory")
    sys.exit(1)