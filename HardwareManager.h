#ifndef HARDWAREMANAGER_H
#define HARDWAREMANAGER_H

#include <Adafruit_MCP23X17.h>
#include <Arduino.h>

struct HardwareConfig {
    uint8_t pinNivelInf;
    uint8_t pinNivelSup;
    uint8_t pinAguaEntrada;
    uint8_t pinAguaOsmosis;
    uint8_t pinAguaDisp;
    bool flotadorNC;
    bool sensoresAguaInvertidos;
    
    uint8_t pinLuzBidon;
    uint8_t pinMotorAire;
    uint8_t pinValvulaLlenado;
    uint8_t pinBombaDisp;
    bool releActivoLow;
    
    bool reqRedAgua;
    unsigned long intervaloOxigenacionMs;
    unsigned long duracionOxigenacionMs;
};

class HardwareManager {
public:
    HardwareManager(Adafruit_MCP23X17& mcpRef);
    void init(HardwareConfig config);
    void update();
    
    bool canSell(); 
    void startDispensing();
    void stopDispensing();
    bool isWaterDispensing();
    
    bool isAirMotorOn() { return isOxigenando; }
    unsigned long getAirMotorTimeLeft();
    
    void setBottleLight(bool state);
    void setAirMotor(bool state);
    void setRefillValve(bool state);
    
    bool getSensorRaw(uint8_t pin);
    bool getRelayState(uint8_t pin);
    
    bool getWaterSensorState(uint8_t pin);
    bool getFloatSensorState(uint8_t pin);
    
    void enableAutoControl(bool enable);
    
    HardwareConfig getConfig() { return cfg; }

private:
    Adafruit_MCP23X17& mcp;
    HardwareConfig cfg;
    
    unsigned long lastOxigenacionTime;
    bool isOxigenando;
    bool isRefilling;
    bool autoControlEnabled;
    
    void setRelay(uint8_t pin, bool active);
    bool isTankFull();
    bool isTankEmpty();
    bool hasInputWater();
    bool hasOsmosisWater();
};

#endif
