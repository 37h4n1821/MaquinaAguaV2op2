# Manual de Uso Avanzado - Dispensador de Agua Automático V2

Este documento está diseñado para administradores, mantenedores y técnicos de la Máquina de Agua V2. Detalla el flujo de uso, la lógica detrás de los componentes y ofrece un espacio para documentar el sistema visualmente.

---

## 1. Proceso de Arranque (Boot)

Al conectar la máquina a la corriente eléctrica (220V/110V a fuente 5V), la pantalla LCD y el aro de luz LED comenzarán a indicar el proceso de diagnóstico y carga de archivos base (`config.json`, `butons.json`, `hardware.json`).

> **[ INSERTE AQUÍ CAPTURA/FOTO DE LA PANTALLA LCD INICIANDO ]**

**Validaciones de Arranque:**
- **I2C y Pantalla:** Valida la comunicación para mostrar mensajes.
- **Memoria Interna (LittleFS):** Valida que las configuraciones de red, montos de venta y configuraciones de pines existan y sean JSON válidos.
- **Transbank POS IM30:** Verifica que el terminal de pago esté encendido y responda al comando de comunicación. Si falla 5 veces consecutivas, arroja Error 201 y la máquina se bloquea.

---

## 2. Pantalla de Reposo (IDLE) y Disponibilidad

Una vez iniciada correctamente, la pantalla mostrará el nombre del sistema (configurado en `config.json`) y el Aro LED reproducirá un efecto de "Estrellas". 
En este estado, los botones configurados como `"venta"` estarán iluminados e invitarán al usuario a realizar una compra.

> **[ INSERTE AQUÍ CAPTURA/FOTO DE LOS BOTONES DE VENTA ILUMINADOS ]**

**Monitoreo Automático Interno:**
- **Relleno del Estanque:** Si el estanque baja, y hay presión de entrada de agua, la válvula de entrada se abre automáticamente.
- **Oxigenación:** Cada cierto intervalo de tiempo (configurado en minutos), se enciende un motor de aire por unos minutos para mantener el agua fresca.
- **Alerta de Falta de Agua:** Si el sensor inferior marca vacío total, la máquina entra en estado **"Fuera de Servicio"** y bloquea temporalmente los botones de venta hasta que el nivel de agua vuelva a subir.

---

## 3. Realizando una Compra

### 3.1. Selección del Producto
El usuario presiona uno de los botones de venta iluminados (ej. "Llenado 20 Lts"). 
El sistema desactiva los otros botones. El botón seleccionado comienza a parpadear o "respirar".

> **[ INSERTE AQUÍ CAPTURA/FOTO DEL BOTÓN PARPADEANDO O LCD CON EL PRECIO ]**

### 3.2. Pago en Transbank
La pantalla LCD indica el monto a cobrar y transfiere el control al terminal Transbank IM30.

> **[ INSERTE AQUÍ CAPTURA/FOTO DEL TERMINAL TRANSBANK IM30 ESPERANDO TARJETA ]**

El usuario acerca su tarjeta, inserta el chip o usa banda magnética. El resultado puede ser:
- **Pago Aprobado:** Pasa a la siguiente fase de habilitación.
- **Pago Rechazado/Cancelado:** La LCD muestra error, el Aro LED destella rojo, y el sistema vuelve a reposo.

---

## 4. Proceso de Dispensado

Una vez aprobado el pago, el botón de selección se apaga y se enciende el botón **"Iniciar"** (verde, típicamente).
La pantalla indica "Venta Aprobada, Presione INICIAR".

> **[ INSERTE AQUÍ CAPTURA/FOTO DE LA PANTALLA DE VENTA APROBADA Y BOTÓN INICIAR ]**

### 4.1. Entrega de Agua
Al presionar "Iniciar":
1. Se activa el relé de la bomba de dispensado.
2. Se enciende la luz interna de la cabina del bidón.
3. El botón "Iniciar" se apaga y se enciende el botón de parada de emergencia (**"Stop"**).
4. La LCD muestra una barra de progreso. El Aro LED actúa como un reloj circular (se llena conforme avanza el tiempo).

> **[ INSERTE AQUÍ CAPTURA/FOTO DE LA BARRA DE PROGRESO EN LCD Y EL ARO LED ]**

### 4.2. Detención
El dispensado finaliza automáticamente cuando se cumple el `tiempo_ms` del producto.
Si el usuario requiere parar antes de tiempo (bidón lleno, emergencia), puede pulsar el botón **"Stop"**.

---

## 5. Fin de la Venta
Al detenerse la bomba, el aro LED realiza una animación corta en color verde para celebrar que el proceso terminó con éxito. La LCD muestra "Gracias por su compra".
Tras 3 segundos, la máquina vuelve al estado de reposo (IDLE), lista para el siguiente cliente.

> **[ INSERTE AQUÍ CAPTURA/FOTO DE LA PANTALLA "GRACIAS POR SU COMPRA" ]**

---

## 6. Resolución de Problemas Frecuentes

- **Máquina congelada en Error 1xx:** Fallo en la memoria del ESP32. Requiere reconectar USB y subir datos de LittleFS usando el Arduino IDE.
- **Error 201 persistente:** El cable serial entre el ESP32 y el puerto RS232 del IM30 podría estar desconectado o los baudrates no coinciden.
- **Máquina no da agua pero cobra:** Verificar que el relé de "bomba_dispensador" emita el sonido "click" y que la bomba en sí no tenga aire (necesite purgado). El modo prueba de hardware (vía web o monitor serie) es útil para forzar la bomba.
