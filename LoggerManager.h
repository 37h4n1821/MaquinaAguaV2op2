#ifndef LOGGERMANAGER_H
#define LOGGERMANAGER_H

#include <Arduino.h>
#include <LittleFS.h>

class LoggerManager {
public:
    void init();
    void println(String msg);
    void print(String msg);
    void processSerialCommands();

private:
    File currentLogFile;
    void rotateLogs();
    void printLogFile(String path);
};

extern LoggerManager Logger;

#endif
