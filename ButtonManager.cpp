#include "ButtonManager.h"

ButtonManager::ButtonManager(Adafruit_MCP23X17& mcpRef) : mcp(mcpRef) {
    numButtons = 0;
    lastBlinkTime = 0;
    blinkState = false;
    pressedId = -1;
    for(int i=0; i<10; i++) {
        btnEnabled[i] = false;
        btnBlinking[i] = false;
    }
}

void ButtonManager::init(ButtonConfig* configArray, int count) {
    buttons = configArray;
    numButtons = count;
    disableAll();
}

void ButtonManager::disableAll() {
    for (int i = 0; i < numButtons; i++) {
        setMode(buttons[i].id, false);
    }
}

void ButtonManager::setMode(int id, bool enabled, bool blinking) {
    for (int i = 0; i < numButtons; i++) {
        if (buttons[i].id == id) {
            btnEnabled[i] = enabled;
            btnBlinking[i] = blinking;
            
            if (!enabled) {
                // Desactivado: apagado completamente
                mcp.pinMode(buttons[i].pin, OUTPUT);
                mcp.digitalWrite(buttons[i].pin, LOW);
            }
            break;
        }
    }
}

void ButtonManager::update() {
    unsigned long currentMillis = millis();
    // Parpadeo lento de 300ms
    if (currentMillis - lastBlinkTime >= 300) {
        lastBlinkTime = currentMillis;
        blinkState = !blinkState;
    }
    
    pressedId = -1;
    
    for (int i = 0; i < numButtons; i++) {
        if (!btnEnabled[i]) continue;
        
        bool shouldBeLit = true;
        if (btnBlinking[i]) {
            shouldBeLit = blinkState;
        }
        
        // Poner brevemente en modo lectura para evitar cortos
        mcp.pinMode(buttons[i].pin, INPUT_PULLUP);
        delayMicroseconds(50); // Pequeño retardo para estabilizar voltaje
        
        bool isPressed = (mcp.digitalRead(buttons[i].pin) == LOW);
        
        if (isPressed) {
            pressedId = buttons[i].id;
            // IMPORTANTE: Si está presionado, dejarlo como INPUT_PULLUP
            // para no generar un cortocircuito a GND si se pusiera en OUTPUT HIGH
            mcp.pinMode(buttons[i].pin, INPUT_PULLUP);
        } else {
            // Si no está presionado, restaurar el estado de iluminación
            mcp.pinMode(buttons[i].pin, OUTPUT);
            if (shouldBeLit) {
                mcp.digitalWrite(buttons[i].pin, HIGH);
            } else {
                mcp.digitalWrite(buttons[i].pin, LOW);
            }
        }
    }
}

int ButtonManager::getPressedButtonId() {
    return pressedId;
}
