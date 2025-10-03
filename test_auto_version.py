#!/usr/bin/env python3
"""
Test script for automatic versioning system
"""

import requests
import json

def test_automatic_version_upload():
    """Test uploading firmware with automatic version generation"""
    
    # Test firmware file
    firmware_file = "firmwares/ESP32_DevKit_V1_LED_Blink_v2.2.bin"
    
    # Upload data
    upload_data = {
        'device_type': 'ESP32_LED_Blink',
        'description': 'Test auto-version upload',
        'auto_assign': 'true'
    }
    
    # Upload via API
    with open(firmware_file, 'rb') as f:
        files = {'firmware': f}
        response = requests.post(
            'http://localhost:5000/api/ota/upload',
            data=upload_data,
            files=files
        )
    
    print(f"Upload Response Status: {response.status_code}")
    print(f"Upload Response: {response.text}")
    
    if response.status_code == 200:
        result = response.json()
        print(f"✅ Auto-generated version: {result.get('version', 'Not found')}")
        print(f"✅ Upload successful: {result.get('success', False)}")
        return result.get('version')
    else:
        print(f"❌ Upload failed with status {response.status_code}")
        return None

def test_ota_check():
    """Test OTA check endpoint with different device types"""
    
    # Test with ESP32_LED_Blink
    check_data = {
        'device_id': 'TEST_DEVICE_001',
        'current_version': '2.0.0',
        'firmware_type': 'ESP32_LED_Blink'
    }
    
    response = requests.post(
        'http://localhost:5000/api/ota/check',
        headers={'X-Device-Type': 'ESP32_LED_Blink'},
        data=check_data
    )
    
    print(f"\nOTA Check Response Status: {response.status_code}")
    print(f"OTA Check Response: {response.text}")
    
    if response.status_code == 200:
        result = response.json()
        print(f"✅ Update available: {result.get('update_available', False)}")
        print(f"✅ Latest version: {result.get('latest_version', 'Not found')}")
    
if __name__ == '__main__':
    print("🧪 Testing Automatic Versioning System")
    print("=" * 50)
    
    # Test upload
    version = test_automatic_version_upload()
    
    if version:
        print(f"\n📦 Uploaded version: {version}")
        
        # Test OTA check
        test_ota_check()
    
    print("\n✅ Testing complete!")