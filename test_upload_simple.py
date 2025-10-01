#!/usr/bin/env python3
"""
Simple firmware upload test script
Tests the OTA upload endpoint with minimal complexity
"""

import requests
import os

def test_upload():
    # Server URL - use localhost
    server_url = "http://127.0.0.1:5000"
    upload_endpoint = f"{server_url}/ota-upload"
    
    # Find a firmware file to upload
    firmware_files = [
        "custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_PersonalCMS/bin/ESP32_DevKit_V1_PersonalCMS_v2.0.bin",
        "custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_OTA_Base/bin/ESP32_DevKit_V1_OTA_Base_v2.0.bin",
        "custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_LED_Blink/bin/ESP32_DevKit_V1_LED_Blink_v2.0.bin"
    ]
    
    firmware_file = None
    for fw_path in firmware_files:
        if os.path.exists(fw_path):
            firmware_file = fw_path
            break
    
    if not firmware_file:
        print("❌ No firmware file found!")
        print("Available files:")
        for fw_path in firmware_files:
            exists = "✓" if os.path.exists(fw_path) else "✗"
            print(f"  {exists} {fw_path}")
        return False
    
    print(f"📁 Using firmware file: {firmware_file}")
    print(f"📊 File size: {os.path.getsize(firmware_file)} bytes")
    
    # Prepare upload data
    upload_data = {
        'version': '2.0',
        'device_type': 'ESP32_OTA_Base',
        'description': 'Test upload from script'
    }
    
    try:
        # Test server connectivity first
        print(f"🔗 Testing server connectivity to {server_url}...")
        response = requests.get(server_url, timeout=5)
        print(f"✓ Server responding: {response.status_code}")
        
        # Now try the upload
        print(f"📤 Uploading to {upload_endpoint}...")
        
        with open(firmware_file, 'rb') as fw_file:
            files = {'firmware': fw_file}
            
            response = requests.post(
                upload_endpoint,
                data=upload_data,
                files=files,
                timeout=30
            )
        
        print(f"📨 Upload response: {response.status_code}")
        print(f"📄 Response content: {response.text[:500]}...")
        
        if response.status_code == 200:
            print("✅ Upload successful!")
            return True
        else:
            print(f"❌ Upload failed with status {response.status_code}")
            return False
            
    except requests.exceptions.ConnectionError as e:
        print(f"❌ Connection error: {e}")
        print("💡 Make sure the server is running at http://192.168.1.107:5000")
        return False
    except Exception as e:
        print(f"❌ Upload error: {e}")
        return False

if __name__ == "__main__":
    print("🚀 Testing firmware upload...")
    test_upload()