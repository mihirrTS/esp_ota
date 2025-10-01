# Simple upload test
Write-Host "Testing firmware upload..." -ForegroundColor Green

# Test server connectivity
$serverUrl = "http://127.0.0.1:5000"
try {
    Write-Host "Testing server connectivity..." -ForegroundColor Yellow
    $response = Invoke-WebRequest -Uri $serverUrl -TimeoutSec 5 -ErrorAction Stop
    Write-Host "Server responding: $($response.StatusCode)" -ForegroundColor Green
} catch {
    Write-Host "Server not accessible: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Make sure the server is running at $serverUrl" -ForegroundColor Cyan
    exit 1
}

# Find firmware file
$firmwareFile = "custom_driver\ESP32_DevKit_V1_PersonalCMS\ESP32_DevKit_V1_PersonalCMS\bin\ESP32_DevKit_V1_PersonalCMS_v2.0.bin"

if (-not (Test-Path $firmwareFile)) {
    Write-Host "Firmware file not found: $firmwareFile" -ForegroundColor Red
    exit 1
}

Write-Host "Using firmware file: $firmwareFile" -ForegroundColor Cyan
$fileSize = (Get-Item $firmwareFile).Length
Write-Host "File size: $fileSize bytes" -ForegroundColor Cyan

# Test upload with curl
$uploadUrl = "$serverUrl/ota-upload"
Write-Host "Uploading to $uploadUrl..." -ForegroundColor Yellow

try {
    $curlCmd = "curl -X POST -F firmware=@`"$firmwareFile`" -F version=2.0 -F device_type=ESP32_PersonalCMS -F description=`"Test upload`" $uploadUrl"
    Write-Host "Running: $curlCmd" -ForegroundColor Gray
    
    $result = cmd /c $curlCmd
    Write-Host "Upload result:" -ForegroundColor Green
    Write-Host $result -ForegroundColor White
    
} catch {
    Write-Host "Upload error: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "Test complete!" -ForegroundColor Magenta