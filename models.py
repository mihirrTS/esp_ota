"""
Complete models and shared logic for PersonalCMS
"""
import logging
import random
import json
import os
import requests
from datetime import datetime, timedelta
from PIL import Image, ImageDraw, ImageFont
from flask_sqlalchemy import SQLAlchemy
from typing import Dict

# Initialize SQLAlchemy
db = SQLAlchemy()

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Check for JSONPath availability
try:
    from jsonpath_ng import parse as jsonpath_parse
    JSONPATH_AVAILABLE = True
except ImportError:
    JSONPATH_AVAILABLE = False
    jsonpath_parse = None


# Database Models
class Device(db.Model):
    __tablename__ = 'devices'
    
    id = db.Column(db.Integer, primary_key=True)
    device_id = db.Column(db.String(255), unique=True, nullable=False, index=True)
    device_name = db.Column(db.String(255), nullable=False)
    occupation = db.Column(db.String(255), nullable=False)
    device_type = db.Column(db.String(100), nullable=False)
    wifi_ssid = db.Column(db.String(255))
    preferences = db.Column(db.Text)  # JSON string
    nickname = db.Column(db.String(255))
    is_active = db.Column(db.Boolean, default=False, index=True)
    is_connected = db.Column(db.Boolean, default=False)
    last_seen = db.Column(db.DateTime, index=True)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)
    custom_content_enabled = db.Column(db.Boolean, default=False)
    
    # Sensor data fields
    temperature = db.Column(db.Float, default=0.0)
    humidity = db.Column(db.Float, default=0.0)
    motion_detected = db.Column(db.Boolean, default=False)
    sleep_mode = db.Column(db.Boolean, default=False)
    sensor_last_update = db.Column(db.DateTime)
    
    # OTA fields
    firmware_version = db.Column(db.String(50))
    uptime_seconds = db.Column(db.Integer, default=0)
    free_heap = db.Column(db.Integer, default=0)
    wifi_rssi = db.Column(db.Integer, default=0)
    
    # Relationship
    device_contents = db.relationship('DeviceContent', backref='device', lazy='dynamic')
    
    def to_dict(self):
        return {
            'id': self.id,
            'device_id': self.device_id,
            'device_name': self.device_name,
            'occupation': self.occupation,
            'device_type': self.device_type,
            'wifi_ssid': self.wifi_ssid,
            'preferences': json.loads(self.preferences) if self.preferences else {},
            'nickname': self.nickname,
            'is_active': self.is_active,
            'is_connected': self.is_connected,
            'last_seen': self.last_seen.isoformat() if self.last_seen else None,
            'created_at': self.created_at.isoformat() if self.created_at else None,
            'updated_at': self.updated_at.isoformat() if self.updated_at else None,
            'custom_content_enabled': self.custom_content_enabled,
            'temperature': self.temperature,
            'humidity': self.humidity,
            'motion_detected': self.motion_detected,
            'sleep_mode': self.sleep_mode,
            'sensor_last_update': self.sensor_last_update.isoformat() if self.sensor_last_update else None,
            'firmware_version': self.firmware_version,
            'uptime_seconds': self.uptime_seconds,
            'free_heap': self.free_heap,
            'wifi_rssi': self.wifi_rssi
        }


class DeviceContent(db.Model):
    __tablename__ = 'device_content'
    
    id = db.Column(db.Integer, primary_key=True)
    device_id = db.Column(db.String(255), db.ForeignKey('devices.device_id'), nullable=False)
    content_type = db.Column(db.String(100), nullable=False)
    subcategory = db.Column(db.String(100))
    title = db.Column(db.String(500))
    content = db.Column(db.Text, nullable=False)
    priority = db.Column(db.Integer, default=0)
    expires_at = db.Column(db.DateTime)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)


