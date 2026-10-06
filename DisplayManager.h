#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <LiquidCrystal_I2C.h>
#include <Arduino.h>

class DisplayManager {
public:
    DisplayManager(LiquidCrystal_I2C& lcdRef);
    void init();
    void showBooting(int step, int totalSteps, String msg);
    void showIdle(String systemName);
    void showSaleSelection(String text, long monto);
    void showWaitPayment();
    void showSaleApproved();
    void showSaleCancelled();
    void showDispensing(unsigned long current, unsigned long total);
    void showFinished();
    void showError(int code);
    void showTestMode();

private:
    LiquidCrystal_I2C& lcd;
    void printCenter(String text, int row);
};

#endif
