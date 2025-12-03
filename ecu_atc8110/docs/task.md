# Implementación de Comunicación CAN

## Objetivo
Implementar la comunicación CAN completa para la ECU ATX-1610, basándose en el código legacy y adaptándolo a SocketCAN en Linux.

## Tareas

### Análisis y Planificación
- [x] Revisar código CAN existente (simulación)
- [x] Analizar código legacy de ECU_FOX_rc30-operativo
- [x] Definir protocolo CAN para el nuevo sistema
- [x] Crear plan de implementación

### Implementación SocketCAN
- [x] Implementar driver SocketCAN real (reemplazar simulación)
- [x] Añadir soporte para múltiples canales CAN
- [x] Implementar filtros CAN
- [x] Añadir manejo de errores robusto

### Protocolo de Mensajes
- [x] Definir IDs CAN para cada componente
- [x] Implementar mensajes de heartbeat
- [x] Implementar mensajes de batería (BMS)
- [/] Implementar mensajes de motores
- [x] Implementar mensajes de supervisor

### Integración
- [x] Integrar con el sistema principal
- [x] Añadir logging de mensajes CAN
- [x] Crear script de configuración de interfaces CAN

### Verificación
- [x] Crear documentación completa
- [ ] Probar comunicación CAN en loopback
- [ ] Verificar con hardware real (requiere ECU)
- [ ] Documentar protocolo CAN
