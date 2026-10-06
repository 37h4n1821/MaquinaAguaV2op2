#include "LEDManager.h"

LEDManager::LEDManager(Adafruit_NeoPixel& ringRef, int count) : ring(ringRef), ledCount(count) {
    lastStarUpdate = 0;
}

void LEDManager::init() {
    ring.begin();
    ring.setBrightness(50);
    ring.clear();
    ring.show();
}

void LEDManager::clear() {
    ring.clear();
    ring.show();
}

void LEDManager::setProgress(int step, int totalSteps, uint32_t color) {
    ring.clear();
    int ledsToLight = (ledCount * step) / totalSteps;
    for (int i = 0; i < ledsToLight; i++) {
        ring.setPixelColor(i, color);
    }
    ring.show();
}

void LEDManager::showError() {
    ring.clear();
    for (int i = 0; i < ledCount; i++) {
        ring.setPixelColor(i, ring.Color(255, 0, 0));
    }
    ring.show();
}

void LEDManager::showSuccess() {
    ring.clear();
    for (int i = 0; i < ledCount; i++) {
        ring.setPixelColor(i, ring.Color(0, 255, 0));
    }
    ring.show();
}

void LEDManager::showStarsEffect() {
    if (millis() - lastStarUpdate > 100) { // Update every 100ms
        lastStarUpdate = millis();
        // Fade all LEDs slightly to create the breathing/twinkling effect
        for(int i=0; i<ledCount; i++) {
            uint32_t col = ring.getPixelColor(i);
            uint8_t r = (uint8_t)(col >> 16);
            uint8_t g = (uint8_t)(col >> 8);
            uint8_t b = (uint8_t)col;
            if(r > 0) r--;
            if(g > 0) g--;
            if(b > 0) b -= 2; // Fade blue faster
            ring.setPixelColor(i, ring.Color(r,g,b));
        }
        
        // Randomly turn on a "star"
        if (random(10) > 6) {
            int led = random(ledCount);
            // Estrellas no tan fuertes, tonos azules
            ring.setPixelColor(led, ring.Color(0, random(20, 50), random(100, 200))); 
        }
        ring.show();
    }
}

void LEDManager::showWaitEffect() {
    // Static or rotating yellow effect
    ring.clear();
    for(int i=0; i<ledCount; i+=3) {
        ring.setPixelColor(i, ring.Color(255, 200, 0));
    }
    ring.show();
}

void LEDManager::showDispensingProgress(unsigned long elapsed, unsigned long total) {
    if (total == 0) return;
    
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < 50) return; // 50ms limit for ultra smooth led progress
    lastUpdate = millis();
    
    int leds = (ledCount * elapsed) / total;
    ring.clear();
    for(int i=0; i<leds; i++) {
        ring.setPixelColor(i, ring.Color(0, 0, 255));
    }
    ring.show();
}
