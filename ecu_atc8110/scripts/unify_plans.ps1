$dest = "c:\Users\ahech\Desktop\FOX\ecu_atc8110\docs\PLAN_MAESTRO_UNIFICADO.md"

$f1 = "c:\Users\ahech\Desktop\FOX\ecu_atc8110\docs\tareas_migracion\task.md"
$t1 = "1. RESUMEN DE TAREAS DE MIGRACIÓN"

$f2 = "c:\Users\ahech\Desktop\FOX\ecu_atc8110\docs\tareas_migracion\plan_migracion_detallado.md"
$t2 = "2. PLAN DE MIGRACIÓN DETALLADO"

$f3 = "c:\Users\ahech\Desktop\FOX\ecu_atc8110\docs\implementation_plan.md"
$t3 = "3. PLAN DE IMPLEMENTACIÓN: COMUNICACIÓN CAN"

$f4 = "c:\Users\ahech\Desktop\FOX\ecu_atc8110\docs\task.md"
$t4 = "4. ESTADO DE TAREAS: COMUNICACIÓN CAN"

"# Plan Maestro Unificado del Proyecto FOX`r`n" | Set-Content $dest -Encoding UTF8
"**Fecha:** $(Get-Date -Format 'yyyy-MM-dd')`r`n" | Add-Content $dest
"Este documento unifica todos los planes de planificación y tareas.`r`n`r`n---`r`n" | Add-Content $dest

function Add-Section($path, $title) {
    "`r`n# $title`r`n" | Add-Content $dest
    "*(Fuente: $path)*`r`n" | Add-Content $dest
    Get-Content $path | Add-Content $dest
    "`r`n---`r`n" | Add-Content $dest
}

Add-Section $f1 $t1
Add-Section $f2 $t2
Add-Section $f3 $t3
Add-Section $f4 $t4

Write-Host "File created at $dest"
