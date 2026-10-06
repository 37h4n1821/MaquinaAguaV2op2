#include "WebManager.h"
#include "HardwareManager.h"
#include "LoggerManager.h"
#include "ButtonManager.h"
#include "AppLogic.h"

extern HardwareManager hardware;
extern ButtonManager buttons;
extern AppLogic* appLogic;

WebManager::WebManager() : server(80) {}

void WebManager::init(String host, String sta_ssid, String sta_pass, String ap_ssid, String ap_pass, String user, String pass) {
    webUser = user;
    webPass = pass;
    
    WiFi.setHostname(host.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(sta_ssid.c_str(), sta_pass.c_str());
    
    Logger.println("Conectando a WiFi " + sta_ssid);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 15) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Logger.println("WiFi Conectado! IP: " + WiFi.localIP().toString());
        // Init NTP
        configTime(-14400, 3600, "pool.ntp.org", "time.nist.gov"); // GMT-4
        Logger.println("NTP Configurado");
    } else {
        Logger.println("Fallo WiFi STA. Iniciando Modo AP...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ap_ssid.c_str(), ap_pass.c_str());
        Logger.println("AP Iniciado. IP: " + WiFi.softAPIP().toString());
    }
    
    // Configurar recoleccion de headers para leer Cookies
    const char* headerkeys[] = {"Cookie"};
    server.collectHeaders(headerkeys, 1);
    
    registerApis();
    
    // Archivos estáticos permitidos publicamente
    server.serveStatic("/style.css", LittleFS, "/www/style.css");
    server.serveStatic("/app.js", LittleFS, "/www/app.js");
    
    server.begin();
    Logger.println("Web Server iniciado en puerto 80");
}

bool WebManager::handleAuth() {
    if (server.hasHeader("Cookie")) {
        String cookie = server.header("Cookie");
        if (cookie.indexOf("SESSIONID=AGUA123") != -1) {
            return true;
        }
    }
    
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return false;
}

void WebManager::registerApis() {
    // Paginas web (Rutas Frontend)
    server.on("/", HTTP_GET, [this]() {
        if (server.hasHeader("Cookie") && server.header("Cookie").indexOf("SESSIONID=AGUA123") != -1) {
            File f = LittleFS.open("/www/index.html", "r");
            server.streamFile(f, "text/html");
            f.close();
        } else {
            server.sendHeader("Location", "/login");
            server.send(302, "text/plain", "Redirecting...");
        }
    });
    
    server.on("/login", HTTP_GET, [this]() {
        File f = LittleFS.open("/www/login.html", "r");
        server.streamFile(f, "text/html");
        f.close();
    });
    
    server.on("/api/login", HTTP_POST, [this]() {
        if (server.arg("user") == webUser && server.arg("pass") == webPass) {
            server.sendHeader("Set-Cookie", "SESSIONID=AGUA123; Path=/; HttpOnly");
            server.send(200, "text/plain", "OK");
        } else {
            server.send(401, "text/plain", "Invalid credentials");
        }
    });

    server.on("/api/logout", HTTP_POST, [this]() {
        server.sendHeader("Set-Cookie", "SESSIONID=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT");
        server.send(200, "text/plain", "OK");
    });
    server.on("/api/status", HTTP_GET, [this]() {
        if (!handleAuth()) return;
        apiStatus();
    });
    
    server.on("/api/test_relay", HTTP_POST, [this]() {
        if (!handleAuth()) return;
        apiTestRelay();
    });

    server.on("/api/logs", HTTP_GET, [this]() {
        if (!handleAuth()) return;
        apiGetLogs();
    });
    
    server.on("/api/ventas", HTTP_GET, [this]() {
        if (!handleAuth()) return;
        apiVentasCSV();
    });
    
    server.on("/api/ventas", HTTP_DELETE, [this]() {
        if (!handleAuth()) return;
        apiClearVentas();
    });
    
    server.on("/api/reboot", HTTP_POST, [this]() {
        if (!handleAuth()) return;
        apiReboot();
    });
    
    server.on("/api/config", HTTP_GET, [this]() {
        if (!handleAuth()) return;
        apiConfigGet();
    });
    
    server.on("/api/config", HTTP_POST, [this]() {
        if (!handleAuth()) return;
        apiConfigSave();
    });
    
    server.on("/api/test_mode", HTTP_POST, [this]() {
        if (!handleAuth()) return;
        apiTestMode();
    });
    
    server.on("/api/test_dispense", HTTP_POST, [this]() {
        if (!handleAuth()) return;
        apiTestDispense();
    });
    
    server.on("/api/test_button", HTTP_POST, [this]() {
        if (!handleAuth()) return;
        apiTestButton();
    });
}

void WebManager::update() {
    server.handleClient();
}

