# Herramientas de Diagnóstico CAN

Este directorio contiene herramientas para diagnosticar y verificar la comunicación CAN en la ECU ATX-1610.

## Scripts Disponibles

### 1. `diagnose_can.sh` - Diagnóstico Completo del Sistema CAN

Script bash que verifica el estado completo del sistema CAN.

**Uso**:
```bash
cd /c/Users/ahech/Desktop/FOX/ecu_atx1610
sudo ./scripts/diagnose_can.sh
```

**Qué verifica**:
- ✓ Interfaces CAN disponibles (emuccan0, emuccan1)
- ✓ Estado de las interfaces (UP/DOWN)
- ✓ Estadísticas de tráfico (RX/TX packets, errores)
- ✓ Captura de tráfico en tiempo real
- ✓ Detección de IDs CAN esperados (BMS, motores, supervisor)
- ✓ Recomendaciones de solución de problemas

**Salida esperada**:
```
╔════════════════════════════════════════╗
║   DIAGNÓSTICO CAN - ECU ATX-1610      ║
╚════════════════════════════════════════╝

[1/5] Verificando interfaces CAN disponibles...
  ✓ emuccan0: UP
  ✓ emuccan1: UP

[2/5] Verificando estadísticas de tráfico CAN...
Interfaz: emuccan0
  RX packets: 1234 (errors: 0)
  TX packets: 567 (errors: 0)
  ✓ Recibiendo mensajes CAN
  ✓ Enviando mensajes CAN
...
```

### 2. `can_diagnostic.cpp` - Monitor CAN en Tiempo Real

Programa C++ que monitorea y clasifica el tráfico CAN en tiempo real.

**Compilación**:
```bash
cd /c/Users/ahech/Desktop/FOX/ecu_atx1610/build
cmake ..
make can_diagnostic
```

**Uso**:
```bash
# Usar interfaz por defecto (emuccan0)
./tools/can_diagnostic

# Especificar interfaz
./tools/can_diagnostic emuccan1
```

**Características**:
- Muestra cada mensaje CAN recibido en tiempo real
- Clasifica mensajes por tipo (BMS, Motor 1-4, Supervisor)
- Resumen cada 5 segundos con contadores
- Presionar Ctrl+C para detener

**Salida esperada**:
```
╔════════════════════════════════════════╗
║  DIAGNÓSTICO CAN - ECU ATX-1610       ║
║  Verificación de lectura de mensajes  ║
╚════════════════════════════════════════╝

Interfaz CAN: emuccan0
✓ Interfaz CAN iniciada correctamente

Monitoreando tráfico CAN...
================================================================================
 Dirección |      ID CAN |   DLC | Datos
--------------------------------------------------------------------------------
        RX | ID: 0x180 | DLC: 8 | Data: aa 55 01 02 03 04 05 06 
        RX | ID: 0x281 | DLC: 5 | Data: f4 00 48 65 6c 
        RX | ID: 0x100 | DLC: 3 | Data: aa 55 01 
================================================================================
RESUMEN (últimos 5 segundos):
  BMS (0x180):        45 mensajes
  Motor 1 CMD:        0 mensajes
  Motor 1 RESP:       12 mensajes
  Motor 2 CMD:        0 mensajes
  Motor 2 RESP:       12 mensajes
  ...
  Supervisor HB:      15 mensajes
  Supervisor CMD:     0 mensajes
  Desconocidos:       2 mensajes
================================================================================
```

## Solución de Problemas Comunes

### Problema: "No se encontraron interfaces CAN"

**Solución**:
```bash
sudo ./scripts/setup_can.sh --real
```

### Problema: "Interfaz no inicializada" o "Permission denied"

**Solución**:
```bash
# Ejecutar con sudo
sudo ./tools/can_diagnostic

# O dar permisos al usuario
sudo usermod -a -G dialout $USER
# Luego cerrar sesión y volver a entrar
```

### Problema: "No se detecta tráfico CAN"

**Verificar**:
1. Conexiones físicas CAN (cables, conectores)
2. Terminación CAN (resistencias 120Ω en ambos extremos)
3. Dispositivos encendidos (BMS, motores, supervisor)
4. Baudrate correcto:
   ```bash
   ip -details link show emuccan0 | grep bitrate
   ```

### Problema: "Solo se detecta BMS, no motores"

**Causa**: Los motores necesitan activación desde la ECU

**Solución**: Ejecutar la ECU principal que incluye el `CanInitializer`:
```bash
./build/ecu_atx1610
```

El inicializador enviará la secuencia de activación automáticamente.

## Comandos Útiles

```bash
# Ver todas las interfaces de red
ip link show

# Ver estadísticas detalladas de CAN
ip -s -s link show emuccan0

# Monitorear CAN con candump
candump emuccan0

# Enviar mensaje CAN de prueba
cansend emuccan0 100#AABBCCDD

# Grabar tráfico CAN a archivo
candump -l emuccan0

# Ver errores del kernel relacionados con CAN
dmesg | grep -i can

# Reiniciar interfaz CAN
sudo ip link set emuccan0 down
sudo ip link set emuccan0 up
```

## IDs CAN del Sistema

| Dispositivo | ID CAN (Hex) | ID CAN (Dec) | Descripción |
|-------------|--------------|--------------|-------------|
| BMS | 0x180 | 384 | Battery Management System |
| Motor 1 CMD | 0x201 | 513 | Comando a Motor 1 |
| Motor 1 RESP | 0x281 | 641 | Respuesta de Motor 1 |
| Motor 2 CMD | 0x202 | 514 | Comando a Motor 2 |
| Motor 2 RESP | 0x282 | 642 | Respuesta de Motor 2 |
| Motor 3 CMD | 0x203 | 515 | Comando a Motor 3 |
| Motor 3 RESP | 0x283 | 643 | Respuesta de Motor 3 |
| Motor 4 CMD | 0x204 | 516 | Comando a Motor 4 |
| Motor 4 RESP | 0x284 | 644 | Respuesta de Motor 4 |
| Supervisor HB | 0x100 | 256 | Heartbeat del Supervisor |
| Supervisor CMD | 0x101 | 257 | Comandos del Supervisor |

## Referencias

- [Guía de Inicialización CAN](../docs/can_initialization_guide.md)
- [Protocolo CAN](../comunicacion_can/can_protocol.hpp)
- [Gestor CAN](../comunicacion_can/can_manager.hpp)
