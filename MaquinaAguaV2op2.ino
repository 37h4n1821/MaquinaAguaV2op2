#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "TransbankIM30.h"
#include "DisplayManager.h"
#include "LEDManager.h"
#include "ButtonManager.h"
#include "HardwareManager.h"
#include "LoggerManager.h"
#include "WebManager.h"
#include "AppLogic.h"

// Pines RS232 para Serial2
#define RXD2 16
#define TXD2 17

// Configuración Aro Neopixel (WS2812)
#define LED_PIN    4
#define LED_COUNT  24 
Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
LEDManager leds(ring, LED_COUNT);

// Configuración Pantalla LCD I2C 20x4
LiquidCrystal_I2C lcd(0x27, 20, 4);
DisplayManager displayMsg(lcd);

// Configuración Expansor I2C MCP23017
Adafruit_MCP23X17 mcp;
ButtonManager buttons(mcp);
HardwareManager hardware(mcp);
HardwareConfig hwConfig;

// Instancia de POS
TransbankIM30 pos(Serial2);

RTC_DATA_ATTR int transbankFailCount = 0;

// Variables globales para la configuración
String systemVersion = "";
String systemName = "";
String hostname = "";
WebManager web;

ButtonConfig buttonConfigs[10];
int numButtons = 0;

AppLogic* appLogic = nullptr;

uint8_t getMcpPin(String pinStr) {
  if (pinStr.length() < 2) return 0;
  char port = pinStr.charAt(0);
  int num = pinStr.charAt(1) - '0';
  if (port == 'A' || port == 'a') return num;
  if (port == 'B' || port == 'b') return 8 + num;
  return 0;
}

void haltWithError(int errorCode) {
  Logger.print(">>> ERROR CRÍTICO DE ARRANQUE <<< CODIGO: ");
  Logger.println(String(errorCode));
  displayMsg.showError(errorCode);
  leds.showError();
  while (true) {
    delay(100);
  }
}

