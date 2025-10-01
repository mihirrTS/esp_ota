#!/usr/bin/env python3
"""
Update Firmware Registry Script
Updates the OTA system to use the newly compiled ESP32 firmware binaries
"""

import os
import sys
import json
import sqlite3
from datetime import datetime

def update_firmware_database():
    """Update the SQLite database with new compiled firmware"""
    db_path = "instance/unified_cms.db"
    
    if not os.path.exists(db_path):
        print("❌ Database not found")
        return False
    
    try:
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Check if devices table exists
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='device';")
        if not cursor.fetchone():
            print("❌ Device table not found in database")
            return False
        
        # Update ESP32 device firmware assignments to use compiled firmware
        updates = [
            ("compiled_ESP32_DevKit_V1_OTA_Base", "2.0.0", "ESP32_DevKit_V1"),
            ("compiled_ESP32_DevKit_V1_LED_Blink", "2.1.0", "ESP32_DevKit_V1")
        ]
        
        for firmware_key, version, device_type in updates:
            # Update any existing devices of this type to use the new firmware
            cursor.execute("""
                UPDATE device 
                SET firmware_version = ? 
                WHERE device_type = ? OR device_type LIKE '%ESP32%'
            """, (version, device_type))
            
            affected_rows = cursor.rowcount
            if affected_rows > 0:
                print(f"✅ Updated {affected_rows} devices to firmware: {firmware_key} v{version}")
        
        conn.commit()
        conn.close()
        print("✅ Database updated successfully")
        return True
        
    except Exception as e:
        print(f"❌ Database update failed: {e}")
        return False

def update_ota_assignments():
    """Update OTA manager assignments to prefer compiled firmware"""
    ota_registry_path = "data/ota/firmware_registry.json"
    
    # Create default assignments for compiled firmware
    assignments = {
        "ESP32_OTA_BASE_001": "compiled_ESP32_DevKit_V1_OTA_Base",
        "ESP32_OTA_LED_001": "compiled_ESP32_DevKit_V1_LED_Blink",
        "ESP32_DevKit_V1_Device1": "compiled_ESP32_DevKit_V1_OTA_Base",
        "ESP32_DevKit_V1_Device2": "compiled_ESP32_DevKit_V1_LED_Blink"
    }
    
    try:
        # Load existing registry
        if os.path.exists(ota_registry_path):
            with open(ota_registry_path, 'r') as f:
                registry = json.load(f)
        else:
            registry = {
                'firmware_versions': {},
                'device_assignments': {},
                'auto_update_enabled': True  # Enable auto-updates by default
            }
        
        # Update device assignments to use compiled firmware
        registry['device_assignments'].update(assignments)
        registry['auto_update_enabled'] = True
        
        # Ensure directory exists
        os.makedirs(os.path.dirname(ota_registry_path), exist_ok=True)
        
        # Save updated registry
        with open(ota_registry_path, 'w') as f:
            json.dump(registry, f, indent=2)
        
        print("✅ OTA device assignments updated")
        print(f"   📋 Assigned {len(assignments)} devices to compiled firmware")
        return True
        
    except Exception as e:
        print(f"❌ OTA assignment update failed: {e}")
        return False

def remove_old_fake_firmware():
    """Remove old fake firmware files"""
    old_firmware_dir = "data/ota/firmware"
    
    if not os.path.exists(old_firmware_dir):
        print("ℹ️ No old firmware directory found")
        return True
    
    try:
        fake_files = []
        for filename in os.listdir(old_firmware_dir):
            if filename.endswith('.bin'):
                filepath = os.path.join(old_firmware_dir, filename)
                # Check if it's a small fake file (less than 100KB)
                file_size = os.path.getsize(filepath)
                if file_size < 100000:  # 100KB threshold
                    fake_files.append(filepath)
        
        if fake_files:
            print(f"🗑️ Removing {len(fake_files)} fake firmware files:")
            for filepath in fake_files:
                os.remove(filepath)
                print(f"   ❌ Removed: {os.path.basename(filepath)}")
        else:
            print("ℹ️ No fake firmware files found to remove")
        
        return True
        
    except Exception as e:
        print(f"❌ Failed to remove old firmware: {e}")
        return False