void WebManager::apiStatus() {
    HardwareConfig cfg = hardware.getConfig();
    
    StaticJsonDocument<512> doc;
    JsonObject sensores = doc.createNestedObject("sensores");
    sensores["nivel_inferior"] = hardware.getFloatSensorState(cfg.pinNivelInf);
    sensores["nivel_superior"] = hardware.getFloatSensorState(cfg.pinNivelSup);
    sensores["agua_entrada"] = hardware.getWaterSensorState(cfg.pinAguaEntrada);
    sensores["agua_osmosis"] = hardware.getWaterSensorState(cfg.pinAguaOsmosis);
    sensores["agua_dispensador"] = hardware.getWaterSensorState(cfg.pinAguaDisp);
    
    JsonObject reles = doc.createNestedObject("reles");
    reles["luz_bidon"] = hardware.getRelayState(cfg.pinLuzBidon);
    reles["motor_aire"] = hardware.getRelayState(cfg.pinMotorAire);
    reles["valvula_llenado"] = hardware.getRelayState(cfg.pinValvulaLlenado);
    reles["bomba_dispensador"] = hardware.getRelayState(cfg.pinBombaDisp);
    
    JsonObject oxigeno = doc.createNestedObject("oxigeno");
    oxigeno["estado"] = hardware.isAirMotorOn();
    oxigeno["tiempo_restante_ms"] = hardware.getAirMotorTimeLeft();
    oxigeno["intervalo_ms"] = cfg.intervaloOxigenacionMs;
    oxigeno["duracion_ms"] = cfg.duracionOxigenacionMs;
    
    doc["canSell"] = hardware.canSell();
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebManager::apiTestRelay() {
    if (server.hasArg("relay") && server.hasArg("state")) {
        String relay = server.arg("relay");
        bool state = server.arg("state") == "1";
        
        HardwareConfig cfg = hardware.getConfig();
        
        if (relay == "luz_bidon") hardware.setBottleLight(state);
        else if (relay == "motor_aire") hardware.setAirMotor(state);
        else if (relay == "valvula_llenado") hardware.setRefillValve(state);
        else if (relay == "bomba_dispensador") {
            if (state) hardware.startDispensing();
            else hardware.stopDispensing();
        }
        
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void WebManager::apiGetLogs() {
    if (server.hasArg("file")) {
        String filename = server.arg("file");
        if (filename != "log1.txt" && filename != "log2.txt" && filename != "log3.txt") {
            server.send(400, "text/plain", "Log invalido");
            return;
        }
        File f = LittleFS.open("/logs/" + filename, "r");
        if (!f) {
            server.send(404, "text/plain", "Log no encontrado o vacio");
            return;
        }
        server.streamFile(f, "text/plain");
        f.close();
    } else {
        server.send(400, "text/plain", "Falta parametro 'file'");
    }
}

void WebManager::apiTestMode() {
    if (server.hasArg("enable")) {
        bool enable = server.arg("enable") == "1";
        if (appLogic) appLogic->setTestMode(enable);
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing arg");
    }
}

void WebManager::apiTestDispense() {
    if (server.hasArg("time")) {
        unsigned long timeMs = server.arg("time").toInt();
        if (appLogic) appLogic->calibrateDispense(timeMs);
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing arg");
    }
}

void WebManager::apiTestButton() {
    if (server.hasArg("id") && server.hasArg("state")) {
        int id = server.arg("id").toInt();
        bool state = server.arg("state") == "1";
        buttons.setMode(id, state, false); // Statically turn LED on or off
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing args");
    }
}

void WebManager::apiVentasCSV() {
    if (!LittleFS.exists("/ventas.csv")) {
        server.send(404, "text/plain", "No hay ventas registradas");
        return;
    }
    File f = LittleFS.open("/ventas.csv", "r");
    server.sendHeader("Content-Disposition", "attachment; filename=\"ventas.csv\"");
    server.streamFile(f, "text/csv");
    f.close();
}

void WebManager::apiClearVentas() {
    LittleFS.remove("/ventas.csv");
    Logger.println("Archivo de ventas borrado desde web");
    server.send(200, "text/plain", "OK");
}

void WebManager::apiReboot() {
    server.send(200, "text/plain", "Reiniciando...");
    delay(500);
    ESP.restart();
}

void WebManager::apiConfigGet() {
    if (server.hasArg("file")) {
        String filename = server.arg("file");
        if (filename != "butons.json" && filename != "hardware.json" && filename != "config.json" && filename != "pago.json") {
            server.send(400, "text/plain", "Invalid file");
            return;
        }
        File f = LittleFS.open("/" + filename, "r");
        if (!f) {
            server.send(404, "application/json", "{}");
            return;
        }
        server.streamFile(f, "application/json");
        f.close();
    } else {
        server.send(400, "text/plain", "Missing arg");
    }
}

void WebManager::apiConfigSave() {
    if (server.hasArg("file") && server.hasArg("data")) {
        String filename = server.arg("file");
        if (filename != "butons.json" && filename != "hardware.json" && filename != "config.json" && filename != "pago.json") {
            server.send(400, "text/plain", "Invalid file");
            return;
        }
        File f = LittleFS.open("/" + filename, "w");
        f.print(server.arg("data"));
        f.close();
        Logger.println("Configuracion " + filename + " guardada desde Web.");
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing args");
    }
}
