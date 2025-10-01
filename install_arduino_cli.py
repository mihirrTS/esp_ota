#!/usr/bin/env python3
"""
Arduino CLI Installation and ESP32 Setup Script
Installs Arduino CLI, ESP32 core, and compiles firmware binaries
"""

import os
import sys
import subprocess
import urllib.request
import zipfile
import json
import shutil
from pathlib import Path

def run_command(cmd, cwd=None, shell=True):
    """Run a command and return success status"""
    try:
        print(f"🔧 Running: {cmd}")
        result = subprocess.run(cmd, cwd=cwd, shell=shell, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"✅ Command successful")
            if result.stdout.strip():
                print(f"   Output: {result.stdout.strip()}")
            return True, result.stdout
        else:
            print(f"❌ Command failed: {result.stderr.strip()}")
            return False, result.stderr
    except Exception as e:
        print(f"❌ Command exception: {e}")
        return False, str(e)

def download_file(url, destination):
    """Download a file from URL"""
    try:
        print(f"📥 Downloading: {url}")
        urllib.request.urlretrieve(url, destination)
        print(f"✅ Downloaded to: {destination}")
        return True
    except Exception as e:
        print(f"❌ Download failed: {e}")
        return False

def install_arduino_cli():
    """Install Arduino CLI for Windows"""
    print("🔧 Installing Arduino CLI...")
    
    # Create tools directory
    tools_dir = Path("tools")
    tools_dir.mkdir(exist_ok=True)
    
    arduino_cli_dir = tools_dir / "arduino-cli"
    arduino_cli_exe = arduino_cli_dir / "arduino-cli.exe"
    
    if arduino_cli_exe.exists():
        print("✅ Arduino CLI already installed")
        return str(arduino_cli_exe)
    
    # Download Arduino CLI
    arduino_cli_url = "https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip"
    zip_file = tools_dir / "arduino-cli.zip"
    
    if not download_file(arduino_cli_url, zip_file):
        return None
    
    # Extract Arduino CLI
    try:
        with zipfile.ZipFile(zip_file, 'r') as zip_ref:
            zip_ref.extractall(arduino_cli_dir)
        
        os.remove(zip_file)
        print("✅ Arduino CLI installed successfully")
        return str(arduino_cli_exe)
    except Exception as e:
        print(f"❌ Failed to extract Arduino CLI: {e}")
        return None

def setup_arduino_environment(arduino_cli_path):
    """Setup Arduino environment with ESP32 core and libraries"""
    print("🔧 Setting up Arduino environment...")
    
    # Initialize Arduino CLI config
    success, _ = run_command(f'"{arduino_cli_path}" config init --overwrite')
    if not success:
        print("❌ Failed to initialize Arduino CLI config")
        return False
    
    # Update core index
    success, _ = run_command(f'"{arduino_cli_path}" core update-index')
    if not success:
        print("❌ Failed to update core index")
        return False
    
    # Add ESP32 board manager URL
    esp32_url = "https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json"
    success, _ = run_command(f'"{arduino_cli_path}" config add board_manager.additional_urls {esp32_url}')
    if not success:
        print("❌ Failed to add ESP32 board manager URL")
        return False
    
    # Update index with ESP32
    success, _ = run_command(f'"{arduino_cli_path}" core update-index')
    if not success:
        print("❌ Failed to update index with ESP32")
        return False
    
    # Install ESP32 core (this may take a while)
    print("📦 Installing ESP32 core (this may take several minutes)...")
    success, _ = run_command(f'"{arduino_cli_path}" core install esp32:esp32')
    if not success:
        print("❌ Failed to install ESP32 core")
        return False
    
    # Install required libraries
    libraries = [
        "ArduinoJson",
        "HTTPClient",
        "WiFi",
        "WebServer",
        "DNSServer",
        "Preferences",
        "HTTPUpdate"
    ]
    
    for lib in libraries:
        print(f"📦 Installing library: {lib}")
        success, _ = run_command(f'"{arduino_cli_path}" lib install "{lib}"')
        # Some libraries might already be built-in, so we continue even if this fails
    
    print("✅ Arduino environment setup complete")
    return True

