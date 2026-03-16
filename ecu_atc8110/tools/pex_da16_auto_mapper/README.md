# PEX-DA16 Auto Mapper

Herramienta para identificar automáticamente qué salida analógica (AO0–AO7) corresponde a cada motor observando cambios en CAN.

## Seguridad

- Señal de prueba limitada a **0.0V – 1.0V**
- Solo un canal activo a la vez
- Todos los demás canales en 0V
- Uso en vehículo real: mantener modo seguro (sin tracción)

## Compilación

```bash
cd ecu_atc8110/tools/pex_da16_auto_mapper
make
```

## Uso recomendado

```bash
sudo ./pex_scan --device /dev/ixpio1 --can can0
```

Opciones:
- `--step=4` (segundos por estado)
- `--channel=2 --voltage=0.6` (modo manual)
- `--monitor-can` (solo monitor CAN)

## Salida esperada

Durante cada AO:

```
Testing AO2
Output = 0.6V. Monitoring CAN...
CAN changes detected: 1
  ID 0x182 (motor2_status)
```

Resumen final:

```
AO0 -> motor3_accelerator
AO1 -> motor1_accelerator
...
```

Se guarda un archivo:

```
ecu_io_map.json
```

## Procedimiento en vehículo

1. Detener vehículo y asegurar modo seguro.
2. Ejecutar el tool.
3. Observar qué motor responde a cada AO.
4. Confirmar con el mapeo generado.

## Notas

El mapeo automático depende de cambios visibles en CAN.
Si no se detecta cambio, el mapeo quedará como `unknown`.