class UserImage(db.Model):
    __tablename__ = 'user_images'
    
    id = db.Column(db.Integer, primary_key=True)
    filename = db.Column(db.String(255), nullable=False)
    filepath = db.Column(db.String(500), nullable=False)
    file_size = db.Column(db.Integer, default=0)  # File size in bytes
    device_assignments = db.Column(db.Text, default='[]')  # JSON array of device IDs
    uploaded_at = db.Column(db.DateTime, default=datetime.utcnow)


class ContentAPI(db.Model):
    __tablename__ = 'content_apis'
    
    id = db.Column(db.Integer, primary_key=True)
    category = db.Column(db.String(100), nullable=False)
    subcategory = db.Column(db.String(100), nullable=False)
    api_url = db.Column(db.String(500), nullable=False)
    api_key = db.Column(db.String(500))
    headers = db.Column(db.Text)  # JSON string
    response_path = db.Column(db.String(255))  # JSONPath
    is_active = db.Column(db.Boolean, default=True)
    success_count = db.Column(db.Integer, default=0)
    error_count = db.Column(db.Integer, default=0)
    last_fetched = db.Column(db.DateTime)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)


class ContentSource(db.Model):
    __tablename__ = 'content_sources'
    
    id = db.Column(db.Integer, primary_key=True)
    category = db.Column(db.String(100), nullable=False)
    subcategory = db.Column(db.String(100), nullable=False)
    source_type = db.Column(db.String(50), nullable=False)  # 'api' or 'preset'
    created_at = db.Column(db.DateTime, default=datetime.utcnow)


class DefaultContent(db.Model):
    __tablename__ = 'default_content'
    
    id = db.Column(db.Integer, primary_key=True)
    category = db.Column(db.String(100), nullable=False, index=True)
    subcategory = db.Column(db.String(100), nullable=False, index=True)
    content = db.Column(db.Text, nullable=False)
    title = db.Column(db.String(500))
    question = db.Column(db.Text)  # For riddles
    answer = db.Column(db.Text)   # For riddles
    url = db.Column(db.String(500))  # For news items
    is_active = db.Column(db.Boolean, default=True, index=True)
    created_at = db.Column(db.DateTime, default=datetime.utcnow, index=True)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)
    
    def to_dict(self):
        return {
            'id': self.id,
            'category': self.category,
            'subcategory': self.subcategory,
            'content': self.content,
            'title': self.title,
            'question': self.question,
            'answer': self.answer,
            'url': self.url,
            'is_active': self.is_active,
            'created_at': self.created_at.isoformat() if self.created_at else None,
            'updated_at': self.updated_at.isoformat() if self.updated_at else None
        }


