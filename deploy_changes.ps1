# Script PowerShell para transferir archivos modificados al ECU

$ECU_IP = "193.147.165.236"
$ECU_USER = "fox"
$LOCAL_DIR = "ecu_atc8110"
$REMOTE_DIR = "/home/fox/ecu_atc8110"

Write-Host "=== Transferencia de archivos al ECU ===" -ForegroundColor Cyan
Write-Host ""

# Transferir CMakeLists.txt
Write-Host "1. Transfiriendo CMakeLists.txt..." -ForegroundColor Yellow
scp "$LOCAL_DIR\CMakeLists.txt" "${ECU_USER}@${ECU_IP}:${REMOTE_DIR}/"

# Transferir traction_control_wrapper.cpp
Write-Host "2. Transfiriendo traction_control_wrapper.cpp..." -ForegroundColor Yellow
scp "$LOCAL_DIR\control_vehiculo\traction_control_wrapper.cpp" "${ECU_USER}@${ECU_IP}:${REMOTE_DIR}/control_vehiculo/"

Write-Host ""
Write-Host "=== Transferencia completada ===" -ForegroundColor Green
Write-Host ""
Write-Host "Ahora compila en el ECU con:" -ForegroundColor Cyan
Write-Host "  ssh ${ECU_USER}@${ECU_IP}"
Write-Host "  cd ${REMOTE_DIR}"
Write-Host "  rm -rf build && mkdir build && cd build"
Write-Host "  cmake .. && make -j4"
