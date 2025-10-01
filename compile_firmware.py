#!/usr/bin/env python3
"""
Firmware Compilation Script for ESP32 PersonalCMS
Compiles Arduino .ino files to .bin files for OTA updates
"""

import os
import subprocess
import shutil
import json
from datetime import datetime

def check_arduino_cli():
    """Check if Arduino CLI is installed"""
    try:
        result = subprocess.run(["arduino-cli", "version"], capture_output=True, text=True)
        if result.returncode == 0:
            print("✅ Arduino CLI found")
            return True
    except FileNotFoundError:
        print("❌ Arduino CLI not found!")
        print("📥 Installing Arduino CLI...")
        
        # Download and install Arduino CLI
        try:
            if os.name == 'nt':  # Windows
                # Download Arduino CLI for Windows
                import urllib.request
                url = "https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip"
                print(f"   Downloading from {url}")
                urllib.request.urlretrieve(url, "arduino-cli.zip")
                
                # Extract
                import zipfile
                with zipfile.ZipFile("arduino-cli.zip", 'r') as zip_ref:
                    zip_ref.extractall(".")
                
                os.remove("arduino-cli.zip")
                print("✅ Arduino CLI installed")
                return True
        except Exception as e:
            print(f"❌ Failed to install Arduino CLI: {e}")
            print("Please install manually from: https://arduino.github.io/arduino-cli/")
            return False

def setup_arduino_cli():
    """Setup Arduino CLI with ESP32 board package"""
    print("🔧 Setting up Arduino CLI...")
    
    commands = [
        ["arduino-cli", "config", "init"],
        ["arduino-cli", "core", "update-index"],
        ["arduino-cli", "core", "install", "esp32:esp32"],
        ["arduino-cli", "lib", "install", "ArduinoJson"],
        ["arduino-cli", "lib", "install", "WiFi"],
        ["arduino-cli", "lib", "install", "WebServer"],
        ["arduino-cli", "lib", "install", "HTTPClient"],
        ["arduino-cli", "lib", "install", "HTTPUpdate"],
        ["arduino-cli", "lib", "install", "Preferences"],
        ["arduino-cli", "lib", "install", "SPIFFS"],
    ]
    
    for cmd in commands:
        try:
            print(f"   Running: {' '.join(cmd)}")
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                print(f"   ⚠️  Warning: {' '.join(cmd)} failed")
                print(f"   Error: {result.stderr}")
        except Exception as e:
            print(f"   ⚠️  Warning: {e}")
    
    print("✅ Arduino CLI setup complete")

def compile_firmware(sketch_path, output_name, board="esp32:esp32:esp32"):
    """Compile Arduino sketch to binary"""
    print(f"\n🔨 Compiling {output_name}...")
    
    # Create output directory
    os.makedirs("firmware_binaries", exist_ok=True)
    
    # Build directory
    build_dir = f"build_{output_name}"
    os.makedirs(build_dir, exist_ok=True)
    
    try:
        # Compile command
        cmd = [
            "arduino-cli", "compile",
            "--fqbn", board,
            "--build-path", build_dir,
            "--output-dir", "firmware_binaries",
            sketch_path
        ]
        
        print(f"   Command: {' '.join(cmd)}")
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode == 0:
            # Find the .bin file
            sketch_name = os.path.basename(sketch_path).replace('.ino', '')
            bin_file = os.path.join(build_dir, f"{sketch_name}.ino.bin")
            
            if os.path.exists(bin_file):
                # Copy to firmware_binaries with proper name
                final_bin = os.path.join("firmware_binaries", f"{output_name}.bin")
                shutil.copy2(bin_file, final_bin)
                
                # Get file size
                size = os.path.getsize(final_bin)
                print(f"   ✅ Success: {final_bin} ({size:,} bytes)")
                
                # Create firmware info JSON
                info = {
                    "name": output_name,
                    "version": "1.0.0",
                    "description": f"ESP32 firmware: {output_name}",
                    "build_date": datetime.now().isoformat(),
                    "size": size,
                    "board": board,
                    "filename": f"{output_name}.bin"
                }
                
                info_file = os.path.join("firmware_binaries", f"{output_name}.json")
                with open(info_file, 'w') as f:
                    json.dump(info, f, indent=2)
                
                print(f"   📝 Info: {info_file}")
                return True
            else:
                print(f"   ❌ Binary file not found: {bin_file}")
                return False
        else:
            print(f"   ❌ Compilation failed:")
            print(f"   Error: {result.stderr}")
            return False
            
    except Exception as e:
        print(f"   ❌ Exception: {e}")
        return False
    finally:
        # Cleanup build directory
        if os.path.exists(build_dir):
            shutil.rmtree(build_dir, ignore_errors=True)

def main():
    """Main compilation process"""
    print("=" * 80)
    print("🚀 ESP32 PersonalCMS Firmware Compiler")
    print("=" * 80)
    
    # Check Arduino CLI
    if not check_arduino_cli():
        return 1
    
    # Setup Arduino CLI
    setup_arduino_cli()
    
    # Define firmware projects
    firmware_projects = [
        {
            "path": "custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_PersonalCMS.ino",
            "name": "ESP32_PersonalCMS_Full",
            "board": "esp32:esp32:esp32"
        },
        {
            "path": "custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_OTA_Base.ino",
            "name": "ESP32_OTA_Base",
            "board": "esp32:esp32:esp32"
        },
        {
            "path": "custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_LED_Blink.ino",
            "name": "ESP32_LED_Blink",
            "board": "esp32:esp32:esp32"
        }
    ]
    
    print(f"\n📦 Found {len(firmware_projects)} firmware projects to compile:")
    
    success_count = 0
    for project in firmware_projects:
        if os.path.exists(project["path"]):
            print(f"   📁 {project['name']}: {project['path']}")
            if compile_firmware(project["path"], project["name"], project["board"]):
                success_count += 1
        else:
            print(f"   ❌ Missing: {project['path']}")
    
    print(f"\n🎯 Compilation Summary:")
    print(f"   ✅ Successful: {success_count}")
    print(f"   ❌ Failed: {len(firmware_projects) - success_count}")
    
    if success_count > 0:
        print(f"\n📁 Binary files available in: firmware_binaries/")
        print(f"   Ready for OTA deployment!")
        
        # List created files
        if os.path.exists("firmware_binaries"):
            print(f"\n📋 Created files:")
            for file in os.listdir("firmware_binaries"):
                if file.endswith(('.bin', '.json')):
                    size = os.path.getsize(os.path.join("firmware_binaries", file))
                    print(f"   📄 {file} ({size:,} bytes)")
    
    return 0 if success_count > 0 else 1

if __name__ == "__main__":
    exit(main())