
"""
PersonalCMS - Unified Per-Device Content Management System
- Individual CMS for each device with personalized content
- Nickname support for friendly device identification
- Content managed per device with default preferences
- Server-side content override capabilities
"""

from flask import Flask, request, jsonify, render_template, send_file, redirect, url_for, flash, send_from_directory
from werkzeug.utils import secure_filename
import json
import os
import io
from datetime import datetime, timedelta
import uuid
import logging
from typing import Dict, List, Optional, Tuple
import random
from ota_manager import OTAManager
try:
    from jsonpath_ng import parse as jsonpath_parse
    JSONPATH_AVAILABLE = True
except ImportError:
    JSONPATH_AVAILABLE = False
    jsonpath_parse = None

# Initialize Flask app
app = Flask(__name__, template_folder='templates')
app.config['SECRET_KEY'] = 'personalcms-unified-server-2024'
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///unified_cms.db'  # Use relative path in project root
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['UPLOAD_FOLDER'] = 'data/uploads'
app.config['GENERATED_FOLDER'] = 'data/generated'
app.config['DASHBOARD_FOLDER'] = 'dashboards'
app.config['DEVICE_CONTENT_FOLDER'] = 'data/device_content'
app.config['OTA_FOLDER'] = 'data/ota'
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16MB max file size
app.config['DEVICE_ACTIVE_TIMEOUT_SECONDS'] = 300  # Consider offline if no ping within 5 minutes

# Ensure directories exist
for folder in ['data', 'data/uploads', 'data/generated', 'data/device_content', 'data/ota', 'data/ota/firmware', 'templates', 'static', 'dashboards']:
    os.makedirs(folder, exist_ok=True)

# Initialize database
from models import db, Device, DeviceContent, UserImage, ContentAPI, ContentSource, DefaultContent, PerDeviceCMS, ImageProcessor
db.init_app(app)

# Initialize CMS components after database setup
per_device_cms = PerDeviceCMS()
image_processor = ImageProcessor()
ota_manager = OTAManager(app.config['OTA_FOLDER'])

# Add Jinja2 filter for JSON parsing
@app.template_filter('from_json')
def from_json_filter(value):
    """Convert JSON string to Python object"""
    try:
        return json.loads(value) if value else []
    except (json.JSONDecodeError, TypeError):
        return []

# Add Jinja2 function for current datetime
@app.template_global()
def moment():
    """Return current datetime formatted like moment.js"""
    return datetime.now()

# Add Jinja2 filter for datetime formatting
@app.template_filter('strftime')
def datetime_filter(date, format='%Y-%m-%d %H:%M'):
    """Format datetime"""
    if date:
        return date.strftime(format)
    return ''

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Create all tables
with app.app_context():
    db.create_all()

# Basic homepage route 
@app.route('/')
def index():
    """Main dashboard with per-device overview"""
    from models import refresh_device_activity_statuses
    # Refresh device is_active flags based on last_seen timeout
    try:
        refresh_device_activity_statuses()
    except Exception:
        pass
    devices = Device.query.order_by(Device.last_seen.desc()).all()
    recent_content = DeviceContent.query.order_by(DeviceContent.created_at.desc()).limit(10).all()
    images = UserImage.query.order_by(UserImage.uploaded_at.desc()).limit(5).all()
    
    # Stats
    total_devices = len(devices)
    active_devices = len([d for d in devices if d.is_active])
    custom_content_devices = len([d for d in devices if d.custom_content_enabled])
    
    # Pass current datetime to template
    current_time = datetime.now().strftime('%Y-%m-%d %H:%M')
    
    return render_template('unified_dashboard.html', 
                         devices=devices, 
                         recent_content=recent_content,
                         images=images,
                         current_time=current_time,
                         stats={
                             'total_devices': total_devices,
                             'active_devices': active_devices,
                             'custom_content_devices': custom_content_devices,
                             'total_content': DeviceContent.query.count()
                         })

# API Routes for device registration
@app.route('/api/devices/register', methods=['POST'])
def register_device():
    """Register or update a device"""
    try:
        data = request.get_json(force=True)
        device_id = data.get('device_id')
        device_name = data.get('device_name')
        occupation = data.get('occupation')
        device_type = data.get('device_type')
        wifi_ssid = data.get('wifi_ssid')
        preferences = json.dumps(data.get('preferences', {}))
        nickname = data.get('nickname')

        if not device_id or not device_name or not occupation or not device_type:
            return jsonify({'success': False, 'message': 'Missing required fields'}), 400

        device = Device.query.filter_by(device_id=device_id).first()
        if device:
            # Update existing device
            device.device_name = device_name
            device.occupation = occupation
            device.device_type = device_type
            device.wifi_ssid = wifi_ssid
            device.preferences = preferences
            device.nickname = nickname
            device.last_seen = datetime.utcnow()
            device.is_connected = True
            device.is_active = True
        else:
            # Create new device
            device = Device(
                device_id=device_id,
                device_name=device_name,
                occupation=occupation,
                device_type=device_type,
                wifi_ssid=wifi_ssid,
                preferences=preferences,
                nickname=nickname,
                last_seen=datetime.utcnow(),
                is_connected=True,
                is_active=True
            )
            db.session.add(device)
        
        db.session.commit()
        logger.info(f"Device registered: {device_id} ({device_name})")
        return jsonify({'success': True, 'message': 'Device registered successfully', 'device_id': device_id})
    
    except Exception as e:
        logger.error(f"Device registration error: {str(e)}")
        return jsonify({'success': False, 'message': f'Registration failed: {str(e)}'}), 500

