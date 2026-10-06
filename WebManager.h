#ifndef WEBMANAGER_H
#define WEBMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

class WebManager {
public:
    WebManager();
    void init(String host, String sta_ssid, String sta_pass, String ap_ssid, String ap_pass, String user, String pass);
    void update();

private:
    WebServer server;
    String webUser;
    String webPass;

    bool handleAuth();
    void registerApis();
    
    void apiStatus();
    void apiTestRelay();
    void apiGetLogs();
    
    void apiTestMode();
    void apiConfigGet();
    void apiConfigSave();
    void apiVentasCSV();
    void apiClearVentas();
    void apiReboot();
    void apiTestDispense();
    void apiTestButton();
};

#endif
