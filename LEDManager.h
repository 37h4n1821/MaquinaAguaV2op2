#ifndef LEDMANAGER_H
#define LEDMANAGER_H

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

class LEDManager {
public:
    LEDManager(Adafruit_NeoPixel& ringRef, int count);
    void init();
    void setProgress(int step, int totalSteps, uint32_t color);
    void showError();
    void showStarsEffect();
    void showWaitEffect();
    void showSuccess();
    void showDispensingProgress(unsigned long elapsed, unsigned long total);
    void clear();

private:
    Adafruit_NeoPixel& ring;
    int ledCount;
    unsigned long lastStarUpdate;
};

#endif
