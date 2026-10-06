#ifndef BUTTONMANAGER_H
#define BUTTONMANAGER_H

#include <Adafruit_MCP23X17.h>
#include <Arduino.h>

struct ButtonConfig {
    int id;
    uint8_t pin;
    String tipo;
    long monto;
    long tiempo_ms;
    String texto;
    String accion;
};

class ButtonManager {
public:
    ButtonManager(Adafruit_MCP23X17& mcpRef);
    void init(ButtonConfig* configArray, int count);
    void setMode(int id, bool enabled, bool blinking = false);
    void disableAll();
    void update(); // Debe llamarse en el loop para parpadeo y lectura
    int getPressedButtonId(); // Retorna el ID del boton presionado, o -1

private:
    Adafruit_MCP23X17& mcp;
    ButtonConfig* buttons;
    int numButtons;
    
    bool btnEnabled[10];
    bool btnBlinking[10];
    
    unsigned long lastBlinkTime;
    bool blinkState;
    
    int pressedId;
};

#endif
