# Lógica de la Máquina de Estados: Dispensador de Agua V2

Este documento describe el flujo completo de venta y uso de periféricos (Pantalla LCD, Aro LED NeoPixel, Botones MCP23017 y Lector Transbank). Está diseñado como referencia principal tanto para el desarrollador humano como para los agentes de IA que colaboren en el proyecto.

## Máquina de Estados Principales

El flujo del dispensador se divide en los siguientes estados:

### 1. Estado: IDLE (Espera de Cliente)
- **Condición**: La máquina está encendida, el POS está listo y no hay ventas en curso.
- **Botones (MCP23017)**: 
  - Botones de **Venta** (`A0`, `A1`, `A2`, `A3`): **Habilitados**. Se utiliza lectura multiplexada (alternando entre `OUTPUT HIGH` y `INPUT_PULLUP` rápidamente) para que se vean iluminados y a la vez detecten pulsaciones.
  - Botones de **Acción** (`iniciar`, `stop`): **Deshabilitados** (Configurados como `OUTPUT LOW` para que no se iluminen y no se leen).
- **Pantalla LCD**: Mensaje de bienvenida (Ej. "Sistema Listo - Seleccione una opcion").
- **Aro LED (NeoPixel)**: Efecto "estrellas" en tonos azules. Los LEDs se encienden y apagan aleatoriamente con bajo brillo simulando un cielo estrellado.

### 2. Estado: SELECCION_VENTA (Botón de Venta Presionado)
- **Condición**: El usuario presiona uno de los botones de venta.
- **Acciones**:
  - Se lee la configuración de ese botón desde `butons.json` (`monto`, `tiempo_ms`, `texto`).
  - **Botones**: Se deshabilitan (apagan) los demás botones de venta. El botón presionado queda con un efecto de "respiración" (subiendo y bajando el brillo). *Nota: En el MCP23017 esto se emulará modulando el tiempo de encendido/apagado vía I2C o con parpadeo suave*.
  - **Pantalla LCD**: Muestra el `texto` de la selección y el `monto` a cobrar.
  - **Transbank**: Se envía el comando de venta (`startSale`) al IM30 con el `monto` correspondiente.
- **Transición**: Pasa inmediatamente al estado `ESPERA_PAGO`.

### 3. Estado: ESPERA_PAGO
- **Condición**: Esperando que el usuario acerque la tarjeta y el banco apruebe.
- **Pantalla LCD**: "Siga instrucciones en el terminal..."
- **Aro LED**: Efecto de carga o giro en color amarillo, indicando "procesando".
- **Transición**: 
  - Si Transbank aprueba $\rightarrow$ Pasa a `PAGO_APROBADO`.
  - Si Transbank rechaza / cancela $\rightarrow$ Muestra error, suena alerta (si hay) y vuelve a `IDLE`.

### 4. Estado: PAGO_APROBADO (Espera de Inicio)
- **Condición**: El pago fue exitoso y la máquina está lista para dar agua.
- **Botones**: 
  - El botón de venta se apaga.
  - Se **habilita** el botón `iniciar` (Acción 1). Se ilumina (multiplexado) para invitar al usuario a presionarlo.
- **Pantalla LCD**: "Venta Aprobada" en la primera línea. "Presione INICIAR" en la segunda.
- **Aro LED**: Brilla en verde fijo o pulsa en verde.
- **Transición**: Queda a la espera de que se presione el botón "iniciar". Al presionarse, pasa a `DISPENSANDO`.

### 5. Estado: DISPENSANDO
- **Condición**: Usuario presiona el botón "iniciar".
- **Botones**: 
  - Se deshabilita (apaga) el botón `iniciar`.
  - Se **habilita** el botón `stop` (Acción 2) iluminándolo.
- **Hardware (Próximamente)**: Se activan los relés de las válvulas o bombas.
- **Pantalla LCD**: "Entregando agua..." + Barra de progreso de caracteres `[██████    ]`.
- **Aro LED**: Funciona como una barra de progreso circular en color azul. Se van encendiendo los LEDs en proporción al tiempo transcurrido respecto al `tiempo_ms` del botón seleccionado.
- **Transición**: 
  - Si transcurre el `tiempo_ms` completo $\rightarrow$ Pasa a `FINALIZADO`.
  - Si el usuario presiona el botón `stop` $\rightarrow$ Pasa a `FINALIZADO` prematuramente.

### 6. Estado: FINALIZADO
- **Condición**: Tiempo cumplido o parada manual (stop).
- **Acciones**:
  - **Hardware**: Se desactivan los relés de las bombas.
  - **Botones**: Se deshabilitan todos (incluido `stop`).
  - **Pantalla LCD**: "Gracias por su compra".
  - **Aro LED**: Efecto de flash verde o arcoíris corto de celebración.
- **Transición**: Tras 3 segundos de pausa, retorna automáticamente al estado `IDLE`.

---

## Estructura de Código (Arquitectura Propuesta)

Para mantener el proyecto ordenado ("clean code"), el código principal `MaquinaAguaV2op2.ino` se simplificará y delegará responsabilidades en librerías/clases locales separadas:

1. **`ButtonManager.h` / `.cpp`**:
   - Maneja el expansor MCP23017.
   - Expone métodos como `enableVentaButtons()`, `enableStartButton()`, `readButtons()`.
   - Contiene la lógica del "multiplexado" o "software PWM" para mantener los botones iluminados y detectar pulsaciones casi al mismo tiempo.
2. **`LEDManager.h` / `.cpp`**:
   - Maneja el aro NeoPixel.
   - Contiene funciones como `showStarsEffect()`, `showProgressBar(int current, int total)`, `showSuccess()`.
3. **`DisplayManager.h` / `.cpp`**:
   - Maneja la pantalla LCD I2C.
   - Contiene menús empaquetados: `showIdle()`, `showDispensing(int progress)`, `showError()`.
4. **`AppLogic.h` / `.cpp`**:
   - Coordina la máquina de estados (IDLE, VENTA, etc) usando los Managers mencionados y la librería `TransbankIM30.h`.