def compile_esp32_firmware(arduino_cli_path):
    """Compile ESP32 firmware files"""
    print("🔧 Compiling ESP32 firmware...")
    
    # Firmware configurations
    firmwares = [
        {
            "name": "ESP32_DevKit_V1_OTA_Base",
            "path": "custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_OTA_Base/ESP32_DevKit_V1_OTA_Base.ino",
            "version": "2.0.0",
            "description": "PersonalCMS Base Firmware with OTA"
        },
        {
            "name": "ESP32_DevKit_V1_LED_Blink",
            "path": "custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_LED_Blink/ESP32_DevKit_V1_LED_Blink.ino",
            "version": "2.1.0",
            "description": "LED Patterns Firmware with OTA"
        }
    ]
    
    # Create firmwares directory
    fw_dir = Path("firmwares")
    fw_dir.mkdir(exist_ok=True)
    
    firmware_registry = {}
    
    for fw in firmwares:
        print(f"\n🔧 Compiling {fw['name']}...")
        
        sketch_path = Path(fw["path"])
        if not sketch_path.exists():
            print(f"❌ Sketch not found: {sketch_path}")
            continue
        
        # Create build directory
        build_dir = fw_dir / f"{fw['name']}_build"
        build_dir.mkdir(exist_ok=True)
        
        # Compile command
        compile_cmd = f'"{arduino_cli_path}" compile --fqbn esp32:esp32:esp32 --output-dir "{build_dir}" "{sketch_path}"'
        
        success, output = run_command(compile_cmd)
        
        if success:
            # Find the compiled binary
            bin_files = list(build_dir.glob("*.bin"))
            if bin_files:
                src_bin = bin_files[0]
                dest_bin = fw_dir / f"{fw['name']}.bin"
                
                # Copy binary to firmwares directory
                shutil.copy2(src_bin, dest_bin)
                
                # Calculate file size
                file_size = dest_bin.stat().st_size
                
                print(f"✅ Compiled successfully: {dest_bin} ({file_size} bytes)")
                
                # Add to registry
                firmware_registry[fw['name']] = {
                    "version": fw["version"],
                    "description": fw["description"],
                    "filename": f"{fw['name']}.bin",
                    "size": file_size,
                    "compatible_devices": ["ESP32_DevKit_V1"],
                    "build_date": "2024-01-01T00:00:00Z"
                }
            else:
                print(f"❌ No binary file found in {build_dir}")
        else:
            print(f"❌ Compilation failed for {fw['name']}")
    
    # Save firmware registry
    registry_file = fw_dir / "firmware_registry.json"
    with open(registry_file, 'w') as f:
        json.dump(firmware_registry, f, indent=2)
    
    print(f"\n✅ Firmware registry saved: {registry_file}")
    print(f"📦 Compiled {len(firmware_registry)} firmware(s)")
    
    return len(firmware_registry) > 0

def update_server_firmware_paths():
    """Update server to use new firmware paths"""
    print("🔧 Updating server firmware configuration...")
    
    # Update unified_cms.py to use the new firmware directory
    unified_cms_file = Path("unified_cms.py")
    if unified_cms_file.exists():
        try:
            content = unified_cms_file.read_text()
            
            # Replace firmware paths
            content = content.replace(
                'FIRMWARE_DIR = "firmware"',
                'FIRMWARE_DIR = "firmwares"'
            )
            
            unified_cms_file.write_text(content)
            print("✅ Updated unified_cms.py firmware directory")
        except Exception as e:
            print(f"❌ Failed to update unified_cms.py: {e}")
    
    print("✅ Server firmware configuration updated")

def main():
    """Main installation and compilation process"""
    print("🚀 ESP32 Arduino CLI Installation and Firmware Compilation")
    print("=" * 70)
    
    # Change to script directory
    script_dir = Path(__file__).parent
    os.chdir(script_dir)
    
    # Install Arduino CLI
    arduino_cli_path = install_arduino_cli()
    if not arduino_cli_path:
        print("❌ Failed to install Arduino CLI")
        return False
    
    # Setup Arduino environment
    if not setup_arduino_environment(arduino_cli_path):
        print("❌ Failed to setup Arduino environment")
        return False
    
    # Compile firmware
    if not compile_esp32_firmware(arduino_cli_path):
        print("❌ Failed to compile firmware")
        return False
    
    # Update server configuration
    update_server_firmware_paths()
    
    print("\n🎉 Installation and compilation completed successfully!")
    print("✅ Arduino CLI installed and configured")
    print("✅ ESP32 core and libraries installed")
    print("✅ Firmware binaries compiled")
    print("✅ Server configuration updated")
    print("\nYou can now use the OTA system with properly compiled binaries!")
    
    return True

if __name__ == "__main__":
    if main():
        sys.exit(0)
    else:
        sys.exit(1)