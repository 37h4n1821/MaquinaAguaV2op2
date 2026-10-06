#include "AppLogic.h"

AppLogic::AppLogic(DisplayManager& disp, LEDManager& led, ButtonManager& btn, HardwareManager& hw, TransbankIM30& posRef, String name, ButtonConfig* btns, int numBtns)
    : display(disp), leds(led), buttons(btn), hardware(hw), pos(posRef), systemName(name), btnConfig(btns), numButtons(numBtns) {
    currentState = STATE_IDLE;
    currentSaleBtn = nullptr;
    inTestMode = false;
    
    dummyTestBtn.monto = 0;
    dummyTestBtn.tiempo_ms = 0;
    dummyTestBtn.accion = "";
}

ButtonConfig* AppLogic::getButtonById(int id) {
    for (int i = 0; i < numButtons; i++) {
        if (btnConfig[i].id == id) return &btnConfig[i];
    }
    return nullptr;
}

void AppLogic::setState(AppState newState) {
    currentState = newState;
    stateStartTime = millis();
    
    switch (currentState) {
        case STATE_HARDWARE_TEST:
            Logger.println("ESTADO: MODO PRUEBA HARDWARE");
            display.showTestMode();
            buttons.disableAll();
            hardware.stopDispensing();
            hardware.setBottleLight(false);
            hardware.setAirMotor(false);
            hardware.setRefillValve(false);
            break;
            
        case STATE_IDLE:
            Logger.println("ESTADO: IDLE");
            display.showIdle(systemName);
            buttons.disableAll();
            // Habilitar solo botones de venta
            for (int i = 0; i < numButtons; i++) {
                if (btnConfig[i].tipo == "venta") {
                    buttons.setMode(btnConfig[i].id, true, false); 
                }
            }
            break;
            
        case STATE_FUERA_SERVICIO_AGUA:
            Logger.println("ESTADO: FUERA DE SERVICIO (Sin Agua)");
            display.showError(300); // 300 = Sin agua
            leds.showError();
            buttons.disableAll();
            break;
            
        case STATE_SELECCION_VENTA:
            if (currentSaleBtn) {
                display.showSaleSelection(currentSaleBtn->texto, currentSaleBtn->monto);
                buttons.disableAll();
                buttons.setMode(currentSaleBtn->id, true, true); // Parpadeo
                
                // Iniciar venta (bloqueante por ahora), manteniendo la selección en pantalla
                leds.showWaitEffect();
                
                Logger.println("Iniciando venta POS: $" + String(currentSaleBtn->monto));
                String ticket = String(millis());
                int saleResult = pos.startSale(currentSaleBtn->monto, ticket);
                
                if (saleResult == 0) {
                    Logger.println("Venta APROBADA por POS.");
                    setState(STATE_PAGO_APROBADO);
                } else if (saleResult == 1) {
                    Logger.println("Venta CANCELADA por el usuario.");
                    display.showSaleCancelled();
                    leds.showError(); // Puedes cambiar a un color distinto si quieres
                    delay(3000);
                    setState(STATE_IDLE);
                } else {
                    Logger.println("Venta rechazada/fallida por error. Cod: 202");
                    display.showError(202);
                    leds.showError();
                    delay(3000);
                    setState(STATE_IDLE);
                }
            }
            break;
            
        case STATE_ESPERA_PAGO:
            // Omitido ya que startSale es bloqueante. Si se hace asíncrono, se maneja aquí.
            break;

        case STATE_PAGO_APROBADO:
            display.showSaleApproved();
            leds.showSuccess();
            buttons.disableAll();
            // Habilitar boton INICIAR con parpadeo
            for (int i = 0; i < numButtons; i++) {
                if (btnConfig[i].accion == "iniciar") {
                    buttons.setMode(btnConfig[i].id, true, true);
                }
            }
            break;
            
        case STATE_DISPENSANDO:
            Logger.println("ESTADO: DISPENSANDO AGUA");
            display.showDispensing(0, 100); 
            buttons.disableAll();
            // Habilitar boton STOP
            for (int i = 0; i < numButtons; i++) {
                if (btnConfig[i].accion == "stop") {
                    buttons.setMode(btnConfig[i].id, true, false); 
                }
            }
            hardware.setBottleLight(true);
            hardware.startDispensing();
            Logger.println("RELES ACTIVADOS: Dispensador y Luz");
            break;
            
        case STATE_FINALIZADO:
            Logger.println("ESTADO: VENTA FINALIZADA");
            display.showFinished();
            leds.showSuccess();
            buttons.disableAll();
            hardware.stopDispensing();
            hardware.setBottleLight(false);
            Logger.println("RELES DESACTIVADOS");
            break;
    }
}