@app.route('/api/devices/<device_id>/sensor-data', methods=['POST'])
def update_sensor_data(device_id):
    """Update sensor data for a specific device"""
    try:
        data = request.get_json()
        
        # Enhanced logging for debugging
        logger.info(f"🌡️  Sensor data received for {device_id}")
        logger.info(f"📦 Raw JSON data: {data}")
        
        if not data:
            logger.warning(f"❌ No JSON data provided for {device_id}")
            return jsonify({'error': 'No JSON data provided'}), 400
        
        # Find the device
        device = Device.query.filter_by(device_id=device_id).first()
        if not device:
            logger.warning(f"❌ Device {device_id} not found")
            return jsonify({'error': 'Device not found'}), 404
        
        # Log current values before update
        logger.info(f"📊 Current stored values - Temperature: {device.temperature}°C, Humidity: {device.humidity}%")
        
        # Update sensor data
        updated_fields = []
        if 'temperature' in data:
            old_temp = device.temperature
            device.temperature = data['temperature']
            updated_fields.append('temperature')
            logger.info(f"🌡️  Temperature: {old_temp}°C → {device.temperature}°C")
        if 'humidity' in data:
            old_humidity = device.humidity
            device.humidity = data['humidity']
            updated_fields.append('humidity')
            logger.info(f"💧 Humidity: {old_humidity}% → {device.humidity}%")
        if 'motion_detected' in data:
            device.motion_detected = data['motion_detected']
            updated_fields.append('motion_detected')
            logger.info(f"👁️  Motion: {device.motion_detected}")
        if 'sleep_mode' in data:
            device.sleep_mode = data['sleep_mode']
            updated_fields.append('sleep_mode')
            logger.info(f"💤 Sleep mode: {device.sleep_mode}")
        
        if updated_fields:
            device.sensor_last_update = datetime.utcnow()
            device.last_seen = datetime.utcnow()
            device.is_active = True
            
        db.session.commit()
        logger.info(f"✅ Database committed successfully")
        
        # Verify the data was saved correctly
        db.session.refresh(device)
        logger.info(f"📊 Final stored values - Temperature: {device.temperature}°C, Humidity: {device.humidity}%")
        
        logger.info(f"Sensor data updated for {device_id}: {updated_fields}")
        
        return jsonify({
            'status': 'success',
            'message': f'Sensor data updated: {", ".join(updated_fields)}',
            'device': device.to_dict()
        }), 200
        
    except Exception as e:
        logger.error(f"Sensor data update failed for {device_id}: {e}")
        return jsonify({'error': 'Internal server error'}), 500

@app.route('/api/devices/<device_id>/images-sequence', methods=['GET'])
def get_images_sequence(device_id):
    """Get images sequence for a device"""
    logger.info(f"🔍 ESP32 API Request: device_id={device_id}")
    try:
        device = Device.query.filter_by(device_id=device_id).first()
        if not device:
            logger.warning(f"❌ Device not found: {device_id}")
            return jsonify({'success': False, 'message': 'Device not found'}), 404
        
        logger.info(f"✅ Device found: {device.device_name} ({device_id})")
        
        # Update last seen
        device.last_seen = datetime.utcnow()
        device.is_active = True
        db.session.commit()
        
        # Generate content for device
        content = per_device_cms.get_content_for_device(device)
        
        # Generate dashboard image
        dashboard_path = image_processor.generate_dashboard(device, content, app_config=app.config)
        
        # Get assigned user images for this device
        assigned_images = []
        images = UserImage.query.all()
        for img in images:
            assignments = json.loads(img.device_assignments or '[]')
            if device_id in assignments:
                assigned_images.append({
                    'id': img.id,
                    'filename': img.filename,
                    'url': f"/uploads/{img.filename}",
                    'bmp_url': f"/uploads/{img.filename}/bmp",
                    'file_size': img.file_size
                })
        
        logger.info(f"📊 Found {len(assigned_images)} assigned images for {device_id}")
        for img in assigned_images:
            logger.info(f"  - {img['filename']} -> BMP: {img['bmp_url']}")
        
        response_data = {
            'success': True,
            'message': 'Content generated',
            'device_id': device_id,
            'dashboard_url': f"/dashboards/{device_id}_current.bmp",
            'fallback_url': f"/dashboards/{device_id}_fallback.bmp",
            'assigned_images': assigned_images,
            'content_categories': list(content.keys()) if content else []
        }
        
        logger.info(f"📤 ESP32 Response: dashboard={response_data['dashboard_url']}, images={len(assigned_images)}")
        return jsonify(response_data)
        
    except Exception as e:
        logger.error(f"Images sequence error for {device_id}: {str(e)}")
        return jsonify({'success': False, 'message': str(e)}), 500

# Dashboard BMP file serving
@app.route('/dashboards/<filename>')
def serve_dashboard(filename):
    """Serve generated dashboard BMP files"""
    try:
        return send_from_directory(app.config['DASHBOARD_FOLDER'], filename)
    except Exception as e:
        logger.error(f"Dashboard file serve error: {str(e)}")
        return jsonify({'error': 'Dashboard file not found'}), 404

# Uploaded images serving
@app.route('/uploads/<filename>')
def serve_uploaded_image(filename):
    """Serve uploaded image files"""
    try:
        return send_from_directory(app.config['UPLOAD_FOLDER'], filename)
    except Exception as e:
        logger.error(f"Upload file serve error: {str(e)}")
        return jsonify({'error': 'Upload file not found'}), 404

