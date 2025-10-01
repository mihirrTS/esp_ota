#!/usr/bin/env powershell
# Quick upload test script

Write-Host "🚀 Testing firmware upload..." -ForegroundColor Green

# Test server connectivity first
$serverUrl = "http://127.0.0.1:5000"
try {
    Write-Host "🔗 Testing server connectivity..." -ForegroundColor Yellow
    $response = Invoke-WebRequest -Uri $serverUrl -TimeoutSec 5 -ErrorAction Stop
    Write-Host "✅ Server responding: $($response.StatusCode)" -ForegroundColor Green
} catch {
    Write-Host "❌ Server not accessible: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "💡 Make sure the server is running at $serverUrl" -ForegroundColor Cyan
    exit 1
}

# Find firmware file
$firmwareFiles = @(
    "custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_PersonalCMS\bin\ESP32_DevKit_V1_PersonalCMS_v2.0.bin",
    "custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_OTA_Base\bin\ESP32_DevKit_V1_OTA_Base_v2.0.bin",
    "custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_LED_Blink\bin\ESP32_DevKit_V1_LED_Blink_v2.0.bin"
)

$firmwareFile = $null
foreach ($fw in $firmwareFiles) {
    if (Test-Path $fw) {
        $firmwareFile = $fw
        break
    }
}

if (-not $firmwareFile) {
    Write-Host "❌ No firmware file found!" -ForegroundColor Red
    Write-Host "Available files:" -ForegroundColor Yellow
    foreach ($fw in $firmwareFiles) {
        $exists = if (Test-Path $fw) { "✓" } else { "✗" }
        Write-Host "  $exists $fw" -ForegroundColor Gray
    }
    exit 1
}

Write-Host "📁 Using firmware file: $firmwareFile" -ForegroundColor Cyan
$fileSize = (Get-Item $firmwareFile).Length
Write-Host "📊 File size: $fileSize bytes" -ForegroundColor Cyan

# Create form data for upload
$uploadUrl = "$serverUrl/ota-upload"
Write-Host "📤 Uploading to $uploadUrl..." -ForegroundColor Yellow

try {
    # Use curl for multipart upload
    $curlCmd = "curl -X POST -F `"firmware=@$firmwareFile`" -F `"version=2.0`" -F `"device_type=ESP32_OTA_Base`" -F `"description=PowerShell test upload`" $uploadUrl"
    Write-Host "🔧 Running: $curlCmd" -ForegroundColor Gray
    
    $result = Invoke-Expression $curlCmd
    Write-Host "📨 Upload result:" -ForegroundColor Green
    Write-Host $result -ForegroundColor White
    
    if ($result -match "success|Success|uploaded|Uploaded") {
        Write-Host "✅ Upload appears successful!" -ForegroundColor Green
    } else {
        Write-Host "❌ Upload may have failed" -ForegroundColor Red
    }
} catch {
    Write-Host "❌ Upload error: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "🏁 Test complete!" -ForegroundColor Magenta