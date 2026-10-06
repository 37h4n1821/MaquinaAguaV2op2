#ifndef APPLOGIC_H
#define APPLOGIC_H

#include "DisplayManager.h"
#include "LEDManager.h"
#include "ButtonManager.h"
#include "HardwareManager.h"
#include "TransbankIM30.h"
#include "LoggerManager.h"

enum AppState {
    STATE_IDLE,
    STATE_FUERA_SERVICIO_AGUA,
    STATE_SELECCION_VENTA,
    STATE_ESPERA_PAGO,
    STATE_PAGO_APROBADO,
    STATE_DISPENSANDO,
    STATE_FINALIZADO,
    STATE_HARDWARE_TEST
};

class AppLogic {
public:
    AppLogic(DisplayManager& disp, LEDManager& led, ButtonManager& btn, HardwareManager& hw, TransbankIM30& posRef, String name, ButtonConfig* btns, int numBtns);
    void update();
    void setState(AppState newState);
    void setTestMode(bool enable);
    void calibrateDispense(unsigned long timeMs);
    void recordSale(long monto, unsigned long tiempoMs);

private:
    DisplayManager& display;
    LEDManager& leds;
    ButtonManager& buttons;
    HardwareManager& hardware;
    TransbankIM30& pos;
    String systemName;
    ButtonConfig* btnConfig;
    int numButtons;
    
    AppState currentState;
    
    ButtonConfig* getButtonById(int id);
    
    ButtonConfig* currentSaleBtn;
    unsigned long stateStartTime;
    bool inTestMode = false;
    ButtonConfig dummyTestBtn;
};

#endif