# BMP versions of uploaded images
@app.route('/uploads/<filename>/bmp')
def serve_uploaded_image_bmp(filename):
    """Serve BMP version of uploaded image files for ESP32"""
    try:
        from models import ImageProcessor
        
        # Find the original image path
        original_path = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        if not os.path.exists(original_path):
            return jsonify({'error': 'Original image not found'}), 404
        
        # Create BMP version path
        bmp_filename = os.path.splitext(filename)[0] + '.bmp'
        bmp_path = os.path.join(app.config['UPLOAD_FOLDER'], 'bmp', bmp_filename)
        
        # Create BMP directory if it doesn't exist
        bmp_dir = os.path.join(app.config['UPLOAD_FOLDER'], 'bmp')
        os.makedirs(bmp_dir, exist_ok=True)
        
        # Convert to BMP if not already exists or if original is newer
        if not os.path.exists(bmp_path) or os.path.getmtime(original_path) > os.path.getmtime(bmp_path):
            success = ImageProcessor.convert_to_bmp(original_path, bmp_path, size=(800, 480))
            if not success:
                return jsonify({'error': 'BMP conversion failed'}), 500
        
        return send_from_directory(bmp_dir, bmp_filename)
        
    except Exception as e:
        logger.error(f"BMP image serve error: {str(e)}")
        return jsonify({'error': 'BMP conversion error'}), 500

# Additional API endpoints for dashboard buttons
@app.route('/api/devices/<device_id>/content')
def api_device_content(device_id):
    """API endpoint for ESP32 devices to get content (JSON response)"""
    try:
        logger.info(f"📊 Content API request from ESP32: {device_id}")
        
        # Find or create device
        device = Device.query.filter_by(device_id=device_id).first()
        if not device:
            logger.warning(f"Device {device_id} not found, creating new device")
            device = Device(
                device_id=device_id,
                device_name=f"ESP32 Device {device_id}",
                device_type="ESP32_DevKit_V1",
                occupation="Auto-registered Device"
            )
            db.session.add(device)
            db.session.commit()
        
        # Update last seen
        device.last_seen = datetime.utcnow()
        db.session.commit()
        
        # Get device content using the per-device CMS
        content = per_device_cms.get_content_for_device(device)
        
        # Generate dashboard image
        dashboard_path = image_processor.generate_dashboard(device, content, app_config=app.config)
        
        # Get assigned images
        images = []
        user_images = UserImage.query.all()
        for img in user_images:
            assignments = json.loads(img.device_assignments or '[]')
            if device_id in assignments:
                images.append({
                    'filename': img.filename,
                    'bmp_url': f"/uploads/{img.filename}/bmp",
                    'upload_date': img.upload_date.isoformat() if img.upload_date else None
                })
        
        logger.info(f"📊 Content for {device_id}: dashboard + {len(images)} images")
        for img in images:
            logger.info(f"  - {img['filename']} -> BMP: {img['bmp_url']}")
        
        # Return JSON response for ESP32
        response_data = {
            'device_id': device_id,
            'timestamp': datetime.utcnow().isoformat(),
            'dashboard_url': f"/dashboards/{device_id}_current.bmp",
            'fallback_url': f"/dashboards/{device_id}_fallback.bmp",
            'images': images,
            'content': content,
            'status': 'success'
        }
        
        logger.info(f"✅ Content API response prepared for {device_id}")
        return jsonify(response_data)
        
    except Exception as e:
        logger.error(f"❌ Content API error for {device_id}: {str(e)}")
        return jsonify({
            'device_id': device_id,
            'error': str(e),
            'status': 'error',
            'timestamp': datetime.utcnow().isoformat()
        }), 500

# ESP32 Diagnostic endpoint
@app.route('/api/esp32-debug/<device_id>')
def esp32_debug(device_id):
    """Simple debug endpoint for ESP32 troubleshooting"""
    try:
        device = Device.query.filter_by(device_id=device_id).first()
        if not device:
            return jsonify({
                'error': 'Device not found',
                'device_id': device_id,
                'status': 'NOT_FOUND'
            }), 404
        
        # Get assigned images
        assigned_images = []
        images = UserImage.query.all()
        for img in images:
            assignments = json.loads(img.device_assignments or '[]')
            if device_id in assignments:
                assigned_images.append(img.filename)
        
        debug_info = {
            'device_id': device_id,
            'device_name': device.device_name,
            'status': 'OK',
            'server_time': datetime.utcnow().isoformat(),
            'dashboard_bmp_url': f"http://192.168.1.104:5000/dashboards/{device_id}_current.bmp",
            'assigned_images': assigned_images,
            'image_bmp_urls': [f"http://192.168.1.104:5000/uploads/{img}/bmp" for img in assigned_images],
            'simple_test': 'ESP32_CONNECTED'
        }
        
        logger.info(f"🔍 ESP32 Debug request for {device_id}")
        return jsonify(debug_info)
        
    except Exception as e:
        logger.error(f"ESP32 Debug error: {str(e)}")
        return jsonify({'error': str(e), 'status': 'ERROR'}), 500

@app.route('/api/devices/<device_id>/dashboard')  
def api_device_dashboard(device_id):
    """API endpoint for device dashboard preview (redirects to web page)"""
    return redirect(url_for('preview_dashboard', device_id=device_id))

