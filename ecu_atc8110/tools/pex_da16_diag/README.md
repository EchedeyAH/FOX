# PEX-DA16 Diagnostic Tool

Herramienta standalone para identificar el mapeo real de salidas analógicas (AO0–AO7) hacia motores/funciones.

## Seguridad

- Rango de prueba: **0.0V – 1.0V**
- Solo un canal activo a la vez
- El resto en 0V
- Ejecutar en vehículo real solo en modo seguro (sin tracción)

## Compilación

```bash
cd ecu_atc8110/tools/pex_da16_diag
make
```

## Uso (modo scan)

```bash
sudo ./pex_da16_diag
```

Opcional CAN:

```bash
sudo ./pex_da16_diag --can=can0
```

## Uso (modo manual)

```bash
sudo ./pex_da16_diag --channel=3 --voltage=0.6
```

## Procedimiento recomendado en vehículo

1. Asegurar modo seguro.
2. Ejecutar el scan.
3. Observar qué motor responde a cada AO.
4. Introducir el mapeo al final.
5. Se genera `ecu_output_map.json`.

## Archivo de salida

Ejemplo:

```json
{
  "AO0": "motor_front_left_accel",
  "AO1": "motor_front_right_accel",
  "AO2": "motor_rear_left_accel",
  "AO3": "motor_rear_right_accel"
}
```
