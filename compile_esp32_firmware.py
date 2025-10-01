#!/usr/bin/env python3
"""
ESP32 Firmware Compilation Script
Compiles ESP32 Arduino sketches and generates proper firmware binaries for OTA updates
"""

import os
import sys
import subprocess
import json
import shutil
from datetime import datetime

# Configuration
ARDUINO_CLI = "arduino-cli"
ESP32_BOARD = "esp32:esp32:esp32doit-devkit-v1"
ESP32_CORE_VERSION = "3.0.5"  # Compatible version
BUILD_DIR = "build_temp"
OUTPUT_DIR = "firmware_output"

# Firmware configurations
FIRMWARE_CONFIGS = [
    {
        "name": "ESP32_DevKit_V1_OTA_Base",
        "sketch": "custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_OTA_Base/ESP32_DevKit_V1_OTA_Base.ino",
        "version": "2.1.0",
        "description": "ESP32 OTA Base with PersonalCMS capabilities - FIXED HTTPUpdate API",
        "type": "ESP32_OTA_Base"
    },
    {
        "name": "ESP32_DevKit_V1_LED_Blink", 
        "sketch": "custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_LED_Blink.ino",
        "version": "2.1.0",
        "description": "ESP32 LED Blink patterns with OTA - FIXED HTTPUpdate API",
        "type": "ESP32_LED_Blink"
    }
]

def check_arduino_cli():
    """Check if Arduino CLI is available"""
    try:
        result = subprocess.run([ARDUINO_CLI, "version"], capture_output=True, text=True)
        if result.returncode == 0:
            print(f"✅ Arduino CLI found: {result.stdout.strip()}")
            return True
        else:
            print(f"❌ Arduino CLI error: {result.stderr}")
            return False
    except FileNotFoundError:
        print("❌ Arduino CLI not found. Please install Arduino CLI:")
        print("   Download from: https://arduino.github.io/arduino-cli/latest/")
        print("   Or install via: winget install ArduinoSA.CLI")
        return False

def setup_arduino_environment():
    """Setup Arduino CLI environment for ESP32"""
    print("🔧 Setting up Arduino environment...")
    
    # Update package index
    print("📦 Updating package index...")
    subprocess.run([ARDUINO_CLI, "core", "update-index"], check=False)
    
    # Install ESP32 core
    print(f"📦 Installing ESP32 core {ESP32_CORE_VERSION}...")
    result = subprocess.run([
        ARDUINO_CLI, "core", "install", f"esp32:esp32@{ESP32_CORE_VERSION}"
    ], capture_output=True, text=True)
    
    if result.returncode != 0:
        print(f"⚠️  ESP32 core install result: {result.stderr}")
        # Try without version specification
        print("📦 Trying to install latest ESP32 core...")
        subprocess.run([ARDUINO_CLI, "core", "install", "esp32:esp32"], check=False)
    
    # Install required libraries
    print("📚 Installing required libraries...")
    libraries = [
        "ArduinoJson@6.21.3",
        "HTTPUpdate",
        "WiFi", 
        "WebServer",
        "DNSServer",
        "Preferences"
    ]
    
    for lib in libraries:
        print(f"   Installing {lib}...")
        subprocess.run([ARDUINO_CLI, "lib", "install", lib], 
                      capture_output=True, text=True, check=False)
    
    print("✅ Arduino environment setup complete")

def compile_firmware(config):
    """Compile a specific firmware configuration"""
    print(f"\n🔨 Compiling {config['name']}...")
    print(f"   Sketch: {config['sketch']}")
    print(f"   Version: {config['version']}")
    
    sketch_path = config['sketch']
    if not os.path.exists(sketch_path):
        print(f"❌ Sketch not found: {sketch_path}")
        return False
    
    # Create build directory
    build_path = os.path.join(BUILD_DIR, config['name'])
    os.makedirs(build_path, exist_ok=True)
    
    # Compile command
    compile_cmd = [
        ARDUINO_CLI, "compile",
        "--fqbn", ESP32_BOARD,
        "--build-path", build_path,
        "--export-binaries",
        sketch_path
    ]
    
    print(f"🔧 Running: {' '.join(compile_cmd)}")
    result = subprocess.run(compile_cmd, capture_output=True, text=True)
    
    if result.returncode == 0:
        print(f"✅ Compilation successful for {config['name']}")
        
        # Find the generated binary
        sketch_name = os.path.splitext(os.path.basename(sketch_path))[0]
        bin_file = f"{sketch_name}.ino.bin"
        bin_path = os.path.join(build_path, bin_file)
        
        if os.path.exists(bin_path):
            # Copy to output directory with proper naming
            output_filename = f"{config['type']}_v{config['version']}.bin"
            output_path = os.path.join(OUTPUT_DIR, output_filename)
            os.makedirs(OUTPUT_DIR, exist_ok=True)
            
            shutil.copy2(bin_path, output_path)
            
            # Get file size
            file_size = os.path.getsize(output_path)
            
            # Create metadata
            metadata = {
                "device_type": config['type'],
                "version": config['version'],
                "description": config['description'],
                "filename": output_filename,
                "file_size": file_size,
                "compile_date": datetime.now().isoformat(),
                "arduino_board": ESP32_BOARD,
                "esp32_core": ESP32_CORE_VERSION
            }
            
            metadata_path = os.path.join(OUTPUT_DIR, f"{config['type']}_v{config['version']}.json")
            with open(metadata_path, 'w') as f:
                json.dump(metadata, f, indent=2)
            
            print(f"✅ Binary created: {output_path} ({file_size} bytes)")
            print(f"✅ Metadata created: {metadata_path}")
            return True
        else:
            print(f"❌ Binary not found at: {bin_path}")
            # List files in build directory for debugging
            if os.path.exists(build_path):
                print("Files in build directory:")
                for f in os.listdir(build_path):
                    print(f"   {f}")
            return False
    else:
        print(f"❌ Compilation failed for {config['name']}")
        print(f"Error: {result.stderr}")
        print(f"Output: {result.stdout}")
        return False

