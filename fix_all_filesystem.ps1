# Script para reemplazar TODAS las referencias a std::filesystem en el proyecto
# Solución definitiva para compatibilidad GCC 7.5

Write-Host "=== Actualizando TODOS los archivos del proyecto ===" -ForegroundColor Cyan
Write-Host ""

# Buscar TODOS los archivos .cpp y .hpp en common/
$files = Get-ChildItem -Path "ecu_atc8110\common" -Include *.cpp, *.hpp -Recurse

$totalFiles = 0
$updatedFiles = 0

foreach ($file in $files) {
    $totalFiles++
    $content = Get-Content $file.FullName -Raw
    
    # Verificar si contiene std::filesystem
    if ($content -match 'std::filesystem') {
        Write-Host "Actualizando: $($file.Name)" -ForegroundColor Yellow
        
        # Reemplazar std::filesystem:: con fs::
        $newContent = $content -replace 'std::filesystem::', 'fs::'
        
        # Guardar
        Set-Content -Path $file.FullName -Value $newContent -NoNewline
        
        $updatedFiles++
        Write-Host "  ✓ Actualizado" -ForegroundColor Green
    }
}

Write-Host ""
Write-Host "=== Resumen ===" -ForegroundColor Cyan
Write-Host "Archivos analizados: $totalFiles"
Write-Host "Archivos actualizados: $updatedFiles" -ForegroundColor Green
Write-Host ""
Write-Host "Ahora transfiriendo TODO el directorio common/ al ECU..." -ForegroundColor Yellow

# Transferir TODO el directorio common de una vez
scp -r ecu_atc8110\common\* fox@193.147.165.236:/home/fox/ecu_atc8110/common/

Write-Host ""
Write-Host "=== Completado ===" -ForegroundColor Green
Write-Host "Ahora compila en el ECU con:" -ForegroundColor Cyan
Write-Host "  cd /home/fox/ecu_atc8110/build"
Write-Host "  cmake .. && make -j4"
