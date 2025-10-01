#!/usr/bin/env python3
"""
Test script to verify PersonalCMS dashboard generation and BMP functionality
"""

import requests
import json
import os

def test_personalcms_functionality():
    server_url = "http://127.0.0.1:5000"
    device_id = "ESP32_OTA_BASE_001"
    
    print("🧪 Testing PersonalCMS Dashboard & BMP Generation")
    print("=" * 60)
    
    # Test 1: Device registration
    print("\n1️⃣ Testing device registration...")
    register_data = {
        "device_id": device_id,
        "device_name": "Test ESP32 PersonalCMS Base",
        "occupation": "Personal CMS Device", 
        "device_type": "ESP32_DevKit_V1",
        "firmware_version": "2.0.0",
        "capabilities": "dashboard,images,ota,bmp_download"
    }
    
    try:
        response = requests.post(f"{server_url}/api/devices/register", 
                               json=register_data, timeout=10)
        print(f"   📡 Registration response: {response.status_code}")
        if response.status_code in [200, 201]:
            print("   ✅ Device registration successful")
        else:
            print(f"   ⚠️ Registration response: {response.text[:200]}")
    except Exception as e:
        print(f"   ❌ Registration failed: {e}")
    
    # Test 2: Content endpoint
    print("\n2️⃣ Testing content endpoint...")
    try:
        response = requests.get(f"{server_url}/api/devices/{device_id}/content", timeout=10)
        print(f"   📊 Content response: {response.status_code}")
        
        if response.status_code == 200:
            content = response.json()
            print("   ✅ Content endpoint working")
            print(f"   📊 Dashboard URL: {content.get('dashboard_url', 'None')}")
            print(f"   🖼️ Images available: {len(content.get('images', []))}")
            
            # Test dashboard BMP access
            dashboard_url = content.get('dashboard_url')
            if dashboard_url:
                full_dashboard_url = f"{server_url}{dashboard_url}"
                print(f"\n3️⃣ Testing dashboard BMP access: {full_dashboard_url}")
                
                try:
                    bmp_response = requests.head(full_dashboard_url, timeout=10)
                    print(f"   📊 Dashboard BMP response: {bmp_response.status_code}")
                    
                    if bmp_response.status_code == 200:
                        print("   ✅ Dashboard BMP accessible")
                        content_length = bmp_response.headers.get('Content-Length', 'Unknown')
                        print(f"   📏 Dashboard BMP size: {content_length} bytes")
                    else:
                        print("   ❌ Dashboard BMP not accessible")
                except Exception as e:
                    print(f"   ❌ Dashboard BMP test failed: {e}")
            
            # Test image BMP access
            images = content.get('images', [])
            if images:
                first_image = images[0]
                bmp_url = first_image.get('bmp_url')
                if bmp_url:
                    full_bmp_url = f"{server_url}{bmp_url}"
                    print(f"\n4️⃣ Testing image BMP access: {full_bmp_url}")
                    
                    try:
                        img_response = requests.head(full_bmp_url, timeout=10)
                        print(f"   🖼️ Image BMP response: {img_response.status_code}")
                        
                        if img_response.status_code == 200:
                            print("   ✅ Image BMP accessible")
                            content_length = img_response.headers.get('Content-Length', 'Unknown')
                            print(f"   📏 Image BMP size: {content_length} bytes")
                        else:
                            print("   ❌ Image BMP not accessible")
                    except Exception as e:
                        print(f"   ❌ Image BMP test failed: {e}")
            
        else:
            print(f"   ❌ Content endpoint failed: {response.text[:200]}")
    except Exception as e:
        print(f"   ❌ Content endpoint test failed: {e}")
    
    # Test 3: Heartbeat endpoint
    print("\n5️⃣ Testing heartbeat endpoint...")
    heartbeat_data = {
        "device_id": device_id,
        "device_name": "Test ESP32 PersonalCMS Base",
        "occupation": "Personal CMS Device",
        "version": "2.0.0",
        "uptime": 300,
        "free_heap": 200000,
        "wifi_rssi": -65,
        "current_mode": "dashboard",
        "dashboard_loaded": True,
        "image_loaded": False
    }
    
    try:
        response = requests.post(f"{server_url}/api/devices/{device_id}/heartbeat",
                               json=heartbeat_data, timeout=10)
        print(f"   💓 Heartbeat response: {response.status_code}")
        if response.status_code == 200:
            print("   ✅ Heartbeat successful")
        else:
            print(f"   ⚠️ Heartbeat response: {response.text[:200]}")
    except Exception as e:
        print(f"   ❌ Heartbeat failed: {e}")
    
    print("\n" + "=" * 60)
    print("✅ PersonalCMS functionality test complete!")
    print("🚀 The enhanced ESP32 base code should now work with:")
    print("   📊 Dashboard BMP downloads")
    print("   🖼️ Image assignment and conversion")
    print("   🔄 Server-side BMP generation")
    print("   💓 Enhanced heartbeat with content status")

if __name__ == "__main__":
    test_personalcms_functionality()