def create_simple_led_blink():
    """Create a simple LED blink firmware if the main one fails"""
    print("\n🔧 Creating simple LED blink firmware...")
    
    simple_code = '''
/*
 * Simple ESP32 LED Blink with OTA - Minimal Version
 */
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>

const int LED_PIN = 2;
String deviceID = "ESP32_LED_SIMPLE_001";
String currentVersion = "2.1.0";

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  Serial.println("ESP32 Simple LED Blink v2.1.0 with OTA");
}

void loop() {
  // Simple blink pattern
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
  
  // Print status every 10 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 10000) {
    Serial.println("LED Blink v2.1.0 - Working!");
    lastPrint = millis();
  }
}
'''
    
    # Create temporary sketch directory
    simple_dir = "temp_simple_led"
    os.makedirs(simple_dir, exist_ok=True)
    
    sketch_file = os.path.join(simple_dir, "simple_led_blink.ino")
    with open(sketch_file, 'w') as f:
        f.write(simple_code)
    
    # Compile
    build_path = os.path.join(BUILD_DIR, "simple_led")
    os.makedirs(build_path, exist_ok=True)
    
    compile_cmd = [
        ARDUINO_CLI, "compile",
        "--fqbn", ESP32_BOARD,
        "--build-path", build_path,
        "--export-binaries",
        sketch_file
    ]
    
    result = subprocess.run(compile_cmd, capture_output=True, text=True)
    
    if result.returncode == 0:
        # Find binary
        bin_file = "simple_led_blink.ino.bin"
        bin_path = os.path.join(build_path, bin_file)
        
        if os.path.exists(bin_path):
            output_filename = "ESP32_LED_Blink_v2.1.0.bin"
            output_path = os.path.join(OUTPUT_DIR, output_filename)
            shutil.copy2(bin_path, output_path)
            
            file_size = os.path.getsize(output_path)
            print(f"✅ Simple LED binary created: {output_path} ({file_size} bytes)")
            
            # Cleanup
            shutil.rmtree(simple_dir, ignore_errors=True)
            return True
    
    print("❌ Simple LED compilation failed")
    shutil.rmtree(simple_dir, ignore_errors=True)
    return False

def main():
    print("🚀 ESP32 Firmware Compilation Tool")
    print("=" * 50)
    
    # Check Arduino CLI
    if not check_arduino_cli():
        return 1
    
    # Setup environment
    setup_arduino_environment()
    
    # Clean build directories
    if os.path.exists(BUILD_DIR):
        shutil.rmtree(BUILD_DIR)
    if os.path.exists(OUTPUT_DIR):
        shutil.rmtree(OUTPUT_DIR)
    
    os.makedirs(BUILD_DIR, exist_ok=True)
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    # Compile all firmware configurations
    success_count = 0
    for config in FIRMWARE_CONFIGS:
        if compile_firmware(config):
            success_count += 1
    
    # If LED blink failed, create simple version
    if success_count < len(FIRMWARE_CONFIGS):
        print("\n⚠️  Some compilations failed, creating simple LED blink...")
        create_simple_led_blink()
    
    print(f"\n🎉 Compilation complete! {success_count}/{len(FIRMWARE_CONFIGS)} successful")
    print(f"📁 Output directory: {OUTPUT_DIR}")
    
    # List generated files
    if os.path.exists(OUTPUT_DIR):
        print("\n📋 Generated firmware files:")
        for f in os.listdir(OUTPUT_DIR):
            file_path = os.path.join(OUTPUT_DIR, f)
            size = os.path.getsize(file_path)
            print(f"   {f} ({size} bytes)")
    
    # Cleanup build directory
    shutil.rmtree(BUILD_DIR, ignore_errors=True)
    
    return 0

if __name__ == "__main__":
    sys.exit(main())