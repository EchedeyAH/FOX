# Script para actualizar std::filesystem a std::experimental::filesystem
# Para compatibilidad con GCC 7.5 (Ubuntu 18.04)

$files = @(
    "common\bms_data_logger.hpp",
    "common\can_traffic_logger.hpp",
    "common\error_logger.hpp",
    "common\imu_data_logger.hpp",
    "common\motor_data_logger.hpp",
    "common\sensor_data_logger.hpp",
    "common\system_data_logger.hpp",
    "common\data_logger_manager.hpp"
)

foreach ($file in $files) {
    $fullPath = Join-Path "ecu_atc8110" $file
    
    if (Test-Path $fullPath) {
        Write-Host "Actualizando $file..." -ForegroundColor Yellow
        
        $content = Get-Content $fullPath -Raw
        
        # Reemplazar std::filesystem::path con fs::path
        $content = $content -replace 'std::filesystem::path', 'fs::path'
        
        # Guardar el archivo
        Set-Content -Path $fullPath -Value $content -NoNewline
        
        Write-Host "  ✓ Actualizado" -ForegroundColor Green
    }
    else {
        Write-Host "  ✗ No encontrado: $fullPath" -ForegroundColor Red
    }
}

# Actualizar archivos .cpp también
$cppFiles = @(
    "common\bms_data_logger.cpp",
    "common\can_traffic_logger.cpp",
    "common\error_logger.cpp",
    "common\imu_data_logger.cpp",
    "common\motor_data_logger.cpp",
    "common\sensor_data_logger.cpp",
    "common\system_data_logger.cpp",
    "common\data_logger_manager.cpp"
)

foreach ($file in $cppFiles) {
    $fullPath = Join-Path "ecu_atc8110" $file
    
    if (Test-Path $fullPath) {
        Write-Host "Actualizando $file..." -ForegroundColor Yellow
        
        $content = Get-Content $fullPath -Raw
        
        # Reemplazar std::filesystem::path con fs::path
        $content = $content -replace 'std::filesystem::path', 'fs::path'
        
        # Guardar el archivo
        Set-Content -Path $fullPath -Value $content -NoNewline
        
        Write-Host "  ✓ Actualizado" -ForegroundColor Green
    }
    else {
        Write-Host "  ⚠ No encontrado: $fullPath (puede no existir)" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "=== Actualización completada ===" -ForegroundColor Cyan
Write-Host "Ahora ejecuta: .\deploy_changes.ps1" -ForegroundColor Cyan
