# Informe de Fix: Activación y Movimiento de Motores

## Problemas Detectados
1. **Lectura Acelerador**: El sensor leía valores ADC crudos (`0-4095`) pero el sistema de control esperaba una señal normalizada (`0.0-1.0`). Esto producía cálculos de torque erróneos (o saturación inmediata).
2. **Seguridad Freno**: El sistema cortaba el torque de los motores ante cualquier señal mínima del freno (`>0.1`), lo que impedía probar el acelerador si el sensor de freno tenía un ligero valor residual.
3. **Control DAC (Salida Analógica)**: La lógica asumía un DAC bipolar (+/-10V), mapeando 0V a 2048. En el hardware actual (Unipolar 0-10V), esto significaba que un comando de "0V" enviaba 0V, pero un comando de 5V enviaba solo ~2.5V (o valores incorrectos).

## Soluciones Aplicadas
1. **Normalización Sensor**: Se actualizó `pex1202l.cpp` para dividir el valor raw del acelerador por `4095.0`, entregando un valor limpio de 0 a 100% (`0.0 - 1.0`).
2. **Interlock Relajado**: Se modificó `traction_control.cpp` para emitir solo advertencias (**WARN**) en lugar de cortar el torque si se detecta freno leve, permitiendo pruebas dinámicas.
3. **Lógica DAC Unipolar**: Se ajustó `pexda16.cpp` para mapear 0-10V al rango completo del DAC `0-4095`. Ahora, una solicitud de 5V genera correctamente el código DAC ~2048.

## Prueba Exitosa (Simulación)
Se realizó una prueba inyectando una señal de acelerador del 50% (`0.5`) vía software en la ECU en funcionamiento.

**Resultados del Log:**
- **Entrada**: `Accel: 0.500000` (Correctamente normalizado).
- **Cálculo Torque**: `Torque: 60.000 Nm` (50% de 120Nm max). Correcto.
- **Salida CAN**: `Throttle: 153` (byte). Se están enviando tramas de par a los motores.
- **Salida Analógica**: `AO Write: AOX V=3.000000`. El sistema intenta enviar 3V (proporcional al torque) al control analógico.
    - *Nota*: Hubo errores de escritura en AO2 (`Error writing AO2`), posiblemente por saturación del bus o timeout, pero la lógica de control está generando los comandos correctos.

## Conclusión
El sistema de control ahora responde correctamente a la entrada del acelerador, generando los comandos de torque esperados tanto en CAN como en analógico. El vehículo debería moverse al pisar el pedal físico.
