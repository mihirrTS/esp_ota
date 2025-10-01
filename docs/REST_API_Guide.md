# PersonalCMS REST API & Subcategories Guide

## 🚀 Overview

PersonalCMS now features a comprehensive REST API with subcategorized content system. Instead of choosing between simple categories like "jokes", "riddles", and "news", users can now select specific subcategories within each main category.

## 📊 Content Structure

### Main Categories with Subcategories

#### 😄 Jokes
- **Dad Jokes** (`dad_jokes`) - Classic dad humor and puns
- **Programming Jokes** (`programming_jokes`) - Tech and coding humor  
- **Clean Jokes** (`clean_jokes`) - Family-friendly humor

#### 🧩 Riddles
- **Easy Riddles** (`easy_riddles`) - Simple brain teasers
- **Brain Teasers** (`brain_teasers`) - Medium difficulty puzzles
- **Logic Puzzles** (`logic_puzzles`) - Advanced logical challenges

#### 📰 News
- **Tech News** (`tech_news`) - Technology and innovation updates
- **Science News** (`science_news`) - Scientific discoveries and research
- **Environment News** (`environment_news`) - Climate and environmental updates

## 🛠 REST API Endpoints

### Device Management

#### Get All Devices
```http
GET /api/devices
```
Returns list of all registered devices with their preferences and last seen timestamps.

#### Get Specific Device
```http
GET /api/devices/{device_id}
```
Returns detailed information about a specific device.

#### Device Ping
```http
POST /api/devices/{device_id}/ping
```
Updates device's last seen timestamp and creates device if it doesn't exist.

### Device Preferences

#### Get Device Preferences
```http
GET /api/devices/{device_id}/preferences
```
Returns current device preferences with selected subcategories.

#### Update Device Preferences
```http
POST /api/devices/{device_id}/preferences
Content-Type: application/json

{
  "jokes": ["dad_jokes", "programming_jokes"],
  "riddles": ["brain_teasers"],
  "news": ["tech_news", "science_news"],
  "layout": "default"
}
```
Updates device preferences with new subcategory selections.

### Content & Dashboard

#### Get Device Content
```http
GET /api/devices/{device_id}/content
```
Returns content filtered by device's selected subcategories.

#### Generate Dashboard
```http
GET /api/devices/{device_id}/dashboard
```
Generates and returns BMP dashboard image (800x480) for e-ink display.

#### Get Available Categories
```http
GET /api/categories
```
Returns all available categories and subcategories with item counts.

## 📱 Mobile Interface

Each device has a dedicated mobile interface for customer configuration:

```
http://your-server:5000/mobile/{device_id}
```

Features:
- 🎨 Modern dark UI with mobile-responsive design
- 📋 Subcategory selection within each main category
- 💾 Real-time preference updates
- 🔄 Automatic dashboard regeneration

## 🔧 ESP32 Integration

### Device IDs
- `ESP32S3_Device1` - First ESP32-S3 device
- `ESP32S3_Device2` - Second ESP32-S3 device

### API Usage in ESP32 Code
```cpp
// Device ping (every 5 minutes)
POST /api/devices/ESP32S3_Device1/ping

// Get dashboard image
GET /api/devices/ESP32S3_Device1/dashboard
```

### Hardware Setup
- **Display**: 800x480 e-ink display (GxEPD2_750_T7)
- **WiFi**: Pre-configured (Mihirr/12345678)
- **Server**: http://192.168.120.63:5000

## 🧪 Testing

Run the comprehensive API test suite:

```bash
python test_rest_api.py
```

This tests all endpoints including:
- ✅ Category retrieval
- ✅ Device ping and registration
- ✅ Preference updates with subcategories
- ✅ Content filtering
- ✅ Dashboard generation
- ✅ Device management

## 📋 Example Usage

### 1. Register Device
```bash
curl -X POST http://localhost:5000/api/devices/MyDevice/ping
```

### 2. Set Preferences
```bash
curl -X POST http://localhost:5000/api/devices/MyDevice/preferences \
  -H "Content-Type: application/json" \
  -d '{
    "jokes": ["dad_jokes", "clean_jokes"],
    "riddles": ["easy_riddles"],
    "news": ["tech_news"],
    "layout": "default"
  }'
```

### 3. Get Dashboard
```bash
curl http://localhost:5000/api/devices/MyDevice/dashboard \
  --output MyDevice_dashboard.bmp
```

## 🎯 Key Improvements

### From Simple Categories to Subcategories
**Before:**
```json
{
  "categories": ["jokes", "news"],
  "layout": "default"
}
```

**Now:**
```json
{
  "jokes": ["dad_jokes", "programming_jokes"],
  "riddles": [],
  "news": ["tech_news", "science_news"],
  "layout": "default"
}
```

### Enhanced Mobile Interface
- Organized subcategory selection
- Visual category grouping
- Item count display
- Responsive design for mobile devices

### Robust Device Management
- Automatic device registration on first ping
- Device-specific content filtering
- Real-time preference updates
- Comprehensive device tracking

## 🚀 Getting Started

1. **Start the server:**
   ```bash
   python app.py
   ```

2. **Configure a device via mobile interface:**
   ```
   http://localhost:5000/mobile/your_device_id
   ```

3. **Test the API:**
   ```bash
   python test_rest_api.py
   ```

4. **Upload ESP32 code** to your ESP32-S3 with e-ink display

5. **Monitor devices** via the main dashboard at `http://localhost:5000`

The system now provides granular content control, robust device management, and a modern mobile interface for customer configuration!