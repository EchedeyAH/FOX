# Informe de Implementación: Activación BRAKE_SW en ECU FOX

## Resumen
Se ha implementado exitosamente la lectura del pedal de freno (BRAKE_SW) a través de la tarjeta de I/O Digital **ICP DAS PCI-1202** (`/dev/ixpio1`) y su integración como condición de seguridad (interlock) para el arranque del sistema.

## Cambios Realizados

### 1. Driver de I/O (`pex_device.hpp`)
- Se añadió gestión independiente para el file descriptor de `/dev/ixpio1` mediante `GetDioFd()`.
- Se separó la lógica de analógico (`/dev/ixpci1`) y digital para evitar conflictos.

### 2. Lectura de Señal (`pex1202l.cpp`)
- Implementación del comando IOCTL `0x40046901` para leer entradas digitales.
- Mapeo del **Bit 0** de la entrada digital al sensor virtual `brake_switch`.
- **Modo Debug/Remoto**: Se añadió capacidad de "pisar el freno" por software escribiendo en `/tmp/force_brake` para pruebas sin acceso físico.

### 3. Máquina de Estados (`state_machine.hpp`)
- **Lógica de Arranque**: El sistema ahora inicia en estado `Inicializando` y **espera** activamente que el freno sea presionado (`brake >= 0.9` ó `brake_switch == 1`).
- **Transición Segura**: Solo tras detectar el freno se ejecuta la secuencia de activación de motores (ENABLE + CAN) y se pasa a `Operando`.
- **Integración de Sensores**: El valor del freno se calcula como el *máximo* entre la lectura analógica (si existe) y el switch digital, asegurando redundancia.

## Verificación
1. **Despliegue**: Código compilado y subido a la ECU (`193.147.165.236`).
2. **Prueba**:
    - El sistema inició esperando el freno: logging `Esperando pedal de freno...`.
    - Se simuló la activación mediante `echo 1 > /tmp/force_brake`.
    - El sistema detectó la señal, activó motores y comenzó la transmisión CAN (`TX id=0x51X`).

## Instrucciones de Uso

### Arranque Normal (Con Pedal)
1. Encender el vehículo/ECU.
2. Ejecutar el software ECU.
3. **Pisar el pedal de freno** a fondo.
4. El sistema detectará la señal, activará el relé ENABLE y comenzará la operación.

### Arranque de Prueba (Sin Pedal)
Si se necesita arrancar remotamente sin operario:
```bash
# En una terminal SSH a la ECU:
echo 1 > /tmp/force_brake
```
Esto simulará permanentemente el freno pisado para la lógica de arranque.

## Archivos Modificados
- `ecu_atc8110/adquisicion_datos/pex_device.hpp`
- `ecu_atc8110/adquisicion_datos/pex1202l.cpp`
- `ecu_atc8110/logica_sistema/state_machine.hpp`
- `manual_deploy.ps1` (Script de despliegue Windows)