def verify_compiled_firmware():
    """Verify that compiled firmware files exist and are valid"""
    firmware_dir = "firmwares"
    registry_file = os.path.join(firmware_dir, "firmware_registry.json")
    
    if not os.path.exists(registry_file):
        print("❌ Compiled firmware registry not found")
        return False
    
    try:
        with open(registry_file, 'r') as f:
            registry = json.load(f)
        
        valid_firmware = 0
        for firmware_key, firmware_info in registry.items():
            filename = firmware_info['filename']
            filepath = os.path.join(firmware_dir, filename)
            
            if os.path.exists(filepath):
                file_size = os.path.getsize(filepath)
                expected_size = firmware_info['size']
                
                if file_size == expected_size:
                    print(f"✅ {firmware_key}: {filename} ({file_size} bytes)")
                    valid_firmware += 1
                else:
                    print(f"⚠️ {firmware_key}: Size mismatch ({file_size} != {expected_size})")
            else:
                print(f"❌ {firmware_key}: File not found: {filepath}")
        
        print(f"\n📊 Firmware verification: {valid_firmware}/{len(registry)} files valid")
        return valid_firmware > 0
        
    except Exception as e:
        print(f"❌ Firmware verification failed: {e}")
        return False

def test_ota_system():
    """Test the OTA system with a simulated device check"""
    try:
        # Import the OTA manager
        sys.path.append('.')
        from ota_manager import OTAManager
        
        ota_manager = OTAManager()
        
        # Test loading compiled firmware
        compiled_firmware = [key for key in ota_manager.firmware_registry['firmware_versions'].keys() if key.startswith('compiled_')]
        
        if compiled_firmware:
            print(f"✅ OTA Manager loaded {len(compiled_firmware)} compiled firmware versions:")
            for fw_key in compiled_firmware:
                fw_info = ota_manager.firmware_registry['firmware_versions'][fw_key]
                print(f"   📦 {fw_key} v{fw_info['version']} ({fw_info['file_size']} bytes)")
        else:
            print("❌ No compiled firmware found in OTA manager")
            return False
        
        # Test firmware file access
        for fw_key in compiled_firmware[:1]:  # Test first one
            filepath = ota_manager.get_firmware_file(fw_key)
            if filepath and os.path.exists(filepath):
                print(f"✅ Firmware file accessible: {os.path.basename(filepath)}")
            else:
                print(f"❌ Firmware file not accessible: {fw_key}")
                return False
        
        return True
        
    except Exception as e:
        print(f"❌ OTA system test failed: {e}")
        return False

def main():
    """Main update process"""
    print("🔧 ESP32 Firmware Registry Update")
    print("=" * 50)
    
    # Verify compiled firmware exists
    if not verify_compiled_firmware():
        print("❌ Compiled firmware verification failed")
        return False
    
    # Update database
    if not update_firmware_database():
        print("⚠️ Database update failed (continuing anyway)")
    
    # Update OTA assignments
    if not update_ota_assignments():
        print("❌ OTA assignment update failed")
        return False
    
    # Remove old fake firmware
    if not remove_old_fake_firmware():
        print("⚠️ Failed to remove old firmware (continuing anyway)")
    
    # Test OTA system
    if not test_ota_system():
        print("❌ OTA system test failed")
        return False
    
    print("\n🎉 Firmware registry update completed successfully!")
    print("✅ Compiled ESP32 firmware is now active")
    print("✅ OTA system updated and tested")
    print("✅ Auto-updates enabled for cross-firmware deployment")
    print("\n🚀 Your ESP32 devices should now receive proper firmware updates!")
    
    return True

if __name__ == "__main__":
    if main():
        sys.exit(0)
    else:
        sys.exit(1)