# Content Management System
class PerDeviceCMS:
    def __init__(self):
        self.default_content_sources = {
            'jokes': {
                'dad_jokes': self._get_dad_jokes,
                'programming_jokes': self._get_programming_jokes,
            },
            'riddles': {
                'easy_riddles': self._get_easy_riddles,
                'math_riddles': self._get_math_riddles,
                'logic_riddles': self._get_logic_riddles,
            },
            'news': {
                'tech_news': self._get_tech_news,
                'sports_news': self._get_sports_news,
                'world_news': self._get_world_news,
            }
        }
    
    def get_content_for_device(self, device: Device):
        """Get default content based on device preferences and content sources"""
        if not device.preferences:
            return {}
        
        try:
            prefs = json.loads(device.preferences)
        except:
            return {}
        
        content = {}
        
        for main_category, subcategories in prefs.items():
            if main_category in self.default_content_sources:
                content[main_category] = {}
                for subcategory in subcategories:
                    # Check if this category/subcategory should use API source
                    content_source = ContentSource.query.filter_by(
                        category=main_category,
                        subcategory=subcategory
                    ).first()
                    
                    if content_source and content_source.source_type == 'api':
                        # Try to fetch from API first
                        api_content = self._get_api_content(main_category, subcategory)
                        if api_content:
                            content[main_category][subcategory] = api_content
                            continue
                    
                    # Fall back to preset content
                    if subcategory in self.default_content_sources[main_category]:
                        content[main_category][subcategory] = self.default_content_sources[main_category][subcategory]()
                    else:
                        # If subcategory doesn't exist in preset, try to get from default content
                        default_items = self._get_default_content_items(main_category, subcategory)
                        if default_items:
                            content[main_category][subcategory] = default_items
        
        return content
    
    def add_custom_content(self, device_id: str, content_type: str, subcategory: str, 
                          title: str, content: str, priority: int = 0, expires_at: datetime = None):
        """Add custom content for a specific device"""
        device_content = DeviceContent(
            device_id=device_id,
            content_type=content_type,
            subcategory=subcategory,
            title=title,
            content=content,
            priority=priority,
            expires_at=expires_at
        )
        
        db.session.add(device_content)
        db.session.commit()
        
        logger.info(f"Custom content added for device {device_id}: {title}")
        return device_content
    
    def _get_api_content(self, category: str, subcategory: str):
        """Fetch content from API endpoints"""
        try:
            # Find API endpoint for this category/subcategory
            api_endpoint = ContentAPI.query.filter_by(
                category=category,
                subcategory=subcategory
            ).first()
            
            if not api_endpoint:
                return None
            
            # Prepare headers
            headers = {}
            if api_endpoint.headers:
                try:
                    headers = json.loads(api_endpoint.headers)
                except:
                    pass
            
            # Add API key if available
            if api_endpoint.api_key:
                if api_endpoint.api_key.startswith('Bearer '):
                    headers['Authorization'] = api_endpoint.api_key
                else:
                    headers['Authorization'] = f'Bearer {api_endpoint.api_key}'
            
            # Make API request
            response = requests.get(api_endpoint.api_url, headers=headers, timeout=10)
            response.raise_for_status()
            
            data = response.json()
            
            # Extract content using JSONPath if specified
            if api_endpoint.response_path and JSONPATH_AVAILABLE:
                jsonpath_expr = jsonpath_parse(api_endpoint.response_path)
                matches = [match.value for match in jsonpath_expr.find(data)]
                content = matches[:10]  # Limit to 10 items
            else:
                # Try to extract content from common response structures
                if isinstance(data, list):
                    content = data[:10]
                elif isinstance(data, dict):
                    if 'data' in data and isinstance(data['data'], list):
                        content = data['data'][:10]
                    elif 'items' in data and isinstance(data['items'], list):
                        content = data['items'][:10]
                    else:
                        content = [data]
                else:
                    content = [str(data)]
            
            # Update API statistics
            api_endpoint.success_count += 1
            api_endpoint.last_fetched = datetime.utcnow()
            db.session.commit()
            
            logger.info(f"Successfully fetched {len(content)} items from API for {category}/{subcategory}")
            return content
            
        except Exception as e:
            logger.error(f"Error fetching API content for {category}/{subcategory}: {str(e)}")
            if 'api_endpoint' in locals():
                api_endpoint.error_count += 1
                db.session.commit()
            return None
    
    def _get_default_content_items(self, category: str, subcategory: str):
        """Get default content items from database for given category/subcategory"""
        try:
            # Query DefaultContent model for matching items
            items = DefaultContent.query.filter_by(
                category=category,
                subcategory=subcategory,
                is_active=True
            ).order_by(DefaultContent.created_at.desc()).limit(10).all()
            
            if not items:
                # Try to get items from the same category but default subcategory
                items = DefaultContent.query.filter_by(
                    category=category,
                    subcategory='default',
                    is_active=True
                ).order_by(DefaultContent.created_at.desc()).limit(10).all()
            
            # Convert to format expected by content system
            content_items = []
            for item in items:
                if category == 'jokes':
                    content_items.append(item.content)
                elif category == 'riddles':
                    content_items.append({
                        'question': item.question or item.content,
                        'answer': item.answer or 'No answer provided'
                    })
                elif category == 'news':
                    content_items.append({
                        'title': item.title or 'No title',
                        'content': item.content,
                        'url': item.url
                    })
                else:
                    content_items.append(item.content)
            
            return content_items
            
        except Exception as e:
            logger.error(f"Error fetching default content for {category}/{subcategory}: {str(e)}")
            return []
    
    # Default content generators
    def _get_dad_jokes(self, count=5):
        jokes = [
            "Why don't scientists trust atoms? Because they make up everything!",
            "I invented a new word: Plagiarism!",
            "Why don't eggs tell jokes? They'd crack each other up!",
            "What do you call a fake noodle? An impasta!",
            "Why did the coffee file a police report? It got mugged!",
            "What's the best thing about Switzerland? I don't know, but the flag is a big plus!",
            "Why don't skeletons fight each other? They don't have the guts!",
            "What do you call a bear with no teeth? A gummy bear!"
        ]
        return random.sample(jokes, min(count, len(jokes)))
    
    def _get_programming_jokes(self, count=5):
        jokes = [
            "Why do programmers prefer dark mode? Because light attracts bugs!",
            "A SQL query walks into a bar, walks up to two tables and asks: 'Can I join you?'",
            "Why do Java developers wear glasses? Because they can't C#!",
            "How many programmers does it take to change a light bulb? None, that's a hardware problem!",
            "Why did the programmer quit his job? He didn't get arrays!",
            "What's a programmer's favorite hangout place? Foo Bar!"
        ]
        return random.sample(jokes, min(count, len(jokes)))
    
    def _get_easy_riddles(self, count=3):
        riddles = [
            {"question": "What has keys but no locks?", "answer": "A piano"},
            {"question": "What gets wet while drying?", "answer": "A towel"},
            {"question": "What can you catch but not throw?", "answer": "A cold"},
            {"question": "What has hands but can't clap?", "answer": "A clock"}
        ]
        return random.sample(riddles, min(count, len(riddles)))
    
    def _get_math_riddles(self, count=3):
        riddles = [
            {"question": "If you have 3 apples and you take away 2, how many do you have?", "answer": "2 (the ones you took)"},
            {"question": "What comes next: 2, 3, 5, 7, 11, __?", "answer": "13 (prime numbers)"},
            {"question": "I'm thinking of a number. If you multiply it by 2 and add 10, you get 20. What's the number?", "answer": "5"}
        ]
        return random.sample(riddles, min(count, len(riddles)))
    
    def _get_logic_riddles(self, count=3):
        riddles = [
            {"question": "What can you hold in your right hand but not in your left?", "answer": "Your left hand"},
            {"question": "What gets bigger the more you take away from it?", "answer": "A hole"},
            {"question": "The more you take, the more you leave behind. What am I?", "answer": "Footsteps"}
        ]
        return random.sample(riddles, min(count, len(riddles)))
    
    def _get_tech_news(self, count=3):
        news = [
            {"title": "AI Breakthrough in Medical Diagnosis", "summary": "New AI system achieves 95% accuracy in early disease detection...", "published": datetime.now().strftime("%Y-%m-%d")},
            {"title": "Quantum Computing Milestone Reached", "summary": "Scientists demonstrate quantum supremacy in cryptography applications...", "published": datetime.now().strftime("%Y-%m-%d")},
            {"title": "Sustainable Tech Solutions Emerge", "summary": "New green technology reduces data center energy consumption by 40%...", "published": datetime.now().strftime("%Y-%m-%d")}
        ]
        return news[:count]
    
    def _get_sports_news(self, count=3):
        news = [
            {"title": "Championship Finals Set", "summary": "Teams prepare for the ultimate showdown...", "published": datetime.now().strftime("%Y-%m-%d")},
            {"title": "Record Breaking Performance", "summary": "Athlete sets new world record...", "published": datetime.now().strftime("%Y-%m-%d")}
        ]
        return news[:count]
    
    def _get_world_news(self, count=3):
        news = [
            {"title": "Global Climate Summit", "summary": "World leaders gather to discuss climate action...", "published": datetime.now().strftime("%Y-%m-%d")},
            {"title": "Economic Recovery Signs", "summary": "Markets show positive indicators...", "published": datetime.now().strftime("%Y-%m-%d")}
        ]
        return news[:count]


