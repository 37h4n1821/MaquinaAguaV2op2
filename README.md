# Máquina de Agua V2 (Opción 2)

![Imagen de la Máquina de Agua (Vista Frontal)](URL_IMAGEN_FRONTAL_AQUI)
*Espacio para la imagen principal de la máquina*

![Diagrama de Conexiones](URL_IMAGEN_CONEXIONES_AQUI)
*Espacio para el diagrama de conexiones o vista interna*

## Descripción del Proyecto
Este proyecto es el software de control para la **Máquina de Agua V2**, un sistema de autoatención basado en un microcontrolador ESP32. Integra una interfaz de usuario mediante botones, indicadores LED, pantalla LCD y conectividad con un terminal de pagos POS (Transbank IM30). Todo esto permite un flujo de venta de agua de forma autónoma.

---
## Hardware Principal
**Microcontrolador:** ESP32 (dual-core Tensilica LX6 de 32 bits)
- **Frecuencia de CPU:** Hasta 240 MHz
- **Conectividad:**
  - WiFi 2.4 GHz IEEE 802.11 b/g/n (Modos AP, STA y AP+STA)
  - Bluetooth Classic (BR/EDR) y Bluetooth Low Energy (BLE)
- **Memoria:** SRAM: 520 KB | Flash: 16 MB
- **Chip USB a Serial:** CP2102 (Silicon Labs) - Driver USB: CP210x USB to UART Bridge
- **Alimentación:** 5 V vía puerto micro USB (Voltaje de operación del chip: 2.2 V – 3.6 V)
- **Sensores y Periféricos:** Sensor Hall, Sensores táctiles capacitivos, UART, SPI, I2C, I2S, ADC de 12 bits, DAC (2 canales), PWM, timers y GPIO
- **Otros:** Botones integrados BOOT y RESET, 38 pines.

## Módulos Adicionales
1. **Módulo RS232 (TTL a RS232)**:
   - Conectado al ESP32 para proveer comunicación serial RS232.
   - Pines típicos de conexión sugeridos: RX2 (Pin 16) y TX2 (Pin 17) del ESP32.

2. **Terminal de Pago POS:** Transbank IM30 (PAX IM30 u homólogo de autoatención)
   - Conectado a través del módulo RS232.
   - **Protocolo de Comunicación:** Protocolo Transbank POS / Integración de Autoatención (STX, DATA, ETX, LRC).
   - **Baudrate:** 115200 bps, 8N1.

3. **Pantalla LCD 20x4 (I2C):**
   - Conectada al bus I2C (pines SDA/SCL por defecto).
   - Dirección típica: 0x27.

4. **Módulo Expansor I2C MCP23017:**
   - Conectado al bus I2C para expansión de entradas/salidas.
   - Dirección típica: 0x20.

5. **Aro LED WS2812:**
   - Conectado al pin G4 (GPIO 4) del ESP32.
   - Usado para animación de progreso de inicio y estados de error (Ej. rojo fijo si el boot falla).

## Estado del Software
- Plataforma: Arduino IDE.
- Archivo principal: `MaquinaAguaV2op2.ino`.
- Librería implementada localmente: `TransbankIM30.h` y `TransbankIM30.cpp` para manejar las peticiones al POS IM30 (Polling, Inicialización, Venta).

## Funcionalidad de la Librería IM30
- Comunicación asíncrona/síncrona por UART2 (Serial2).
- Verificación de comunicación (`Polling 0100`).
- Comando de Inicialización (`0070`).
- Comando de Venta (`0200`).
- Recepción y parsing de la trama de respuesta.