# Web interface routes for dashboard functionality
@app.route('/device/<device_id>')
def device_detail(device_id):
    """Device detail page"""
    try:
        device = Device.query.filter_by(device_id=device_id).first()
        if not device:
            flash('Device not found', 'error')
            return redirect(url_for('index'))
        
        # Get device content
        device_content = DeviceContent.query.filter_by(device_id=device_id).order_by(DeviceContent.created_at.desc()).all()
        
        return render_template('device_detail.html', device=device, device_content=device_content)
    except Exception as e:
        logger.error(f"Device detail error: {str(e)}")
        flash(f'Error loading device: {str(e)}', 'error')
        return redirect(url_for('index'))

@app.route('/device/<device_id>/content')
def view_device_content(device_id):
    """View device content"""
    try:
        device = Device.query.filter_by(device_id=device_id).first()
        if not device:
            return jsonify({'error': 'Device not found'}), 404
        
        # Get content for device
        content = per_device_cms.get_content_for_device(device)
        
        return render_template('device_content.html', device=device, content=content)
    except Exception as e:
        logger.error(f"View content error: {str(e)}")
        return jsonify({'error': str(e)}), 500

@app.route('/device/<device_id>/dashboard-preview')
def preview_dashboard(device_id):
    """Preview device dashboard"""
    try:
        device = Device.query.filter_by(device_id=device_id).first()
        if not device:
            return jsonify({'error': 'Device not found'}), 404
        
        # Generate content for device
        content = per_device_cms.get_content_for_device(device)
        
        return render_template('dashboard_preview.html', device=device, content=content)
    except Exception as e:
        logger.error(f"Dashboard preview error: {str(e)}")
        return jsonify({'error': str(e)}), 500

@app.route('/content-management')
def content_management():
    """Content management page"""
    try:
        devices = Device.query.all()
        recent_content = DeviceContent.query.order_by(DeviceContent.created_at.desc()).limit(20).all()
        
        return render_template('content_management.html', devices=devices, recent_content=recent_content)
    except Exception as e:
        logger.error(f"Content management error: {str(e)}")
        flash(f'Error loading content management: {str(e)}', 'error')
        return redirect(url_for('index'))

@app.route('/upload', methods=['GET', 'POST'])
def upload_image():
    """Handle file upload page and uploads"""
    if request.method == 'POST':
        try:
            if 'file' not in request.files:
                flash('No file selected', 'error')
                return redirect(url_for('upload_image'))
            
            file = request.files['file']
            if file.filename == '':
                flash('No file selected', 'error')
                return redirect(url_for('upload_image'))
            
            if file:
                filename = secure_filename(file.filename)
                unique_filename = f"{uuid.uuid4()}_{filename}"
                file_path = os.path.join(app.config['UPLOAD_FOLDER'], unique_filename)
                file.save(file_path)
                
                # Get file size
                file_size = os.path.getsize(file_path)
                
                # Save to database
                user_image = UserImage(
                    filename=unique_filename,
                    filepath=file_path,
                    file_size=file_size
                )
                db.session.add(user_image)
                db.session.commit()
                
                flash('File uploaded successfully', 'success')
                return redirect(url_for('upload_image'))
        except Exception as e:
            flash(f'Upload failed: {str(e)}', 'error')
            return redirect(url_for('upload_image'))
    
    # GET request - show the upload form
    return render_template('upload.html')

# Content management routes
@app.route('/content-sources')
def content_sources():
    """Content sources management page"""
    try:
        # Get all content sources
        sources = ContentSource.query.all()
        
        # Build source configuration dict for the template
        source_config = {}
        for source in sources:
            if source.category not in source_config:
                source_config[source.category] = {}
            source_config[source.category][source.subcategory] = source.source_type
        
        return render_template('content_sources.html', sources=sources, source_config=source_config)
    except Exception as e:
        logger.error(f"Content sources error: {str(e)}")
        flash(f'Error loading content sources: {str(e)}', 'error')
        return redirect(url_for('index'))

@app.route('/content-apis')
def content_apis():
    """Content APIs management page"""
    try:
        apis = ContentAPI.query.all()
        return render_template('content_apis.html', apis=apis)
    except Exception as e:
        logger.error(f"Content APIs error: {str(e)}")
        flash(f'Error loading content APIs: {str(e)}', 'error')
        return redirect(url_for('index'))

@app.route('/default-content')
def default_content():
    """Default content management page"""
    try:
        # Get default content by category
        jokes = DefaultContent.query.filter_by(category='jokes', is_active=True).all()
        riddles = DefaultContent.query.filter_by(category='riddles', is_active=True).all() 
        news = DefaultContent.query.filter_by(category='news', is_active=True).all()
        
        return render_template('default_content.html', 
                             jokes=jokes, riddles=riddles, news=news)
    except Exception as e:
        logger.error(f"Default content error: {str(e)}")
        flash(f'Error loading default content: {str(e)}', 'error')
        return redirect(url_for('index'))

# Placeholder routes for missing functionality
@app.route('/toggle-content-source', methods=['POST'])
def toggle_content_source():
    """Toggle content source configuration"""
    try:
        category = request.form.get('category')
        subcategory = request.form.get('subcategory')
        source_type = request.form.get('source_type')
        
        if not all([category, subcategory, source_type]):
            flash('Missing form data', 'error')
            return redirect(url_for('content_sources'))
        
        # Find or create content source
        content_source = ContentSource.query.filter_by(
            category=category, 
            subcategory=subcategory
        ).first()
        
        if content_source:
            content_source.source_type = source_type
        else:
            content_source = ContentSource(
                category=category,
                subcategory=subcategory,
                source_type=source_type
            )
            db.session.add(content_source)
        
        db.session.commit()
        flash(f'Updated {category} -> {subcategory} to use {source_type}', 'success')
        return redirect(url_for('content_sources'))
        
    except Exception as e:
        logger.error(f"Toggle content source error: {str(e)}")
        flash(f'Error updating content source: {str(e)}', 'error')
        return redirect(url_for('content_sources'))