void AppLogic::update() {
    hardware.update(); // Actualizar niveles y oxígeno
    buttons.update(); // Actualiza multiplexado y parpadeo
    int pressedId = buttons.getPressedButtonId();
    
    if (inTestMode && currentState != STATE_HARDWARE_TEST && currentState != STATE_DISPENSANDO) {
        // En modo test, ignoramos el update habitual, excepto para dispensar de prueba.
        // El test de relés/sensores se hace directo al HardwareManager
        return;
    }
    
    switch (currentState) {
        case STATE_HARDWARE_TEST:
            // No hacemos nada de logica de venta en este estado.
            break;
            
        case STATE_IDLE:
            if (!hardware.canSell()) {
                setState(STATE_FUERA_SERVICIO_AGUA);
            } else {
                leds.showStarsEffect();
                if (pressedId != -1) {
                    ButtonConfig* btn = getButtonById(pressedId);
                    if (btn && btn->tipo == "venta") {
                        currentSaleBtn = btn;
                        setState(STATE_SELECCION_VENTA);
                    }
                }
            }
            break;
            
        case STATE_FUERA_SERVICIO_AGUA:
            if (hardware.canSell()) {
                Logger.println("Agua detectada nuevamente, regresando a IDLE.");
                setState(STATE_IDLE);
            }
            break;
            
        case STATE_SELECCION_VENTA:
        case STATE_ESPERA_PAGO:
            // Nada que hacer aquí mientras startSale sea síncrono.
            break;
            
        case STATE_PAGO_APROBADO:
            if (pressedId != -1) {
                ButtonConfig* btn = getButtonById(pressedId);
                if (btn && btn->accion == "iniciar") {
                    setState(STATE_DISPENSANDO);
                }
            }
            break;
            
        case STATE_DISPENSANDO: {
            unsigned long elapsed = millis() - stateStartTime;
            if (currentSaleBtn) {
                if (elapsed >= currentSaleBtn->tiempo_ms) {
                    if (!inTestMode) {
                        recordSale(currentSaleBtn->monto, currentSaleBtn->tiempo_ms);
                    }
                    setState(STATE_FINALIZADO);
                } else {
                    display.showDispensing(elapsed, currentSaleBtn->tiempo_ms);
                    leds.showDispensingProgress(elapsed, currentSaleBtn->tiempo_ms);
                }
            }
            
            // Chequear si presionó STOP
            if (pressedId != -1) {
                ButtonConfig* btn = getButtonById(pressedId);
                if (btn && btn->accion == "stop") {
                    setState(STATE_FINALIZADO);
                }
            }
            break;
        }
            
        case STATE_FINALIZADO:
            if (millis() - stateStartTime > 3000) {
                if (inTestMode) {
                    setState(STATE_HARDWARE_TEST);
                } else {
                    setState(STATE_IDLE);
                }
            }
            break;
    }
}

void AppLogic::setTestMode(bool enable) {
    inTestMode = enable;
    hardware.enableAutoControl(!enable);
    if (enable) {
        setState(STATE_HARDWARE_TEST);
    } else {
        setState(STATE_IDLE);
    }
}

void AppLogic::calibrateDispense(unsigned long timeMs) {
    inTestMode = true;
    dummyTestBtn.tiempo_ms = timeMs;
    currentSaleBtn = &dummyTestBtn;
    setState(STATE_DISPENSANDO);
}

void AppLogic::recordSale(long monto, unsigned long tiempoMs) {
    // Si no está el archivo, colocar encabezado
    bool addHeader = !LittleFS.exists("/ventas.csv");
    File f = LittleFS.open("/ventas.csv", "a");
    if (f) {
        if (addHeader) {
            f.println("FechaHora,Monto,Tiempo_ms");
        }
        
        // Obtener hora (requiere libreria time.h configurada, si falla usa millis)
        struct tm timeinfo;
        String fecha = "No-NTP-" + String(millis());
        if(getLocalTime(&timeinfo, 10)) {
            char timeStringBuff[50];
            strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
            fecha = String(timeStringBuff);
        }
        
        f.printf("%s,%ld,%lu\n", fecha.c_str(), monto, tiempoMs);
        f.close();
        Logger.println("Venta registrada en CSV: $" + String(monto));
    } else {
        Logger.println("ERROR: No se pudo abrir ventas.csv para escribir.");
    }
}
