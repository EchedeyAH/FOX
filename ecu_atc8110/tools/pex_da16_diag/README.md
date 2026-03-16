# PEX-DA16 Diagnostic Tool

Herramienta de diagnóstico para identificar el mapeo real de salidas analógicas (AO0–AO7) hacia los motores.

## Objetivo

Activar una sola salida analógica por vez con un patrón seguro (0.0V → 0.6V → 0.0V) y observar qué motor responde.

## Requisitos

- Driver PEX-DA16 (ixpio) cargado
- Nodo de dispositivo disponible: `/dev/ixpio1` (fallback a `/dev/ixpio0`)
- Ejecutar como `root` o con permisos para `open`/`ioctl`

## Compilación

```bash
cd ecu_atc8110/tools/pex_da16_diag
make
```

## Uso (modo automático)

```bash
sudo ./pex_da16_diag
```

Recorre AO0..AO7, con un paso de 4s por estado.

## Uso (modo manual)

```bash
sudo ./pex_da16_diag --channel=3 --voltage=0.6
```

Mantiene AO3 en 0.6V hasta que pulses `Ctrl+C`.

## Opciones

- `--device=/dev/ixpio1`
- `--step=4` (segundos por estado)
- `--can-if=can0` (monitor CAN opcional)

## Seguridad

- El rango está limitado a **0.0–1.0V**.
- Solo un canal activo a la vez.
- Todos los demás canales permanecen en 0V.

## Procedimiento sugerido en vehículo

1. Colocar el vehículo en modo seguro (sin tracción).
2. Ejecutar la herramienta en modo automático.
3. Para cada canal, observar qué motor reacciona.
4. Registrar la correspondencia AO ↔ motor.

## Salida esperada

```
Testing AO0
Output = 0.6V. Observe el motor...
Testing AO1
Output = 0.6V. Observe el motor...
...
```
