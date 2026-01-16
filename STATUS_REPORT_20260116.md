# Informe de Estado FOX ECU - 16/01/2026

## 🟢 Tareas Completadas HOY

### 1. Integración PEX-1202L (Sensores)
- **Configuración HW**: Se ha implementado el `ioctl` para configurar el rango de entrada a **+/- 5V Bipolar** (Gain 1). Esto evita lecturas erróneas por usar el default de +/- 10V.
- **Corrección de Escalado**: Se ha corregido la fórmula de lectura. Ahora el valor `0.0` (0%) corresponde a 0V físicos (antes marcaba 50%).
- **Calibración**: El sensor del volante ya usa los valores del YAML (`scale: 0.9280`, `offset: -3.8254`).

### 2. Infraestructura PexDevice
- **Control de Spam**: Se ha añadido un sistema de reintentos (throttle) en `PexDevice.hpp`. Si una tarjeta está ocupada o no existe, solo intenta abrirla cada 5 segundos para no saturar los logs.

### 3. Verificación CAN
- **Mapeo Confirmado**:
    - `emuccan0`: Motores (1 Mbps).
    - `emuccan1`: BMS (Batería) (500 Kbps habitual).
- **Diagnóstico**: Las interfaces están UP, pero falta verificar tráfico real una vez se resuelva el conflicto de hardware.

---

## 🟡 Pendiente para la Próxima Sesión

### 1. Resolver Conflicto "EBUSY"
El software indica que `/dev/ixpci1` está ocupado. 
**Acción**: Ejecutar limpieza de procesos antes de arrancar.
```bash
sudo pkill -9 ecu_atc8110
sudo pkill -9 sensor_diagnostic
```

### 2. Verificación de Sensores Reales
Una vez resuelto el EBUSY, el sistema dejará de entrar en "MODO SIMULACIÓN" y leerá los canales reales.
**Objetivo**: Confirmar que al pisar el freno, el log cambia y permite el arranque (`FSM`).

### 3. Prueba de Movimiento
1. Arrancar ECU: `sudo ./ecu_atc8110`.
2. Pisar Freno (Interlock de seguridad).
3. Verificar que pasa al estado operacional.
4. Probar aceleración suave (con ruedas en el aire).

---

## 🚀 Comandos Rápidos para Reanudar
```bash
# Limpiar procesos antiguos
sudo pkill -9 ecu_atc8110

# Compilar y ejecutar
cd ~/ecu_atc8110/build
cmake .. && make -j4
sudo ./ecu_atc8110
```

*Nota: Si el BMS no responde, recuerda verificar la velocidad del puerto con `ip -details link show emuccan1`.*