# Image Processing
class ImageProcessor:
    @staticmethod
    def convert_to_bmp(image_path: str, output_path: str, size: tuple = (800, 480)) -> bool:
        """Convert uploaded image to monochrome BMP format for ESP32 display with dithering"""
        try:
            with Image.open(image_path) as img:
                # Convert to RGB first if needed
                if img.mode != 'RGB':
                    img = img.convert('RGB')
                # Resize the image
                img = img.resize(size, Image.Resampling.LANCZOS)
                # Convert to grayscale
                img = img.convert('L')
                # Convert to 1-bit monochrome with Floyd-Steinberg dithering
                img = img.convert('1', dither=Image.Dither.FLOYDSTEINBERG)
                # Save as BMP
                img.save(output_path, 'BMP')
                return True
        except Exception as e:
            logger.error(f"Image conversion failed: {e}")
            return False
    
    @staticmethod
    def generate_dashboard(device: Device, content: Dict, size: tuple = (800, 480), app_config=None) -> str:
        """Generate monochrome dashboard image for device"""
        # Create monochrome image (1-bit black and white)
        img = Image.new('1', size, 1)  # 1 = white background in 1-bit mode
        draw = ImageDraw.Draw(img)
        
        try:
            title_font = ImageFont.load_default()
            content_font = ImageFont.load_default()
        except:
            title_font = ImageFont.load_default()
            content_font = ImageFont.load_default()
        
        y = 20
        
        # Header with nickname support
        display_name = device.nickname or device.device_name
        draw.text((20, y), f"PersonalCMS - {display_name}", fill=0, font=title_font)  # 0 = black in 1-bit mode
        y += 40
        
        if device.nickname:
            draw.text((20, y), f"Device: {device.device_name}", fill=0, font=content_font)
            y += 20
        
        draw.text((20, y), f"Occupation: {device.occupation}", fill=0, font=content_font)
        y += 30
        
        # Sensor data section (show if any temperature data exists, even 0)
        logger.info(f"🌡️  Dashboard generation - Temperature: {device.temperature} (type: {type(device.temperature)}), Humidity: {device.humidity} (type: {type(device.humidity)}), sensor_last_update: {device.sensor_last_update}")
        
        # Show sensor data if temperature is not None (including 0.0)
        if device.temperature is not None:
            logger.info("✅ Adding sensor data section to dashboard")
            draw.text((20, y), "SENSOR DATA:", fill=0, font=title_font)
            y += 25
            
            # Temperature (always show if not None, including 0.0)
            temp_text = f"• Temperature: {device.temperature:.1f}°C"
            logger.info(f"📊 Adding temperature text: '{temp_text}'")
            draw.text((40, y), temp_text, fill=0, font=content_font)
            y += 20
                
            # Humidity (show if not None and >= 0)
            if device.humidity is not None and device.humidity >= 0:
                humidity_text = f"• Humidity: {device.humidity:.1f}%"
                logger.info(f"💧 Adding humidity text: '{humidity_text}'")
                draw.text((40, y), humidity_text, fill=0, font=content_font)
                y += 20
            else:
                logger.info(f"💧 Skipping humidity - humidity: {device.humidity}")
                
            # Motion and sleep status
            motion_status = "Motion Detected" if device.motion_detected else "No Motion"
            draw.text((40, y), f"• Status: {motion_status}", fill=0, font=content_font)
            y += 20
            
            if device.sleep_mode:
                draw.text((40, y), "• Mode: Sleep Mode", fill=0, font=content_font)
                y += 20
        else:
            logger.warning(f"❌ Skipping sensor data section - temperature is None")
                
            # Sensor update time
            if device.sensor_last_update:
                sensor_time = device.sensor_last_update.strftime('%H:%M')
                draw.text((40, y), f"• Last Update: {sensor_time}", fill=0, font=content_font)
            else:
                draw.text((40, y), "• Last Update: Never", fill=0, font=content_font)
            y += 30
        
        # Custom content indicator
        if device.custom_content_enabled:
            draw.text((20, y), "Custom Content Mode", fill=0, font=content_font)
            y += 20
        
        draw.text((20, y), f"Updated: {datetime.now().strftime('%Y-%m-%d %H:%M')}", fill=0, font=content_font)
        y += 40
        
        # Content sections
        for category, subcategories in content.items():
            if y > size[1] - 100:
                break
                
            draw.text((20, y), f"{category.upper()}:", fill=0, font=title_font)  # 0 = black
            y += 25
            
            for subcategory, items in subcategories.items():
                if y > size[1] - 60:
                    break
                    
                draw.text((40, y), f"• {subcategory.replace('_', ' ').title()}", fill=0, font=content_font)
                y += 20
                
                # Show first item
                if items and len(items) > 0:
                    first_item = items[0]
                    if isinstance(first_item, dict):
                        text = first_item.get('title', first_item.get('question', first_item.get('content', str(first_item))))
                    else:
                        text = str(first_item)
                    
                    if len(text) > 60:
                        text = text[:60] + "..."
                    
                    draw.text((60, y), text[:80], fill=0, font=content_font)  # 0 = black
                    y += 20
        
        # Save dashboard as monochrome BMP in dashboards folder
        dashboard_folder = app_config.get('DASHBOARD_FOLDER', 'dashboards') if app_config else 'dashboards'
        if not os.path.exists(dashboard_folder):
            os.makedirs(dashboard_folder, exist_ok=True)
            
        # Create both current and fallback versions
        current_path = os.path.join(dashboard_folder, f"{device.device_id}_current.bmp")
        fallback_path = os.path.join(dashboard_folder, f"{device.device_id}_fallback.bmp")
        
        img.save(current_path, 'BMP')
        img.save(fallback_path, 'BMP')  # Same image for both for now
        
        return current_path

    @staticmethod
    def generate_fallback(device: Device, size: tuple = (800, 480), app_config=None) -> str:
        """Generate a per-device fallback monochrome BMP with simple status text"""
        # Create monochrome image (1-bit black and white)
        img = Image.new('1', size, 1)  # 1 = white background in 1-bit mode
        draw = ImageDraw.Draw(img)
        try:
            title_font = ImageFont.load_default()
            content_font = ImageFont.load_default()
        except:
            title_font = ImageFont.load_default()
            content_font = ImageFont.load_default()

        y = 60
        display_name = device.nickname or device.device_name
        draw.text((40, y), "PersonalCMS", fill=0, font=title_font)  # 0 = black
        y += 40
        draw.text((40, y), f"Device: {display_name}", fill=0, font=content_font)
        y += 25
        draw.text((40, y), "Fallback Screen", fill=0, font=content_font)
        y += 25
        draw.text((40, y), datetime.now().strftime('%Y-%m-%d %H:%M'), fill=0, font=content_font)

        dashboard_folder = app_config.get('DASHBOARD_FOLDER', 'dashboards') if app_config else 'dashboards'
        if not os.path.exists(dashboard_folder):
            os.makedirs(dashboard_folder, exist_ok=True)
            
        output_path = os.path.join(dashboard_folder, f"{device.device_id}_fallback.bmp")
        img.save(output_path, 'BMP')
        return output_path


# Utility functions
def refresh_device_activity_statuses(timeout_seconds: int = None) -> int:
    """Update Device.is_active based on last_seen age.

    Returns the number of devices updated. A device is considered active if
    last_seen is within the configured timeout window.
    """
    try:
        window = timeout_seconds or 300  # Default 5 minutes
        cutoff = datetime.utcnow() - timedelta(seconds=window)
        updated = 0
        devices = Device.query.all()
        for d in devices:
            was_active = bool(d.is_active)
            is_now_active = bool(d.last_seen and d.last_seen > cutoff)
            if was_active != is_now_active:
                d.is_active = is_now_active
                updated += 1
        if updated:
            db.session.commit()
        return updated
    except Exception:
        return 0