@app.route('/test-content-api/<int:api_id>', methods=['POST'])
def test_content_api(api_id):
    """Test content API (placeholder)"""
    flash(f'API test functionality coming soon (API ID: {api_id})', 'info')
    return redirect(url_for('content_apis'))

@app.route('/delete-content-api/<int:api_id>', methods=['POST'])
def delete_content_api(api_id):
    """Delete content API (placeholder)"""
    flash(f'API deletion functionality coming soon (API ID: {api_id})', 'info')
    return redirect(url_for('content_apis'))

@app.route('/edit-default-content/<category>', methods=['POST'])
@app.route('/edit-default-content/<category>/<int:item_id>/<action>', methods=['POST'])
def edit_default_content(category, item_id=None, action=None):
    """Edit default content (placeholder)"""
    if action == 'delete' and item_id:
        flash(f'Delete {category} content functionality coming soon (ID: {item_id})', 'info')
    else:
        flash(f'Edit {category} content functionality coming soon', 'info')
    return redirect(url_for('default_content'))

@app.route('/assign-device-images', methods=['POST'])
def assign_device_images():
    """Handle device image assignments"""
    try:
        devices = Device.query.all()
        images = UserImage.query.all()
        
        for device in devices:
            # Get selected image IDs for this device
            selected_ids = request.form.getlist(f'assign_{device.device_id}')
            selected_ids = [int(id) for id in selected_ids if id.isdigit()]
            
            # Update all images for this device
            for image in images:
                current_assignments = json.loads(image.device_assignments or '[]')
                
                # Remove this device from assignments
                if device.device_id in current_assignments:
                    current_assignments.remove(device.device_id)
                
                # Add back if selected
                if image.id in selected_ids:
                    current_assignments.append(device.device_id)
                
                # Update the image
                image.device_assignments = json.dumps(current_assignments)
        
        db.session.commit()
        flash('Device image assignments updated successfully!', 'success')
        
    except Exception as e:
        logger.error(f"Device assignment error: {str(e)}")
        flash(f'Error updating assignments: {str(e)}', 'error')
    
    return redirect(url_for('index'))

# Device Removal API (Critical route - must be in main file)
@app.route('/api/devices/<device_id>', methods=['DELETE'])
def api_remove_device(device_id):
    """API endpoint to remove a device and all its associated content"""
    logger.info(f"Device removal request received for: {device_id}")
    
    try:
        device = Device.query.filter_by(device_id=device_id).first()
        if not device:
            logger.warning(f"Device not found: {device_id}")
            return jsonify({'error': 'Device not found'}), 404
        
        device_name = device.nickname or device.device_name
        logger.info(f"Removing device: {device_name} ({device_id})")
        
        # Delete device's custom content
        deleted_content = DeviceContent.query.filter_by(device_id=device_id).count()
        logger.info(f"Found {deleted_content} custom content items to delete")
        DeviceContent.query.filter_by(device_id=device_id).delete()
        
        # Remove device from image assignments  
        images = UserImage.query.all()
        unassigned_images = 0
        for img in images:
            if img.device_assignments:
                try:
                    assignments = json.loads(img.device_assignments)
                    if device_id in assignments:
                        assignments.remove(device_id)
                        img.device_assignments = json.dumps(assignments)
                        unassigned_images += 1
                except json.JSONDecodeError:
                    continue
        
        logger.info(f"Removed device from {unassigned_images} image assignments")
        
        # Delete the device
        db.session.delete(device)
        db.session.commit()
        
        logger.info(f"Device removed successfully: {device_id} ({device_name})")
        
        return jsonify({
            'status': 'success',
            'message': f'Device "{device_name}" successfully removed',
            'device_id': device_id,
            'device_name': device_name,
            'cleanup_summary': {
                'custom_content_deleted': deleted_content,
                'image_assignments_removed': unassigned_images
            }
        }), 200
        
    except Exception as e:
        logger.error(f"Device removal failed for {device_id}: {str(e)}", exc_info=True)
        db.session.rollback()
        return jsonify({'error': f'Failed to remove device: {str(e)}'}), 500

@app.route('/api/devices/<device_id>/unlink', methods=['POST']) 
def api_unlink_device(device_id):
    """API endpoint to unlink/disconnect a device without deleting it (marks as inactive)"""
    logger.info(f"Device unlink request received for: {device_id}")
    
    try:
        device = Device.query.filter_by(device_id=device_id).first()
        if not device:
            logger.warning(f"Device not found: {device_id}")
            return jsonify({'error': 'Device not found'}), 404
        
        device_name = device.nickname or device.device_name
        logger.info(f"Unlinking device: {device_name} ({device_id})")
        
        # Mark device as inactive but preserve data
        device.is_active = False
        device.is_connected = False
        db.session.commit()
        
        logger.info(f"Device unlinked successfully: {device_id} ({device_name})")
        
        return jsonify({
            'status': 'success',
            'message': f'Device "{device_name}" unlinked successfully',
            'device_id': device_id,
            'device_name': device_name,
            'note': 'Device marked as inactive but data preserved. Use DELETE /api/devices/{device_id} to completely remove.'
        }), 200
        
    except Exception as e:
        logger.error(f"Device unlink failed for {device_id}: {str(e)}", exc_info=True)
        db.session.rollback()
        return jsonify({'error': f'Failed to unlink device: {str(e)}'}), 500

