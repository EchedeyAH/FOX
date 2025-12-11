# Script PowerShell para transferir archivos modificados al ECU

$ECU_IP = "193.147.165.236"
$ECU_USER = "fox"
$LOCAL_DIR = "ecu_atc8110"
$REMOTE_DIR = "/home/fox/ecu_atc8110"

Write-Host "=== Transferencia de archivos al ECU ===" -ForegroundColor Cyan
Write-Host ""

# Transferir CMakeLists.txt
Write-Host "Syncing CMakeLists.txt..." -ForegroundColor Yellow
scp "$LOCAL_DIR\CMakeLists.txt" "${ECU_USER}@${ECU_IP}:${REMOTE_DIR}/"

# Transferir carpetas completas (Recursive)
Write-Host "Syncing adquisicion_datos..." -ForegroundColor Yellow
scp -r "$LOCAL_DIR\adquisicion_datos" "${ECU_USER}@${ECU_IP}:${REMOTE_DIR}/"

Write-Host "Syncing logica_sistema..." -ForegroundColor Yellow
scp -r "$LOCAL_DIR\logica_sistema" "${ECU_USER}@${ECU_IP}:${REMOTE_DIR}/"

Write-Host "Syncing control_vehiculo..." -ForegroundColor Yellow
scp -r "$LOCAL_DIR\control_vehiculo" "${ECU_USER}@${ECU_IP}:${REMOTE_DIR}/"

Write-Host ""
Write-Host "=== Transferencia completada ===" -ForegroundColor Green
