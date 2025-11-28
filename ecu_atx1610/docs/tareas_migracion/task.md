# Plan de Migración y Puesta en Marcha - Vehículo FOX

## Fase 1: Preparación del Sistema Base
- [ ] Verificar hardware disponible (ECU ATC-8110, EMUC-B2S3, PEX cards)
- [ ] Confirmar versión de Ubuntu 18.04 LTS en ECU
- [ ] Instalar dependencias del sistema (can-utils, cmake, gcc)
- [ ] Configurar acceso SSH a la ECU (193.147.165.236)
- [ ] Verificar conectividad de red y permisos

## Fase 2: Implementación de Comunicación CAN
- [ ] Implementar driver SocketCAN real en `socketcan_interface.cpp`
- [ ] Definir protocolo CAN completo en `can_protocol.hpp`
- [ ] Implementar handler BMS para 24 celdas
- [ ] Implementar comunicación con 4 motores (protocolo CCP)
- [ ] Implementar comunicación con supervisor (heartbeat)
- [ ] Configurar filtros CAN para IDs específicos
- [ ] Actualizar script `setup_can.sh` para can0 y can1

## Fase 3: Integración de Sensores y Actuadores
- [ ] Integrar tarjeta PEX-1202L (entradas analógicas)
- [ ] Integrar tarjeta PEX-DA16 (salidas analógicas/digitales)
- [ ] Configurar GPS ublox SM-76G
- [ ] Implementar lectura de sensores de suspensión
- [ ] Implementar lectura de sensores de tracción

## Fase 4: Control del Vehículo
- [ ] Implementar control de batería (BMS)
- [ ] Implementar control de tracción
- [ ] Implementar control de suspensión activa
- [ ] Implementar sistema de alarmas multinivel
- [ ] Implementar máquina de estados del vehículo

## Fase 5: Pruebas y Validación
- [ ] Pruebas con interfaces CAN virtuales (vcan)
- [ ] Pruebas de loopback CAN
- [ ] Pruebas con hardware real (BMS)
- [ ] Pruebas con motores
- [ ] Pruebas de integración completa
- [ ] Validación de sistema de alarmas

## Fase 6: Despliegue y Puesta en Marcha
- [ ] Compilar código en ECU
- [ ] Configurar interfaces CAN en hardware real
- [ ] Ejecutar sistema completo
- [ ] Monitoreo y ajuste de parámetros
- [ ] Documentar configuración final