@app.route('/api/devices/<device_id>/preferences', methods=['PUT'])
def api_update_device_preferences(device_id):
    """API endpoint to update device content preferences"""
    logger.info(f"Device preferences update request received for: {device_id}")
    
    try:
        device = Device.query.filter_by(device_id=device_id).first()
        if not device:
            logger.warning(f"Device not found: {device_id}")
            return jsonify({'error': 'Device not found'}), 404
        
        device_name = device.nickname or device.device_name
        data = request.get_json()
        
        if not data or 'preferences' not in data:
            return jsonify({'error': 'Preferences data required'}), 400
        
        # Validate preferences structure
        prefs = data['preferences']
        valid_categories = ['jokes', 'riddles', 'news']
        valid_options = {
            'jokes': ['dad_jokes', 'programming_jokes', 'clean_jokes'],
            'riddles': ['easy_riddles', 'logic_riddles', 'math_riddles'],
            'news': ['tech_news', 'world_news', 'science_news']
        }
        
        # Validate and clean preferences
        cleaned_prefs = {}
        for category, items in prefs.items():
            if category in valid_categories and isinstance(items, list):
                cleaned_prefs[category] = [item for item in items if item in valid_options[category]]
        
        # Update device preferences
        device.preferences = json.dumps(cleaned_prefs)
        db.session.commit()
        
        logger.info(f"Device preferences updated successfully: {device_id} ({device_name})")
        
        return jsonify({
            'status': 'success',
            'message': f'Preferences updated for "{device_name}"',
            'device_id': device_id,
            'device_name': device_name,
            'preferences': cleaned_prefs
        }), 200
        
    except Exception as e:
        logger.error(f"Device preferences update failed for {device_id}: {str(e)}", exc_info=True)
        db.session.rollback()
        return jsonify({'error': f'Failed to update preferences: {str(e)}'}), 500

@app.route('/api/devices/<device_id>/preferences', methods=['GET'])
def api_get_device_preferences(device_id):
    """API endpoint to get device content preferences"""
    try:
        device = Device.query.filter_by(device_id=device_id).first()
        if not device:
            return jsonify({'error': 'Device not found'}), 404
        
        preferences = json.loads(device.preferences) if device.preferences else {}
        
        return jsonify({
            'status': 'success',
            'device_id': device_id,
            'device_name': device.nickname or device.device_name,
            'preferences': preferences
        }), 200
        
    except Exception as e:
        logger.error(f"Get device preferences failed for {device_id}: {str(e)}")
        return jsonify({'error': f'Failed to get preferences: {str(e)}'}), 500

# OTA Update API Routes
@app.route('/api/ota/check/<device_id>', methods=['GET'])
def check_ota_update(device_id):
    """Check for OTA updates for a specific device"""
    try:
        # Get device info to determine type
        device = Device.query.filter_by(device_id=device_id).first()
        if not device:
            return jsonify({'error': 'Device not found'}), 404
        
        # Get current version from headers
        current_version = request.headers.get('X-Device-Version', '1.0.0')
        device_type = request.headers.get('X-Device-Type', 'ESP32_PersonalCMS')
        
        logger.info(f"OTA check for {device_id}: current={current_version}, type={device_type}")
        
        # Check for updates
        update_info = ota_manager.check_update_for_device(device_id, current_version, device_type)
        
        # Update device last seen
        device.last_seen = datetime.utcnow()
        device.is_active = True
        db.session.commit()
        
        # If update available, provide full URL
        if update_info.get('update_available'):
            # Fix URL duplication - only add if not already present
            firmware_url = update_info['firmware_url']
            if not firmware_url.startswith('http'):
                update_info['firmware_url'] = request.url_root.rstrip('/') + firmware_url
        
        logger.info(f"OTA response for {device_id}: {update_info}")
        return jsonify(update_info)
        
    except Exception as e:
        logger.error(f"OTA check failed for {device_id}: {str(e)}")
        return jsonify({'error': 'OTA check failed'}), 500

@app.route('/api/ota/download/<firmware_key>')
def download_firmware(firmware_key):
    """Download firmware file"""
    try:
        firmware_path = ota_manager.get_firmware_file(firmware_key)
        if not firmware_path or not os.path.exists(firmware_path):
            return jsonify({'error': 'Firmware not found'}), 404
        
        logger.info(f"Serving firmware download: {firmware_key}")
        return send_file(firmware_path, as_attachment=True, download_name=f"{firmware_key}.bin")
        
    except Exception as e:
        logger.error(f"Firmware download failed for {firmware_key}: {str(e)}")
        return jsonify({'error': 'Download failed'}), 500

@app.route('/api/ota/firmware-types')
def get_firmware_types():
    """Get all available firmware types for cross-firmware updates"""
    try:
        types = ota_manager.get_available_firmware_types()
        return jsonify({'firmware_types': types})
    except Exception as e:
        logger.error(f"Failed to get firmware types: {str(e)}")
        return jsonify({'error': 'Failed to get firmware types'}), 500

@app.route('/api/ota/firmware')
def get_firmware_list():
    """Get all firmware versions"""
    try:
        firmware_list = ota_manager.list_firmware_versions()
        return jsonify({'firmware_versions': firmware_list})
    except Exception as e:
        logger.error(f"Failed to get firmware list: {str(e)}")
        return jsonify({'error': 'Failed to get firmware list'}), 500

