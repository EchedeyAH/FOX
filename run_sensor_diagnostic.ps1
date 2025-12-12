# Script PowerShell para compilar y ejecutar el diagnóstico de sensores en la ECU

$ECU_HOST = "fox@193.147.165.236"
$ECU_PASSWORD = "FOX"

Write-Host "=== Diagnóstico de Sensores ECU FOX ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "📡 Conectando a ECU ($ECU_HOST)..." -ForegroundColor Yellow

# Primero, sincronizar el archivo sensor_diagnostic.cpp a la ECU
Write-Host "📤 Sincronizando archivos..." -ForegroundColor Yellow
scp -o StrictHostKeyChecking=no .\ecu_atc8110\tools\sensor_diagnostic.cpp ${ECU_HOST}:/home/fox/ecu_atc8110/tools/

Write-Host ""
Write-Host "🔨 Compilando en la ECU..." -ForegroundColor Yellow

# Ejecutar compilación y diagnóstico en la ECU
$commands = @"
cd /home/fox/ecu_atc8110/build
cmake .. -DCMAKE_BUILD_TYPE=Debug > /dev/null 2>&1
make sensor_diagnostic 2>&1
if [ `$? -eq 0 ]; then
    echo '✅ Compilación exitosa'
    echo ''
    echo '🔍 Ejecutando diagnóstico de sensores...'
    echo '==========================================='
    sudo ./tools/sensor_diagnostic
else
    echo '❌ Error en la compilación'
    make sensor_diagnostic
fi
"@

ssh -o StrictHostKeyChecking=no $ECU_HOST $commands

Write-Host ""
Write-Host "✅ Diagnóstico completado" -ForegroundColor Green
