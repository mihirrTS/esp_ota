# ESP32 HTTPUpdate API Fix

## Issue
The ESP32 HTTPUpdate library has changed its API in newer versions. The old `httpUpdate.update(String url)` method is no longer available.

## Error Message
```
error: no matching function for call to 'HTTPUpdate::update(String&)'
```

## Solution
Updated all ESP32 firmware files to use the new HTTPUpdate API that requires an HTTPClient object:

### Before (Old API)
```cpp
HTTPUpdate httpUpdate;
httpUpdate.setAuthorization(username, password);
t_httpUpdate_return ret = httpUpdate.update(firmwareURL);
```

### After (New API)
```cpp
HTTPClient httpClient;
HTTPUpdate httpUpdate;

httpClient.begin(firmwareURL);
httpClient.setTimeout(30000); // 30 second timeout
httpClient.setAuthorization(username, password);

t_httpUpdate_return ret = httpUpdate.update(httpClient, "");
```

## Files Updated
1. `custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_OTA_Base/ESP32_DevKit_V1_OTA_Base.ino`
2. `custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_PersonalCMS.ino`
3. `custom_driver/ESP32_DevKit_V1_PersonalCMS/ESP32_DevKit_V1_LED_Blink.ino`

## Changes Made
- Added HTTPClient object initialization
- Moved authentication setup from HTTPUpdate to HTTPClient
- Added 30-second timeout for HTTP requests
- Updated httpUpdate.update() call to use HTTPClient object
- Maintained all existing callbacks and functionality

## Compatibility
- Works with ESP32 Core 3.x and newer
- Backward compatible with all existing OTA functionality
- No changes needed to server-side implementation