void bootProcess() {
  int totalSteps = 7;
  int currentStep = 1;

  // Paso 1: Iniciar LEDs
  leds.setProgress(currentStep++, totalSteps, ring.Color(0, 0, 255));
  delay(400);

  // Paso 2: Iniciar Bus I2C y Pantalla LCD
  Logger.println("1. Iniciando LCD 20x4...");
  Wire.begin();
  displayMsg.init();
  displayMsg.showBooting(currentStep, totalSteps, "Iniciando LCD...");
  leds.setProgress(currentStep++, totalSteps, ring.Color(0, 0, 255));
  delay(600);

  // Paso 3: Iniciar Expansor MCP23017
  Logger.println("2. Iniciando MCP23017...");
  displayMsg.showBooting(currentStep, totalSteps, "Iniciando I/O...");
  if (!mcp.begin_I2C()) { 
    haltWithError(101);
  }
  leds.setProgress(currentStep++, totalSteps, ring.Color(0, 0, 255));
  delay(500);

  // Paso 4: Iniciar Sistema de Archivos y Leer config.json
  Logger.println("3. Montando LittleFS y leyendo config.json...");
  displayMsg.showBooting(currentStep, totalSteps, "Cargando config...");
  if (!LittleFS.begin(true)) {
    haltWithError(102);
  }
  
  // Ahora que LittleFS esta montado, iniciamos el Logger
  Logger.init();

  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    haltWithError(103);
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    configFile.close();
    haltWithError(104);
  }

  systemVersion = doc["version"].as<String>();
  systemName = doc["system_name"].as<String>();
  hostname = doc["hostname"].as<String>();
  
  String staSSID = doc["wifi"]["sta"]["ssid"].as<String>();
  String staPass = doc["wifi"]["sta"]["password"].as<String>();
  String apSSID = doc["wifi"]["ap"]["ssid"].as<String>();
  String apPass = doc["wifi"]["ap"]["password"].as<String>();
  String webUser = doc["web"]["user"].as<String>();
  String webPass = doc["web"]["password"].as<String>();
  
  configFile.close();
  
  // Iniciar servidor web y WiFi
  displayMsg.showBooting(currentStep, totalSteps, "Conectando WiFi...");
  web.init(hostname, staSSID, staPass, apSSID, apPass, webUser, webPass);

  // Leer hardware.json
  File hwFile = LittleFS.open("/hardware.json", "r");
  if (!hwFile) {
    haltWithError(107);
  }
  StaticJsonDocument<1024> hwDoc;
  DeserializationError hwErr = deserializeJson(hwDoc, hwFile);
  if (hwErr) {
    hwFile.close();
    haltWithError(108);
  }

  hwConfig.pinNivelInf = getMcpPin(hwDoc["sensores"]["nivel_inferior"].as<String>());
  hwConfig.pinNivelSup = getMcpPin(hwDoc["sensores"]["nivel_superior"].as<String>());
  hwConfig.pinAguaEntrada = getMcpPin(hwDoc["sensores"]["agua_entrada"].as<String>());
  hwConfig.pinAguaOsmosis = getMcpPin(hwDoc["sensores"]["agua_osmosis"].as<String>());
  hwConfig.pinAguaDisp = getMcpPin(hwDoc["sensores"]["agua_dispensador"].as<String>());
  hwConfig.flotadorNC = hwDoc["sensores"]["flotador_normalmente_cerrado"].as<bool>();

  hwConfig.pinLuzBidon = getMcpPin(hwDoc["reles"]["luz_bidon"].as<String>());
  hwConfig.pinMotorAire = getMcpPin(hwDoc["reles"]["motor_aire"].as<String>());
  hwConfig.pinValvulaLlenado = getMcpPin(hwDoc["reles"]["valvula_llenado"].as<String>());
  hwConfig.pinBombaDisp = getMcpPin(hwDoc["reles"]["bomba_dispensador"].as<String>());
  hwConfig.releActivoLow = hwDoc["reles"]["activacion_en_low"].as<bool>();

  hwConfig.reqRedAgua = hwDoc["config"]["requiere_red_agua"].as<bool>();
  hwConfig.intervaloOxigenacionMs = hwDoc["config"]["intervalo_oxigenacion_min"].as<unsigned long>() * 60000;
  hwConfig.duracionOxigenacionMs = hwDoc["config"]["duracion_oxigenacion_min"].as<unsigned long>() * 60000;
  
  hwFile.close();

  // Iniciar el HardwareManager con la configuracion
  hardware.init(hwConfig);
  
  leds.setProgress(currentStep++, totalSteps, ring.Color(0, 0, 255));
  delay(500);

  // Paso 5: Iniciar Botones y leer butons.json
  Logger.println("4. Leyendo butons.json y configurando botones...");
  displayMsg.showBooting(currentStep, totalSteps, "Configurando botones");
  File btnFile = LittleFS.open("/butons.json", "r");
  if (!btnFile) {
    haltWithError(105);
  }

  StaticJsonDocument<1024> btnDoc;
  DeserializationError btnError = deserializeJson(btnDoc, btnFile);
  if (btnError) {
    btnFile.close();
    haltWithError(106);
  }

  JsonArray botonesArr = btnDoc["botones"].as<JsonArray>();
  numButtons = 0;
  for (JsonObject btnObj : botonesArr) {
    if (numButtons >= 10) break;
    buttonConfigs[numButtons].id = btnObj["id"].as<int>();
    buttonConfigs[numButtons].pin = getMcpPin(btnObj["pin"].as<String>());
    buttonConfigs[numButtons].tipo = btnObj["tipo"].as<String>();
    
    if (buttonConfigs[numButtons].tipo == "venta") {
      buttonConfigs[numButtons].monto = btnObj["monto"].as<long>();
      buttonConfigs[numButtons].tiempo_ms = btnObj["tiempo_ms"].as<long>();
      buttonConfigs[numButtons].texto = btnObj["texto"].as<String>();
    } else {
      buttonConfigs[numButtons].accion = btnObj["accion"].as<String>();
    }
    numButtons++;
  }
  btnFile.close();

  // Inicializar botones con la nueva librería
  buttons.init(buttonConfigs, numButtons);

  // Animación breve de inicialización
  for (int i = 0; i < numButtons; i++) {
    mcp.pinMode(buttonConfigs[i].pin, OUTPUT);
    mcp.digitalWrite(buttonConfigs[i].pin, HIGH);
    delay(300);
  }
  delay(2000);
  buttons.disableAll(); // Apagar todos

  leds.setProgress(currentStep++, totalSteps, ring.Color(0, 0, 255));
  delay(500);

  // Paso 6: Iniciar RS232 y verificar conexión POS IM30
  Logger.println("5. Iniciando y validando POS IM30 (RS232)...");
  displayMsg.showBooting(currentStep, totalSteps, "Conectando Transbank");
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  pos.begin();
  delay(1000); 

  if (!pos.checkCommunication()) {
    transbankFailCount++;
    Logger.println("ERROR: Fallo comunicacion POS IM30. Intento " + String(transbankFailCount) + "/5");
    if (transbankFailCount >= 5) {
      haltWithError(201);
    } else {
      Logger.println("Reiniciando ESP32 en 15 segundos para dar tiempo al POS de iniciar...");
      displayMsg.showBooting(currentStep, totalSteps, "Reintentando POS...");
      delay(15000);
      ESP.restart();
    }
  } else {
    transbankFailCount = 0;
  }
  
  leds.setProgress(currentStep++, totalSteps, ring.Color(0, 0, 255));
  delay(500);

  // Paso 7: Fin del boteo
  leds.showSuccess();
  delay(1500);
  leds.clear();

  // Crear AppLogic e iniciar estado
  appLogic = new AppLogic(displayMsg, leds, buttons, hardware, pos, systemName, buttonConfigs, numButtons);
  appLogic->setState(STATE_IDLE);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  leds.init();

  Serial.println("\n==================================");
  Serial.println("   MÁQUINA DE AGUA V2 INICIANDO");
  Serial.println("==================================");
  
  bootProcess();
  
  Logger.println("==================================");
  Logger.println("   SISTEMA LISTO PARA VENDER");
  Logger.println("==================================");
}

void loop() {
  Logger.processSerialCommands();
  web.update();
  if (appLogic) {
    appLogic->update();
  }
}
