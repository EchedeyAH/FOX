$dest = "c:\Users\ahech\Desktop\FOX\ecu_atc8110\docs\PLAN_MAESTRO_UNIFICADO.md"
$files = @(
    @{Path="c:\Users\ahech\Desktop\FOX\ecu_atc8110\docs\tareas_migracion\task.md"; Title="1. RESUMEN DE TAREAS DE MIGRACIÓN"},
    @{Path="c:\Users\ahech\Desktop\FOX\ecu_atc8110\docs\tareas_migracion\plan_migracion_detallado.md"; Title="2. PLAN DE MIGRACIÓN DETALLADO"},
    @{Path="c:\Users\ahech\Desktop\FOX\ecu_atc8110\docs\implementation_plan.md"; Title="3. PLAN DE IMPLEMENTACIÓN: COMUNICACIÓN CAN"},
    @{Path="c:\Users\ahech\Desktop\FOX\ecu_atc8110\docs\task.md"; Title="4. ESTADO DE TAREAS: COMUNICACIÓN CAN"}
)

"# Plan Maestro Unificado del Proyecto FOX`n" | Set-Content $dest
"**Fecha:** $(Get-Date -Format 'yyyy-MM-dd')`n" | Add-Content $dest
"Este documento unifica todos los planes de planificación y tareas.`n`n---`n" | Add-Content $dest

foreach ($file in $files) {
    "`n# $($file.Title)`n" | Add-Content $dest
    "*(Fuente: $($file.Path))*`n" | Add-Content $dest
    Get-Content $file.Path | Add-Content $dest
    "`n---`n" | Add-Content $dest
}
Write-Host "File created at $dest"
