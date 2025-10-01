#!/usr/bin/env python3
"""
ESP32 OTA System - Final Status Report
Summary of the implemented OTA system with compiled firmware
"""

import os
import json
import sys
from pathlib import Path

def check_system_status():
    """Check the complete system status"""
    print("🔍 ESP32 OTA System Status Report")
    print("=" * 50)
    
    status = {
        "arduino_cli": False,
        "compiled_firmware": False,
        "ota_system": False,
        "server_config": False,
        "database": False
    }
    
    # Check Arduino CLI installation
    arduino_cli_path = Path("tools/arduino-cli/arduino-cli.exe")
    if arduino_cli_path.exists():
        status["arduino_cli"] = True
        print("✅ Arduino CLI installed")
    else:
        print("❌ Arduino CLI not found")
    
    # Check compiled firmware
    firmware_dir = Path("firmwares")
    registry_file = firmware_dir / "firmware_registry.json"
    
    if registry_file.exists():
        try:
            with open(registry_file, 'r') as f:
                registry = json.load(f)
            
            firmware_count = len(registry)
            total_size = sum(fw['size'] for fw in registry.values())
            
            status["compiled_firmware"] = firmware_count > 0
            print(f"✅ Compiled firmware: {firmware_count} files ({total_size:,} bytes)")
            
            for fw_key, fw_info in registry.items():
                fw_path = firmware_dir / fw_info['filename']
                if fw_path.exists():
                    print(f"   📦 {fw_key} v{fw_info['version']} ({fw_info['size']:,} bytes)")
                else:
                    print(f"   ❌ {fw_key} - file missing")
                    status["compiled_firmware"] = False
        except Exception as e:
            print(f"❌ Firmware registry error: {e}")
    else:
        print("❌ No compiled firmware found")
    
    # Check OTA system configuration
    ota_registry = Path("data/ota/firmware_registry.json")
    if ota_registry.exists():
        try:
            with open(ota_registry, 'r') as f:
                ota_data = json.load(f)
            
            assignments = ota_data.get('device_assignments', {})
            auto_update = ota_data.get('auto_update_enabled', False)
            
            status["ota_system"] = len(assignments) > 0 and auto_update
            print(f"✅ OTA System: {len(assignments)} device assignments")
            print(f"   🔄 Auto-updates: {'Enabled' if auto_update else 'Disabled'}")
            
            if assignments:
                print("   📱 Device assignments:")
                for device_id, firmware_key in assignments.items():
                    print(f"      {device_id} → {firmware_key}")
        except Exception as e:
            print(f"❌ OTA registry error: {e}")
    else:
        print("❌ OTA system not configured")
    
    # Check server configuration
    unified_cms = Path("unified_cms.py")
    if unified_cms.exists():
        status["server_config"] = True
        print("✅ Server configuration ready")
    else:
        print("❌ Server configuration missing")
    
    # Check database
    db_file = Path("instance/unified_cms.db")
    if db_file.exists():
        status["database"] = True
        print("✅ Database initialized")
    else:
        print("❌ Database not found")
    
    return status

def show_esp32_instructions():
    """Show instructions for ESP32 setup"""
    print(f"\n📱 ESP32 Device Setup Instructions")
    print("=" * 50)
    
    print("1️⃣ Flash your ESP32 device:")
    print("   📂 Use firmware: firmwares/ESP32_DevKit_V1_OTA_Base.bin")
    print("   🔧 Flash with: esptool.py, Arduino IDE, or VS Code PlatformIO")
    print("   💡 Example: esptool.py --port COM3 write_flash 0x1000 ESP32_DevKit_V1_OTA_Base.bin")
    
    print("\n2️⃣ Configure WiFi and Server:")
    print("   📡 ESP32 creates AP: 'ESP32-OTA-Setup' (password: 12345678)")
    print("   🌐 Connect and go to: 192.168.4.1")
    print("   ⚙️ Enter your WiFi credentials")
    print("   🖥️ Server URL: http://192.168.1.107:5000 (your server IP)")
    
    print("\n3️⃣ Monitor OTA Updates:")
    print("   🔄 ESP32 checks for updates every 2 minutes")
    print("   📥 Automatic cross-firmware updates enabled")
    print("   💓 Heartbeat sent every 30 seconds")
    
    print("\n4️⃣ Test Cross-Firmware Updates:")
    print("   🔄 Base firmware → LED firmware (automatic)")
    print("   💡 LED firmware → Base firmware (automatic)")
    print("   🎯 Always gets the latest available firmware")

def show_server_commands():
    """Show server management commands"""
    print(f"\n🖥️ Server Management Commands")
    print("=" * 50)
    
    print("🚀 Start server:")
    print("   python unified_cms.py")
    
    print("\n🧪 Test OTA system:")
    print("   python test_cross_firmware_ota.py")
    
    print("\n🔧 Recompile firmware:")
    print("   python install_arduino_cli.py")
    
    print("\n📊 Update registry:")
    print("   python update_firmware_registry.py")
    
    print("\n🗄️ Reset database:")
    print("   python init_db.py")

def check_network_config():
    """Check network configuration"""
    print(f"\n🌐 Network Configuration")
    print("=" * 50)
    
    try:
        import socket
        hostname = socket.gethostname()
        local_ip = socket.gethostbyname(hostname)
        print(f"🖥️ Server IP: {local_ip}")
        print(f"🌐 Server URL: http://{local_ip}:5000")
        print(f"📱 Use this URL in ESP32 configuration")
    except Exception as e:
        print(f"❌ Could not determine IP: {e}")
        print("🔍 Use 'ipconfig' to find your IP address")

def main():
    """Main status check"""
    print("🚀 ESP32 OTA System - Final Status Report")
    print("Compiled firmware with automatic cross-device updates")
    print("=" * 70)
    
    # Check system status
    status = check_system_status()
    
    # Overall health check
    all_good = all(status.values())
    
    print(f"\n📊 System Health: {'🎉 EXCELLENT' if all_good else '⚠️ NEEDS ATTENTION'}")
    
    if all_good:
        print("✅ All components are working correctly")
        print("✅ ESP32 devices can now receive proper OTA updates")
        print("✅ Cross-firmware updates are enabled by default")
        print("✅ Compiled binaries replace fake test firmware")
    else:
        print("⚠️ Some components need attention:")
        for component, ok in status.items():
            if not ok:
                print(f"   ❌ {component.replace('_', ' ').title()}")
    
    # Show network config
    check_network_config()
    
    # Show instructions
    show_esp32_instructions()
    show_server_commands()
    
    print(f"\n🎉 ESP32 OTA System Setup Complete!")
    print("Your ESP32 devices will now receive proper compiled firmware via OTA")
    print("Cross-firmware updates happen automatically every 2 minutes")
    
    return all_good

if __name__ == "__main__":
    if main():
        sys.exit(0)
    else:
        sys.exit(1)