@app.route('/api/ota/force-update/<device_id>', methods=['POST'])
def force_firmware_update(device_id):
    """Force a specific firmware type on a device"""
    try:
        data = request.get_json()
        target_firmware_type = data.get('firmware_type')
        
        if not target_firmware_type:
            return jsonify({'error': 'firmware_type required'}), 400
        
        # Get the latest firmware of the specified type
        latest_firmware = ota_manager.get_latest_firmware_for_type(target_firmware_type)
        
        if not latest_firmware:
            return jsonify({'error': f'No firmware found for type: {target_firmware_type}'}), 404
        
        # Create firmware key for download
        firmware_key = f"{target_firmware_type}_{latest_firmware['version']}"
        
        # Create the forced update info
        update_info = {
            'update_available': True,
            'version': latest_firmware['version'],
            'description': f"Forced update to {target_firmware_type}: {latest_firmware['description']}",
            'firmware_url': request.url_root.rstrip('/') + f"/api/ota/download/{firmware_key}",
            'file_size': latest_firmware['file_size'],
            'release_date': latest_firmware['upload_date'],
            'update_type': 'forced_cross_firmware',
            'target_firmware': target_firmware_type
        }
        
        # Store the forced update for this device
        success = ota_manager.set_forced_update(device_id, update_info)
        
        if not success:
            return jsonify({'error': 'Failed to set forced update'}), 500
        
        logger.info(f"Forced firmware update set for {device_id}: {target_firmware_type}")
        return jsonify(update_info)
        
    except Exception as e:
        logger.error(f"Forced update failed for {device_id}: {str(e)}")
        return jsonify({'error': 'Forced update failed'}), 500

@app.route('/api/ota/upload', methods=['POST'])
def upload_firmware():
    """Upload new firmware file"""
    try:
        if 'firmware' not in request.files:
            return jsonify({'error': 'No firmware file provided'}), 400
        
        file = request.files['firmware']
        version = request.form.get('version')
        device_type = request.form.get('device_type', 'ESP32_PersonalCMS')
        description = request.form.get('description', '')
        auto_assign = request.form.get('auto_assign') == 'true'
        
        if not version:
            return jsonify({'error': 'Version is required'}), 400
        
        # Upload firmware
        result = ota_manager.upload_firmware(file, version, device_type, description, auto_assign)
        
        if result['success']:
            logger.info(f"Firmware uploaded: {device_type} v{version}")
            return jsonify(result)
        else:
            return jsonify(result), 400
            
    except Exception as e:
        logger.error(f"Firmware upload failed: {str(e)}")
        return jsonify({'error': 'Upload failed'}), 500

@app.route('/api/ota/versions')
def list_firmware_versions():
    """List all firmware versions"""
    try:
        device_type = request.args.get('device_type')
        versions = ota_manager.list_firmware_versions(device_type)
        return jsonify({'firmware_versions': versions})
    except Exception as e:
        logger.error(f"Failed to list firmware versions: {str(e)}")
        return jsonify({'error': 'Failed to list versions'}), 500

@app.route('/api/ota/delete/<firmware_key>', methods=['DELETE'])
def delete_firmware(firmware_key):
    """Delete a firmware version"""
    try:
        result = ota_manager.delete_firmware(firmware_key)
        if result['success']:
            return jsonify(result)
        else:
            return jsonify(result), 400
    except Exception as e:
        logger.error(f"Failed to delete firmware {firmware_key}: {str(e)}")
        return jsonify({'error': 'Delete failed'}), 500

@app.route('/api/ota/stats')
def ota_statistics():
    """Get OTA update statistics"""
    try:
        stats = ota_manager.get_update_statistics()
        return jsonify(stats)
    except Exception as e:
        logger.error(f"Failed to get OTA stats: {str(e)}")
        return jsonify({'error': 'Failed to get statistics'}), 500

# Device heartbeat endpoint for OTA-enabled devices
@app.route('/api/devices/<device_id>/heartbeat', methods=['POST'])
def device_heartbeat(device_id):
    """Receive heartbeat from OTA-enabled devices"""
    try:
        data = request.get_json()
        
        device = Device.query.filter_by(device_id=device_id).first()
        if not device:
            # Create device if it doesn't exist (for OTA-only devices)
            device = Device(
                device_id=device_id,
                device_name=data.get('device_name', device_id),
                device_type=data.get('device_type', 'ESP32_OTA'),
                last_seen=datetime.utcnow(),
                is_connected=True,
                is_active=True
            )
            db.session.add(device)
        else:
            # Update existing device
            device.last_seen = datetime.utcnow()
            device.is_active = True
            device.is_connected = True
        
        # Update additional heartbeat data if provided
        if 'version' in data:
            device.firmware_version = data['version']
        if 'uptime' in data:
            device.uptime_seconds = data['uptime']
        if 'free_heap' in data:
            device.free_heap = data['free_heap']
        if 'wifi_rssi' in data:
            device.wifi_rssi = data['wifi_rssi']
        
        db.session.commit()
        
        logger.info(f"Heartbeat received from {device_id}")
        return jsonify({'status': 'heartbeat_received', 'timestamp': datetime.utcnow().isoformat()})
        
    except Exception as e:
        logger.error(f"Heartbeat processing failed for {device_id}: {str(e)}")
        return jsonify({'error': 'Heartbeat processing failed'}), 500

