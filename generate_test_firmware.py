#!/usr/bin/env python3
"""
Generate realistic firmware binary files for OTA testing
Creates properly sized binary files that can be used for OTA update testing
"""

import os
import json
from datetime import datetime

def create_firmware_binary(filename, size, description):
    """Create a realistic firmware binary file"""
    print(f"Creating {filename} ({size:,} bytes)...")
    
    # Create binary header (common in ESP32 firmware)
    header = bytearray([
        0xE9, 0x00, 0x00, 0x02,  # ESP32 image header
        0x00, 0x00, 0x00, 0x00,  # Entry point
        0x00, 0x00, 0x00, 0x00,  # Load address
        0x20, 0x00, 0x00, 0x00,  # Segment count
    ])
    
    # Add description string
    desc_bytes = f"ESP32_FIRMWARE:{description}:OTA_ENABLED:v1.0.0".encode('utf-8')
    header.extend(desc_bytes)
    header.extend(b'\x00' * (64 - len(desc_bytes)))  # Pad to 64 bytes
    
    # Create pseudo-code sections (representing compiled Arduino code)
    code_patterns = [
        # WiFi setup code pattern
        bytes([0x41, 0x50, 0x20, 0x69, 0x6E, 0x69, 0x74] * 100),  # "AP init" pattern
        
        # OTA code pattern  
        bytes([0x4F, 0x54, 0x41, 0x20, 0x63, 0x68, 0x65, 0x63, 0x6B] * 100),  # "OTA check" pattern
        
        # HTTP client pattern
        bytes([0x48, 0x54, 0x54, 0x50, 0x20, 0x47, 0x45, 0x54] * 100),  # "HTTP GET" pattern
        
        # JSON parsing pattern
        bytes([0x7B, 0x22, 0x75, 0x70, 0x64, 0x61, 0x74, 0x65] * 100),  # JSON pattern
    ]
    
    # Create the binary file
    with open(f"firmware_binaries/{filename}", 'wb') as f:
        # Write header
        f.write(header)
        
        # Write code sections
        remaining = size - len(header)
        pattern_index = 0
        
        while remaining > 0:
            pattern = code_patterns[pattern_index % len(code_patterns)]
            write_size = min(len(pattern), remaining)
            f.write(pattern[:write_size])
            remaining -= write_size
            pattern_index += 1
            
            # Add some random-looking data (simulating compiled code)
            if remaining > 1024:
                padding = bytearray(1024)
                for i in range(0, 1024, 4):
                    # Create pseudo-ARM instructions
                    padding[i:i+4] = (i + pattern_index * 0x1000).to_bytes(4, 'little')
                f.write(padding)
                remaining -= 1024

def create_realistic_firmware():
    """Create realistic firmware binaries for testing"""
    print("🔨 Creating realistic firmware binaries for OTA testing...")
    print("=" * 60)
    
    os.makedirs("firmware_binaries", exist_ok=True)
    
    # Define firmware configurations
    firmware_configs = [
        {
            "filename": "ESP32_PersonalCMS_Full.bin",
            "size": 1200000,  # 1.2MB
            "description": "PersonalCMS_with_Display_Sensors_OTA"
        },
        {
            "filename": "ESP32_OTA_Base.bin", 
            "size": 800000,   # 800KB
            "description": "Minimal_OTA_WiFi_Base"
        },
        {
            "filename": "ESP32_LED_Blink.bin",
            "size": 700000,   # 700KB  
            "description": "LED_Patterns_Demo_OTA"
        }
    ]
    
    # Create each firmware binary
    for config in firmware_configs:
        create_firmware_binary(
            config["filename"],
            config["size"], 
            config["description"]
        )
        
        # Update the JSON metadata with actual file size
        json_filename = config["filename"].replace('.bin', '.json')
        json_path = f"firmware_binaries/{json_filename}"
        
        if os.path.exists(json_path):
            with open(json_path, 'r') as f:
                metadata = json.load(f)
            
            # Update with actual file size and date
            actual_size = os.path.getsize(f"firmware_binaries/{config['filename']}")
            metadata["size"] = actual_size
            metadata["build_date"] = datetime.now().isoformat()
            metadata["sha256"] = "placeholder_hash_" + config["filename"].replace('.bin', '')
            
            with open(json_path, 'w') as f:
                json.dump(metadata, f, indent=2)
                
            print(f"   ✅ Updated {json_filename} metadata")
    
    print(f"\n🎯 Firmware Generation Complete!")
    print(f"   📁 Location: firmware_binaries/")
    print(f"   🌐 Upload via: http://localhost:5000/ota-management")
    
    # List created files
    print(f"\n📋 Created Files:")
    for file in os.listdir("firmware_binaries"):
        if file.endswith(('.bin', '.json')):
            size = os.path.getsize(f"firmware_binaries/{file}")
            print(f"   📄 {file:<30} {size:>10,} bytes")

if __name__ == "__main__":
    create_realistic_firmware()