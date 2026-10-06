#include "HardwareManager.h"

HardwareManager::HardwareManager(Adafruit_MCP23X17& mcpRef) : mcp(mcpRef) {
    lastOxigenacionTime = 0;
    isOxigenando = false;
    isRefilling = false;
}

void HardwareManager::init(HardwareConfig config) {
    cfg = config;
    
    // Configurar sensores como entradas con pullup
    mcp.pinMode(cfg.pinNivelInf, INPUT_PULLUP);
    mcp.pinMode(cfg.pinNivelSup, INPUT_PULLUP);
    mcp.pinMode(cfg.pinAguaEntrada, INPUT_PULLUP);
    mcp.pinMode(cfg.pinAguaOsmosis, INPUT_PULLUP);
    mcp.pinMode(cfg.pinAguaDisp, INPUT_PULLUP);
    
    // Escribir los estados apagados ANTES de configurar como salida para evitar micro-encendidos de relés
    setBottleLight(false);
    setAirMotor(false);
    setRefillValve(false);
    stopDispensing();

    // Configurar relés como salidas
    mcp.pinMode(cfg.pinLuzBidon, OUTPUT);
    mcp.pinMode(cfg.pinMotorAire, OUTPUT);
    mcp.pinMode(cfg.pinValvulaLlenado, OUTPUT);
    mcp.pinMode(cfg.pinBombaDisp, OUTPUT);
    
    autoControlEnabled = true;
    lastOxigenacionTime = millis() - cfg.intervaloOxigenacionMs;
}

void HardwareManager::setRelay(uint8_t pin, bool active) {
    if (cfg.releActivoLow) {
        mcp.digitalWrite(pin, active ? LOW : HIGH);
    } else {
        mcp.digitalWrite(pin, active ? HIGH : LOW);
    }
}

void HardwareManager::startDispensing() {
    setRelay(cfg.pinBombaDisp, true);
}

void HardwareManager::stopDispensing() {
    setRelay(cfg.pinBombaDisp, false);
}

void HardwareManager::setBottleLight(bool state) {
    setRelay(cfg.pinLuzBidon, state);
}

void HardwareManager::setAirMotor(bool state) {
    setRelay(cfg.pinMotorAire, state);
    isOxigenando = state;
}

unsigned long HardwareManager::getAirMotorTimeLeft() {
    unsigned long elapsed = millis() - lastOxigenacionTime;
    if (isOxigenando) {
        if (elapsed < cfg.duracionOxigenacionMs) return cfg.duracionOxigenacionMs - elapsed;
    } else {
        if (elapsed < cfg.intervaloOxigenacionMs) return cfg.intervaloOxigenacionMs - elapsed;
    }
    return 0;
}

void HardwareManager::setRefillValve(bool state) {
    setRelay(cfg.pinValvulaLlenado, state);
    isRefilling = state;
}

bool HardwareManager::getWaterSensorState(uint8_t pin) {
    bool detected = mcp.digitalRead(pin) == LOW; // LOW indica detección por defecto
    if (cfg.sensoresAguaInvertidos) {
        return !detected;
    }
    return detected;
}

bool HardwareManager::getFloatSensorState(uint8_t pin) {
    bool val = (mcp.digitalRead(pin) == HIGH);
    if (!cfg.flotadorNC) val = !val;
    return val;
}

bool HardwareManager::isWaterDispensing() {
    return getWaterSensorState(cfg.pinAguaDisp);
}

bool HardwareManager::hasInputWater() {
    return getWaterSensorState(cfg.pinAguaEntrada); 
}

bool HardwareManager::hasOsmosisWater() {
    return getWaterSensorState(cfg.pinAguaOsmosis);
}

bool HardwareManager::isTankFull() {
    return getFloatSensorState(cfg.pinNivelSup);
}

bool HardwareManager::isTankEmpty() {
    return !getFloatSensorState(cfg.pinNivelInf);
}

bool HardwareManager::canSell() {
    return !isTankEmpty();
}

bool HardwareManager::getSensorRaw(uint8_t pin) {
    return mcp.digitalRead(pin) == LOW; // Asumimos LOW es activo para la mayoria, o devolvemos estado directo
}

bool HardwareManager::getRelayState(uint8_t pin) {
    bool raw = mcp.digitalRead(pin);
    if (cfg.releActivoLow) return raw == LOW;
    return raw == HIGH;
}

void HardwareManager::enableAutoControl(bool enable) {
    autoControlEnabled = enable;
    if (!enable) {
        setRefillValve(false);
        setAirMotor(false);
        stopDispensing();
        setBottleLight(false);
    }
}

void HardwareManager::update() {
    if (!autoControlEnabled) return;
    
    unsigned long currentMillis = millis();
    
    // Lógica de llenado automático con DEBOUNCE (anti micro-encendidos)
    static unsigned long waterDetectedTime = 0;
    static unsigned long waterLostTime = 0;

    if (!isTankFull()) {
        if (hasInputWater()) {
            waterLostTime = 0;
            if (waterDetectedTime == 0) waterDetectedTime = currentMillis;
            
            // Esperar 1 segundo de sensor estable antes de abrir la válvula
            if (!isRefilling && (currentMillis - waterDetectedTime > 1000)) {
                setRefillValve(true);
            }
        } else {
            waterDetectedTime = 0;
            if (isRefilling) {
                if (waterLostTime == 0) waterLostTime = currentMillis;
                
                // Esperar 2 segundos sin detectar agua antes de cerrar para evitar oscilaciones de presión
                if (currentMillis - waterLostTime > 2000) {
                    setRefillValve(false);
                }
            }
        }
    } else {
        waterDetectedTime = 0;
        waterLostTime = 0;
        if (isRefilling) setRefillValve(false);
    }
    
    // Lógica de oxigenación
    if (!isOxigenando) {
        if (currentMillis - lastOxigenacionTime >= cfg.intervaloOxigenacionMs) {
            setAirMotor(true);
            lastOxigenacionTime = currentMillis;
        }
    } else {
        if (currentMillis - lastOxigenacionTime >= cfg.duracionOxigenacionMs) {
            setAirMotor(false);
            lastOxigenacionTime = currentMillis;
        }
    }
}