@app.route('/device/<device_id>/preferences', methods=['POST'])
def update_device_preferences(device_id):
    """Web route to update device preferences"""
    try:
        device = Device.query.filter_by(device_id=device_id).first_or_404()
        
        # Build preferences from form data
        preferences = {
            'jokes': [],
            'riddles': [], 
            'news': []
        }
        
        # Get all preference checkboxes
        for key in request.form.getlist('prefs'):
            if key in ['dad_jokes', 'programming_jokes', 'clean_jokes']:
                preferences['jokes'].append(key)
            elif key in ['easy_riddles', 'logic_riddles', 'math_riddles']:
                preferences['riddles'].append(key)
            elif key in ['tech_news', 'world_news', 'science_news']:
                preferences['news'].append(key)
        
        # Update device preferences
        device.preferences = json.dumps(preferences)
        db.session.commit()
        
        flash(f'Content preferences updated for {device.nickname or device.device_name}', 'success')
        logger.info(f"Device preferences updated via web: {device_id}")
        
    except Exception as e:
        logger.error(f"Web preferences update failed: {str(e)}")
        flash(f'Error updating preferences: {str(e)}', 'error')
        db.session.rollback()
    
    return redirect(url_for('device_detail', device_id=device_id))

# OTA Management Web Interface
@app.route('/ota-management')
def ota_management():
    """OTA firmware management page"""
    try:
        # Get all firmware versions
        firmware_versions = ota_manager.list_firmware_versions()
        
        # Get OTA statistics
        stats = ota_manager.get_update_statistics()
        
        # Get devices that support OTA
        ota_devices = Device.query.filter(
            Device.device_type.in_(['ESP32_PersonalCMS', 'ESP32_OTA_Base', 'ESP32_LED_Blink'])
        ).all()
        
        return render_template('ota_management.html', 
                             firmware_versions=firmware_versions,
                             stats=stats,
                             ota_devices=ota_devices)
    except Exception as e:
        logger.error(f"OTA management error: {str(e)}")
        flash(f'Error loading OTA management: {str(e)}', 'error')
        return redirect(url_for('index'))

@app.route('/ota-upload', methods=['GET', 'POST'])
def ota_upload():
    """OTA firmware upload page"""
    if request.method == 'POST':
        try:
            print("=== DEBUG: Upload request received ===")
            print(f"Files: {list(request.files.keys())}")
            print(f"Form: {dict(request.form)}")
            
            if 'firmware' not in request.files:
                print("ERROR: No firmware file in request")
                flash('No firmware file selected', 'error')
                return redirect(url_for('ota_upload'))
            
            file = request.files['firmware']
            if file.filename == '':
                print("ERROR: Empty filename")
                flash('No firmware file selected', 'error')
                return redirect(url_for('ota_upload'))
            
            version = request.form.get('version')
            device_type = request.form.get('device_type')
            description = request.form.get('description', '')
            auto_assign = 'auto_assign' in request.form
            
            print(f"Upload params - version: {version}, device_type: {device_type}, file: {file.filename}")
            
            if not version or not device_type:
                print(f"ERROR: Missing required fields - version: {version}, device_type: {device_type}")
                flash('Version and device type are required', 'error')
                return redirect(url_for('ota_upload'))
            
            # Upload firmware
            print("Calling ota_manager.upload_firmware")
            result = ota_manager.upload_firmware(file, version, device_type, description, auto_assign)
            print(f"Upload result: {result}")
            
            if result['success']:
                flash(f'Firmware uploaded successfully: {device_type} v{version}', 'success')
                print(f"Upload successful: {device_type} v{version}")
                return redirect(url_for('ota_management'))
            else:
                flash(f'Upload failed: {result["error"]}', 'error')
                print(f"Upload failed: {result['error']}")
                return redirect(url_for('ota_upload'))
                
        except Exception as e:
            print(f"EXCEPTION in upload: {str(e)}")
            import traceback
            traceback.print_exc()
            flash(f'Upload error: {str(e)}', 'error')
            return redirect(url_for('ota_upload'))
    
    # GET request - show upload form
    return render_template('ota_upload_test.html')

@app.route('/ota-device/<device_id>')
def ota_device_detail(device_id):
    """OTA device detail page"""
    try:
        device = Device.query.filter_by(device_id=device_id).first_or_404()
        
        # Get available firmware for this device type
        available_firmware = ota_manager.list_firmware_versions(device.device_type or 'ESP32_PersonalCMS')
        
        # Get device's current firmware info
        current_version = getattr(device, 'firmware_version', 'Unknown')
        
        return render_template('ota_device_detail.html', 
                             device=device,
                             available_firmware=available_firmware,
                             current_version=current_version)
    except Exception as e:
        logger.error(f"OTA device detail error: {str(e)}")
        flash(f'Error loading device: {str(e)}', 'error')
        return redirect(url_for('ota_management'))

# Error handlers
@app.errorhandler(404)
def not_found_error(error):
    """Handle 404 errors"""
    return render_template('404.html'), 404

@app.errorhandler(500)
def internal_error(error):
    """Handle 500 errors"""
    db.session.rollback()
    return render_template('500.html'), 500

# Import additional route definitions (after basic routes are set up)
# import unified_web
# import unified_routes

# Main application
if __name__ == '__main__':
    print("🚀 PersonalCMS - Unified Per-Device Content Management System")
    print("📱 Features:")
    print("   ✅ Per-device content management")
    print("   ✅ Nickname support") 
    print("   ✅ Default + custom content")
    print("   ✅ Server-managed overrides")
    print("🌐 Server starting on http://0.0.0.0:5000/")
    app.run(host='0.0.0.0', port=5000, debug=True)

