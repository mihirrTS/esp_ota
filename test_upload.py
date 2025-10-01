import requests
import os

# Test firmware upload
def test_upload():
    url = "http://192.168.1.107:5000/ota-upload"
    
    # Use one of our created firmware files
    firmware_path = "custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_OTA_Base/bin/ESP32_DevKit_V1_OTA_Base_v2.0.bin"
    
    if not os.path.exists(firmware_path):
        print(f"❌ Firmware file not found: {firmware_path}")
        return
    
    # Prepare the upload data
    files = {
        'firmware': open(firmware_path, 'rb')
    }
    
    data = {
        'version': '2.0',
        'device_type': 'ESP32_OTA_Base',
        'description': 'Test upload from script',
        'auto_assign': 'on'
    }
    
    try:
        print(f"🔄 Uploading {firmware_path}...")
        response = requests.post(url, files=files, data=data)
        
        print(f"Status Code: {response.status_code}")
        print(f"Response Headers: {dict(response.headers)}")
        
        if response.status_code == 200:
            print("✅ Upload successful!")
        elif response.status_code == 302:
            print("🔄 Redirect (probably successful)")
            print(f"Redirect Location: {response.headers.get('Location', 'Unknown')}")
        else:
            print(f"❌ Upload failed with status {response.status_code}")
            print(f"Response: {response.text[:500]}...")
            
    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        files['firmware'].close()

if __name__ == "__main__":
    test_upload()