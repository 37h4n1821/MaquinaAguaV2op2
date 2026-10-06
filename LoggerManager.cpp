#include "LoggerManager.h"

LoggerManager Logger;

void LoggerManager::init() {
    rotateLogs();
    currentLogFile = LittleFS.open("/logs/log1.txt", "w");
    if (!currentLogFile) {
        Serial.println("ERROR: No se pudo crear el archivo log1.txt");
    } else {
        println("=== NUEVO REGISTRO DE LOG INICIADO ===");
    }
}

void LoggerManager::rotateLogs() {
    if (!LittleFS.exists("/logs")) {
        LittleFS.mkdir("/logs");
    }
    
    if (LittleFS.exists("/logs/log3.txt")) LittleFS.remove("/logs/log3.txt");
    if (LittleFS.exists("/logs/log2.txt")) LittleFS.rename("/logs/log2.txt", "/logs/log3.txt");
    if (LittleFS.exists("/logs/log1.txt")) LittleFS.rename("/logs/log1.txt", "/logs/log2.txt");
}

void LoggerManager::print(String msg) {
    Serial.print(msg);
    if (currentLogFile) {
        currentLogFile.print(msg);
        currentLogFile.flush();
    }
}

void LoggerManager::println(String msg) {
    // Prefix with millis for a simple timestamp
    String logMsg = "[" + String(millis()) + "] " + msg;
    Serial.println(logMsg);
    if (currentLogFile) {
        currentLogFile.println(logMsg);
        currentLogFile.flush();
    }
}

void LoggerManager::processSerialCommands() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "READ_LOG_1") {
            printLogFile("/logs/log1.txt");
        } else if (cmd == "READ_LOG_2") {
            printLogFile("/logs/log2.txt");
        } else if (cmd == "READ_LOG_3") {
            printLogFile("/logs/log3.txt");
        } else if (cmd == "HELP") {
            Serial.println("Comandos disponibles: READ_LOG_1, READ_LOG_2, READ_LOG_3");
        }
    }
}

void LoggerManager::printLogFile(String path) {
    Serial.println("\n=== LEYENDO " + path + " ===");
    File f = LittleFS.open(path, "r");
    if (!f) {
        Serial.println("Archivo no existe.");
        return;
    }
    while (f.available()) {
        Serial.write(f.read());
    }
    f.close();
    Serial.println("\n=== FIN DE LOG ===\n");
}
