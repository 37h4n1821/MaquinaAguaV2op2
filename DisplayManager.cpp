#include "DisplayManager.h"

DisplayManager::DisplayManager(LiquidCrystal_I2C& lcdRef) : lcd(lcdRef) {}

void DisplayManager::init() {
    lcd.init();
    lcd.backlight();
    lcd.clear();
}

void DisplayManager::printCenter(String text, int row) {
    int spaces = (20 - text.length()) / 2;
    if (spaces < 0) spaces = 0;
    lcd.setCursor(spaces, row);
    lcd.print(text);
}

void DisplayManager::showBooting(int step, int totalSteps, String msg) {
    lcd.clear();
    printCenter("Iniciando sistema", 1);
    printCenter(msg, 3);
}

void DisplayManager::showIdle(String systemName) {
    lcd.clear();
    printCenter(systemName, 0);
    printCenter("Sistema Listo", 1);
    printCenter("Seleccione opcion", 3);
}

void DisplayManager::showSaleSelection(String text, long monto) {
    lcd.clear();
    printCenter("Opcion: " + text, 1);
    printCenter("Valor: $" + String(monto), 2);
}

void DisplayManager::showWaitPayment() {
    lcd.clear();
    printCenter("Siga instrucciones", 1);
    printCenter("en el terminal...", 2);
}

void DisplayManager::showSaleApproved() {
    lcd.clear();
    printCenter("VENTA APROBADA", 1);
    printCenter("Presione INICIAR", 2);
}

void DisplayManager::showSaleCancelled() {
    lcd.clear();
    printCenter("VENTA CANCELADA", 1);
    printCenter("POR EL USUARIO", 2);
}

void DisplayManager::showDispensing(unsigned long current, unsigned long total) {
    // Only update the progress bar line if we already printed the header
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < 200 && current > 0) return; // limit updates to avoid LCD flicker
    lastUpdate = millis();

    if (current == 0) {
        lcd.clear();
        printCenter("Entregando agua...", 0);
    }
    
    int progressBars = 0;
    if (total > 0) {
        progressBars = (20 * current) / total;
    }
    
    lcd.setCursor(0, 2);
    for(int i=0; i<20; i++) {
        if(i < progressBars) lcd.print((char)0xFF); // Block char
        else lcd.print(" ");
    }
}

void DisplayManager::showFinished() {
    lcd.clear();
    printCenter("Gracias por", 1);
    printCenter("su compra", 2);
}

void DisplayManager::showError(int code) {
    lcd.clear();
    printCenter("FUERA DE SERVICIO", 1);
    printCenter("COD. ERROR: " + String(code), 2);
}

void DisplayManager::showTestMode() {
    lcd.clear();
    printCenter("  MODO DE PRUEBA  ", 1);
    printCenter("     HARDWARE     ", 2);
}
