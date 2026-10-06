# Lista de Componentes - Máquina de Agua V2

Basado en el código fuente y las configuraciones de hardware (`MaquinaAguaV2op2.ino`, `HardwareManager.cpp`, `proyectoinfo.md`), la máquina requiere los siguientes componentes de hardware:

## 1. Unidad de Procesamiento Principal
* **Microcontrolador:** Placa de desarrollo **ESP32**. 
  * Procesa la lógica, provee conectividad WiFi, levanta el servidor web local y maneja el sistema de archivos LittleFS.

## 2. Interfaz de Usuario (UI) y Medios de Pago
* **Terminal de Pago POS:** **Transbank IM30** (o terminal homólogo PAX IM30).
  * *Conexión:* Módulo TTL a RS232 conectado a los pines RX=16 y TX=17 del ESP32.
* **Pantalla LCD:** **LCD 20x4** con módulo I2C.
  * *Conexión:* Bus I2C del ESP32 (dirección 0x27).
* **Aro LED:** **Anillo NeoPixel WS2812** (24 LEDs).
  * *Conexión:* Directo al GPIO 4 del ESP32.
* **Botonera de Control:** Hasta 10 Botones iluminados (capacitivos o mecánicos con LED integrado).
  * *Conexión:* Multiplexados a través del expansor MCP23017 para manejar lectura y retroiluminación simultánea.

## 3. Expansión de E/S
* **Expansor de Puertos I2C:** Módulo **MCP23017** (16 canales).
  * *Conexión:* Bus I2C del ESP32. Fundamental porque el ESP32 no cuenta con suficientes pines para todos los sensores, relés y botones simultáneamente.

## 4. Sensores de Nivel y Flujo (Conectados al MCP23017)
* **Sensor de Nivel Inferior (Flotador):** Indica si hay nivel mínimo de agua para operar.
* **Sensor de Nivel Superior (Flotador):** Detecta cuando el estanque está lleno (corta llenado automático).
* **Sensor de Agua Entrada:** Detecta presión/presencia de agua proveniente de la red.
* **Sensor de Agua Ósmosis:** Detecta flujo en el proceso de filtrado de ósmosis.
* **Sensor de Agua Dispensador:** Detecta si efectivamente está pasando agua por la salida del bidón.

## 5. Actuadores y Relés (Conectados al MCP23017)
Se requiere un **módulo de relés** (típicamente de 4 u 8 canales, de activación en nivel ALTO o BAJO según `hardware.json`) para controlar la potencia de:
* **Bomba Dispensadora:** Bomba de agua que impulsa el líquido hacia el bidón del cliente.
* **Válvula de Llenado (Solenoide):** Electroválvula que permite el ingreso de agua nueva al estanque.
* **Motor de Aire (Bomba Oxigenadora):** Motor o bomba de acuario utilizada para airear/oxigenar el agua cada cierto intervalo.
* **Luz de Cabina (Bidón):** Iluminación (probablemente LED 12V/24V) que enciende al interior del compartimiento donde el usuario coloca su envase durante el llenado.

## 6. Componentes Auxiliares
* **Módulo Convertidor TTL a RS232** (Chip MAX3232 o similar) para la comunicación segura entre el ESP32 (3.3V) y el puerto serie del Transbank.
* **Fuentes de Alimentación:** 
  * Fuente de 5V para el ESP32, Pantalla LCD, MCP23017, y Aro NeoPixel.
  * Fuente secundaria 12V para la Luz de Cabina.
