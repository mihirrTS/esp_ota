"""
OTA (Over-The-Air) Update Management for ESP32 Devices
Handles firmware upload, version management, and update distribution
"""

import os
import json
import hashlib
from datetime import datetime
from werkzeug.utils import secure_filename
from flask import current_app
import logging

logger = logging.getLogger(__name__)

class OTAManager:
    def __init__(self, upload_folder='data/ota'):
        self.upload_folder = upload_folder
        self.firmware_folder = os.path.join(upload_folder, 'firmware')
        self.metadata_file = os.path.join(upload_folder, 'firmware_registry.json')
        
        # Support for compiled firmware directory
        self.compiled_firmware_folder = 'firmwares'
        self.compiled_metadata_file = os.path.join(self.compiled_firmware_folder, 'firmware_registry.json')
        
        # Ensure directories exist
        os.makedirs(self.upload_folder, exist_ok=True)
        os.makedirs(self.firmware_folder, exist_ok=True)
        
        # Load or create firmware registry (merged from both sources)
        self.firmware_registry = self._load_registry()
        
        # Store forced updates for devices (device_id -> firmware_info)
        self.forced_updates = {}

    def _load_registry(self):
        """Load firmware registry from JSON file, merging compiled and uploaded firmware"""
        registry = {
            'firmware_versions': {},
            'device_assignments': {},
            'auto_update_enabled': False
        }
        
        # Load uploaded firmware registry
        if os.path.exists(self.metadata_file):
            try:
                with open(self.metadata_file, 'r') as f:
                    uploaded_registry = json.load(f)
                    registry.update(uploaded_registry)
            except Exception as e:
                logger.error(f"Failed to load uploaded firmware registry: {e}")
        
        # Load and merge compiled firmware registry
        if os.path.exists(self.compiled_metadata_file):
            try:
                with open(self.compiled_metadata_file, 'r') as f:
                    compiled_registry = json.load(f)
                    
                # Merge compiled firmware into registry
                for firmware_key, firmware_info in compiled_registry.items():
                    # Convert compiled format to our registry format
                    registry_key = f"compiled_{firmware_key}"
                    registry['firmware_versions'][registry_key] = {
                        'filename': firmware_info['filename'],
                        'version': firmware_info['version'],
                        'device_type': firmware_info['compatible_devices'][0] if firmware_info['compatible_devices'] else 'ESP32',
                        'description': firmware_info['description'],
                        'file_size': firmware_info['size'],
                        'upload_date': firmware_info['build_date'],
                        'file_hash': '',  # We'll calculate this when needed
                        'download_count': 0,  # Initialize download count for compiled firmware
                        'is_compiled': True,
                        'source_path': os.path.join(self.compiled_firmware_folder, firmware_info['filename']),
                        'filepath': os.path.join(self.compiled_firmware_folder, firmware_info['filename']),  # For compatibility
                        'is_active': True  # Compiled firmware is always active
                    }
                logger.info(f"Loaded {len(compiled_registry)} compiled firmware versions")
            except Exception as e:
                logger.error(f"Failed to load compiled firmware registry: {e}")
        
        return registry
    
    def _save_registry(self):
        """Save firmware registry to JSON file"""
        try:
            with open(self.metadata_file, 'w') as f:
                json.dump(self.firmware_registry, f, indent=2)
            return True
        except Exception as e:
            logger.error(f"Failed to save firmware registry: {e}")
            return False
    
    def _calculate_file_hash(self, filepath):
        """Calculate SHA256 hash of firmware file"""
        hash_sha256 = hashlib.sha256()
        try:
            with open(filepath, "rb") as f:
                for chunk in iter(lambda: f.read(4096), b""):
                    hash_sha256.update(chunk)
            return hash_sha256.hexdigest()
        except Exception as e:
            logger.error(f"Failed to calculate hash for {filepath}: {e}")
            return None
    
    def upload_firmware(self, file, device_type, description="", auto_assign=False):
        """
        Upload a new firmware file with automatic versioning
        
        Args:
            file: Uploaded file object
            device_type: Type of device (e.g., "ESP32_PersonalCMS", "ESP32_OTA_Base", "ESP32_LED_Blink")
            description: Optional description
            auto_assign: Whether to auto-assign to all devices of this type
        
        Returns:
            dict: Result with success status and details
        """
        try:
            # Validate file
            if not file or file.filename == '':
                return {'success': False, 'error': 'No file provided'}
            
            # Check file extension
            if not file.filename.lower().endswith('.bin'):
                return {'success': False, 'error': 'Only .bin files are allowed'}
            
            # Auto-generate version based on timestamp (YYYYMMDD.HHMMSS format)
            timestamp = datetime.now()
            version = timestamp.strftime('%Y%m%d.%H%M%S')
            
            # Create secure filename
            safe_filename = f"{device_type}_{version}.bin"
            firmware_path = os.path.join(self.firmware_folder, safe_filename)
            
            # Save file
            file.save(firmware_path)
            
            # Calculate file hash and size
            file_hash = self._calculate_file_hash(firmware_path)
            file_size = os.path.getsize(firmware_path)
            
            if not file_hash:
                os.remove(firmware_path)
                return {'success': False, 'error': 'Failed to calculate file hash'}
            
            # Create firmware metadata
            firmware_info = {
                'version': version,
                'device_type': device_type,
                'filename': safe_filename,
                'filepath': firmware_path,
                'description': description,
                'upload_date': datetime.now().isoformat(),
                'file_size': file_size,
                'file_hash': file_hash,
                'download_count': 0,
                'is_active': True
            }
            
            # Add to registry
            firmware_key = f"{device_type}_{version}"
            self.firmware_registry['firmware_versions'][firmware_key] = firmware_info
            
            # Auto-assign to devices if requested
            if auto_assign:
                self._auto_assign_firmware(device_type, version)
            
            # Save registry
            if self._save_registry():
                logger.info(f"Firmware uploaded successfully: {device_type} v{version}")
                return {
                    'success': True,
                    'firmware_key': firmware_key,
                    'firmware_info': firmware_info
                }
            else:
                # Clean up file if registry save failed
                os.remove(firmware_path)
                return {'success': False, 'error': 'Failed to save firmware registry'}
                
        except Exception as e:
            logger.error(f"Firmware upload failed: {e}")
            return {'success': False, 'error': str(e)}
    
    def _auto_assign_firmware(self, device_type, version):
        """Auto-assign firmware to all devices of specified type"""
        # This would integrate with the device registry to assign firmware
        # For now, we'll just set a default assignment
        if device_type not in self.firmware_registry['device_assignments']:
            self.firmware_registry['device_assignments'][device_type] = {}
        
        self.firmware_registry['device_assignments'][device_type]['latest'] = version
        logger.info(f"Auto-assigned {device_type} v{version} to all devices of this type")
    
    def check_update_for_device(self, device_id, current_version, device_type="ESP32_PersonalCMS"):
        """
        Check if update is available for a specific device
        Always provides the latest firmware available, regardless of type (automatic cross-firmware updates)
        
        Args:
            device_id: Device identifier
            current_version: Current firmware version on device
            device_type: Type of device (informational only)
            
        Returns:
            dict: Update information
        """
        try:
            # First check for forced updates for this specific device
            if device_id in self.forced_updates:
                forced_update = self.forced_updates[device_id]
                logger.info(f"Found forced update for {device_id}: {forced_update['target_firmware']} v{forced_update['version']}")
                
                # Remove the forced update after serving it once
                del self.forced_updates[device_id]
                
                return forced_update

            # Always get the latest firmware from ANY type (automatic cross-firmware updates)
            latest_firmware = self._get_any_latest_firmware()
            
            if not latest_firmware:
                return {
                    'update_available': False,
                    'message': 'No firmware available'
                }
            
            # Compare versions - if different, offer update regardless of firmware type
            if latest_firmware['version'] != current_version:
                update_type = 'cross_firmware' if latest_firmware['device_type'] != device_type else 'same_type'
                
                return {
                    'update_available': True,
                    'version': latest_firmware['version'],
                    'description': f"Auto update to {latest_firmware['device_type']}: {latest_firmware['description']}",
                    'firmware_url': f"/api/ota/download/{latest_firmware['device_type']}_{latest_firmware['version']}",
                    'file_size': latest_firmware['file_size'],
                    'release_date': latest_firmware['upload_date'],
                    'update_type': update_type,
                    'target_firmware': latest_firmware['device_type']
                }

            return {
                'update_available': False,
                'message': f'Device is running latest firmware ({current_version})',
                'current_version': current_version
            }
            
        except Exception as e:
            logger.error(f"Update check failed for {device_id}: {e}")
            return {
                'update_available': False,
                'error': str(e)
            }
    
    def _get_latest_firmware(self, device_type):
        """Get latest firmware for device type"""
        latest_firmware = None
        latest_date = None
        
        for key, firmware in self.firmware_registry['firmware_versions'].items():
            if (firmware['device_type'] == device_type and 
                firmware['is_active']):
                
                upload_date = datetime.fromisoformat(firmware['upload_date'])
                if latest_date is None or upload_date > latest_date:
                    latest_date = upload_date
                    latest_firmware = firmware
        
        return latest_firmware

    def _get_any_latest_firmware(self):
        """Get latest firmware from any device type - allows cross-firmware updates"""
        latest_firmware = None
        latest_date = None
        
        for key, firmware in self.firmware_registry['firmware_versions'].items():
            if firmware['is_active']:
                try:
                    # Handle timezone issues by normalizing the date string
                    upload_date_str = firmware['upload_date']
                    if upload_date_str.endswith('Z'):
                        upload_date_str = upload_date_str[:-1]
                    
                    upload_date = datetime.fromisoformat(upload_date_str.replace('Z', ''))
                    if latest_date is None or upload_date > latest_date:
                        latest_date = upload_date
                        latest_firmware = firmware
                except Exception as e:
                    logger.warning(f"Failed to parse date for firmware {key}: {e}")
                    continue
        
        return latest_firmware

    def _get_device_specific_firmware(self, device_id):
        """
        Get firmware specifically assigned to a device.
        This allows manual override of what firmware a specific device should run.
        For now returns None, can be extended with device assignments.
        """
        # This could be extended to check a device assignments registry
        return None

    def get_available_firmware_types(self):
        """Get all available firmware types for cross-firmware updates"""
        types = set()
        for firmware in self.firmware_registry['firmware_versions'].values():
            if firmware['is_active']:
                types.add(firmware['device_type'])
        return sorted(list(types))

    def get_latest_firmware_for_type(self, device_type):
        """Public method to get latest firmware for any device type"""
        return self._get_latest_firmware(device_type)

    def set_forced_update(self, device_id, firmware_info):
        """Set a forced update for a specific device"""
        try:
            self.forced_updates[device_id] = firmware_info
            logger.info(f"Set forced update for {device_id}: {firmware_info['target_firmware']} v{firmware_info['version']}")
            return True
        except Exception as e:
            logger.error(f"Failed to set forced update for {device_id}: {e}")
            return False

    def clear_forced_update(self, device_id):
        """Clear any forced update for a device"""
        if device_id in self.forced_updates:
            del self.forced_updates[device_id]
            logger.info(f"Cleared forced update for {device_id}")
            return True
        return False

    def get_forced_updates(self):
        """Get all current forced updates"""
        return self.forced_updates.copy()
    
    def get_firmware_file(self, firmware_key):
        """Get firmware file path for download"""
        if firmware_key in self.firmware_registry['firmware_versions']:
            firmware_info = self.firmware_registry['firmware_versions'][firmware_key]
            
            # Check if this is compiled firmware
            if firmware_info.get('is_compiled', False):
                # Return the source path for compiled firmware
                return firmware_info.get('source_path')
            else:
                # Increment download count for uploaded firmware
                firmware_info['download_count'] = firmware_info.get('download_count', 0) + 1
                self._save_registry()
                return firmware_info['filepath']
        
        return None
    
    def list_firmware_versions(self, device_type=None):
        """List all firmware versions, optionally filtered by device type"""
        firmwares = []
        
        for key, firmware in self.firmware_registry['firmware_versions'].items():
            if device_type is None or firmware['device_type'] == device_type:
                firmwares.append({
                    'key': key,
                    **firmware
                })
        
        # Sort by upload date (newest first)
        firmwares.sort(key=lambda x: x['upload_date'], reverse=True)
        return firmwares
    
    def delete_firmware(self, firmware_key):
        """Delete a firmware version"""
        try:
            if firmware_key not in self.firmware_registry['firmware_versions']:
                return {'success': False, 'error': 'Firmware not found'}
            
            firmware_info = self.firmware_registry['firmware_versions'][firmware_key]
            
            # Delete file (handle both uploaded and compiled firmware)
            if firmware_info.get('is_compiled', False):
                # Don't delete compiled firmware files - they're managed separately
                logger.info(f"Skipping file deletion for compiled firmware: {firmware_key}")
            else:
                # Delete uploaded firmware file
                filepath = firmware_info.get('filepath')
                if filepath and os.path.exists(filepath):
                    os.remove(filepath)
            
            # Remove from registry
            del self.firmware_registry['firmware_versions'][firmware_key]
            
            # Save registry
            if self._save_registry():
                logger.info(f"Firmware deleted: {firmware_key}")
                return {'success': True}
            else:
                return {'success': False, 'error': 'Failed to save registry'}
                
        except Exception as e:
            logger.error(f"Failed to delete firmware {firmware_key}: {e}")
            return {'success': False, 'error': str(e)}
    
    def get_device_assignments(self):
        """Get current device-firmware assignments"""
        return self.firmware_registry.get('device_assignments', {})
    
    def assign_firmware_to_device(self, device_id, firmware_key):
        """Assign specific firmware to a device"""
        # This would be implemented based on your device management needs
        pass
    
    def get_update_statistics(self):
        """Get OTA update statistics"""
        stats = {
            'total_firmware_versions': len(self.firmware_registry['firmware_versions']),
            'total_downloads': sum(fw.get('download_count', 0) for fw in self.firmware_registry['firmware_versions'].values()),
            'device_types': {},
            'recent_uploads': []
        }
        
        # Group by device type
        for firmware in self.firmware_registry['firmware_versions'].values():
            device_type = firmware['device_type']
            if device_type not in stats['device_types']:
                stats['device_types'][device_type] = {
                    'count': 0,
                    'latest_version': None,
                    'total_downloads': 0
                }
            
            stats['device_types'][device_type]['count'] += 1
            stats['device_types'][device_type]['total_downloads'] += firmware.get('download_count', 0)
            
            # Update latest version
            current_latest = stats['device_types'][device_type]['latest_version']
            if current_latest is None:
                stats['device_types'][device_type]['latest_version'] = firmware
            else:
                try:
                    # Parse dates safely, handling timezone issues
                    firmware_date_str = firmware['upload_date']
                    current_date_str = current_latest['upload_date']
                    
                    # Remove timezone info if present to compare dates only
                    if firmware_date_str.endswith('Z'):
                        firmware_date_str = firmware_date_str[:-1]
                    if current_date_str.endswith('Z'):
                        current_date_str = current_date_str[:-1]
                    
                    firmware_date = datetime.fromisoformat(firmware_date_str.replace('Z', ''))
                    current_date = datetime.fromisoformat(current_date_str.replace('Z', ''))
                    
                    if firmware_date > current_date:
                        stats['device_types'][device_type]['latest_version'] = firmware
                except Exception as e:
                    logger.warning(f"Date comparison failed for {device_type}: {e}")
                    # Default to version string comparison if date parsing fails
                    if firmware['version'] > current_latest['version']:
                        stats['device_types'][device_type]['latest_version'] = firmware
        
        # Recent uploads (last 10)
        all_firmware = list(self.firmware_registry['firmware_versions'].values())
        all_firmware.sort(key=lambda x: x['upload_date'], reverse=True)
        stats['recent_uploads'] = all_firmware[:10]
        
        return stats