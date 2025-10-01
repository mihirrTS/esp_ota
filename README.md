# Enhanced PersonalCMS - Per-Device Content Management System

## 🎯 Overview
Enhanced PersonalCMS is a sophisticated device and content management platform that provides personalized content delivery to ESP32 devices with comprehensive web-based management capabilities.

## ✨ Key Features
- **Device Management**: Connect, disconnect, and remove devices with real-time status tracking
- **Content Sources**: Switch between preset content and API-driven automated content
- **API Integration**: Configure external APIs for automated jokes, riddles, and news
- **Default Content**: Manage preset content with easy editing interface
- **Device Preferences**: Per-device content customization and settings
- **Activity Monitoring**: Real-time device status with configurable timeout
- **Professional UI**: Bootstrap-based responsive web interface

## 🚀 Quick Start

### Prerequisites
- Python 3.8+
- Virtual environment recommended

### Installation & Startup
```powershell
# Install dependencies
python -m pip install -r requirements.txt

# Initialize database
python init_db.py

# Start server
python unified_web.py
# or use batch file
START_UNIFIED_CMS.bat
```

### Access Points
- **Web Interface**: http://localhost:5001
- **Device Management**: http://localhost:5001/device/{device_id}
- **Content Sources**: http://localhost:5001/content-sources
- **API Management**: http://localhost:5001/content-apis

## 🏗️ Architecture

### Core Files
- **`unified_cms.py`** - Core application with models and business logic
- **`unified_web.py`** - Web interface and main entry point  
- **`unified_routes.py`** - API routes for device communication
- **`dynamic_content.py`** - Content generation system

### Key Endpoints
- `GET /api/devices/{device_id}/content` – personalized content JSON
- `GET /api/devices/{device_id}/dashboard` – generated dashboard BMP
- `GET /api/devices/{device_id}/image` – device image (BMP)
- `POST /api/devices/register` – device registration
- `GET /content-sources` – content source management UI
- `GET /content-apis` – API endpoint management UI

## 🎮 New Enhanced Features

### Device Management
- **Real-time Status**: Active/inactive monitoring with configurable timeout
- **Connection Control**: Manual disconnect/reconnect devices
- **Device Removal**: Complete device deletion with confirmation
- **Preferences Editing**: Update device settings and content preferences

### Content Automation
- **API Integration**: External APIs for jokes, riddles, news with JSONPath parsing
- **Source Switching**: Toggle between preset and API content per category
- **Fallback System**: Automatic fallback to preset content when APIs fail
- **Content Testing**: Live API endpoint testing and validation

### Management Interface
- **Tabbed Content Editor**: Organized interface for different content types
- **Bootstrap UI**: Modern, responsive design with comprehensive controls
- **Real-time Updates**: Dynamic status indicators and live content preview
- **Batch Operations**: Bulk content management capabilities

## 📡 ESP32 Integration

### Device Registration
```http
POST /api/devices/register
{
  "device_id": "ESP32-001",
  "device_name": "Living Room Display",
  "nickname": "TV Display",
  "occupation": "entertainment",
  "device_type": "ESP32"
}
```

### Content Retrieval
```http
GET /api/devices/{device_id}/content
```

## 🔧 Configuration

### Environment Variables
- `DEVICE_ACTIVE_TIMEOUT_SECONDS`: Device activity timeout (default: 300)
- `UPLOAD_FOLDER`: File upload directory
- `SECRET_KEY`: Flask secret key

### Database Models
- **Device**: Device registration with connection status
- **DeviceContent**: Custom content per device
- **ContentAPI**: External API endpoint configuration
- **ContentSource**: Content source preferences (preset vs API)
- **UserImage**: Uploaded images and media

## 📂 Project Structure
```
personalised_cms/
├── unified_cms.py          # Core models and business logic
├── unified_web.py          # Web interface (MAIN ENTRY POINT)
├── unified_routes.py       # API routes for devices
├── START_UNIFIED_CMS.bat   # Windows startup script
├── init_db.py             # Database initialization
├── requirements.txt        # Dependencies
│
├── templates/             # Web interface templates
├── static/               # CSS, JS, assets  
├── uploads/              # User uploads
├── instance/            # Database files
├── arduino/             # ESP32 firmware
└── docs/                # Documentation
```

## 🎉 Recent Enhancements (v2.0)

### Completed Features
- ✅ Complete device lifecycle management
- ✅ API-driven content automation with JSONPath support
- ✅ Professional Bootstrap-based responsive UI
- ✅ Real-time device activity monitoring
- ✅ Content source switching (preset ↔ API)
- ✅ Comprehensive error handling and fallback systems
- ✅ Database schema enhancements with new models

### Technology Stack
- **Backend**: Flask 3.0, SQLAlchemy, SQLite
- **Frontend**: Bootstrap 5, Jinja2 templates
- **API Integration**: requests, jsonpath-ng
- **Image Processing**: Pillow (PIL)
- **Device Communication**: REST API with ESP32

## 💡 Usage Examples

### Managing Content Sources
1. Navigate to **Content Sources** to switch between preset and API content
2. Configure **API Endpoints** for automated content delivery
3. Edit **Default Content** for fallback scenarios
4. Test API endpoints before activation

### Device Operations
1. View all devices with real-time connection status
2. Disconnect devices manually without removing data
3. Edit device preferences and content categories
4. Remove devices completely when decommissioned

---

**Enhanced PersonalCMS** - Intelligent device and content management platform 🚀