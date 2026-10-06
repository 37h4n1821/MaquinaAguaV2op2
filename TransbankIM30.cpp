#include "TransbankIM30.h"

TransbankIM30::TransbankIM30(HardwareSerial& serial) : _serial(serial) {}

void TransbankIM30::begin() {
  // Inicialización de Serial se debe hacer en el sketch con los pines adecuados
  // Ej en ESP32: Serial2.begin(19200, SERIAL_8N1, 16, 17);
}

byte TransbankIM30::calculateLRC(String data) {
  byte lrc = 0;
  for (int i = 0; i < data.length(); i++) {
    lrc ^= data[i];
  }
  lrc ^= ETX;
  return lrc;
}

void TransbankIM30::sendCommand(String data) {
  // Limpiar buffer de lectura antes de enviar
  while(_serial.available()) {
    _serial.read();
  }
  
  byte lrc = calculateLRC(data);
  _serial.write(STX);
  _serial.print(data);
  _serial.write(ETX);
  _serial.write(lrc);
}

bool TransbankIM30::waitForACK(unsigned long timeout) {
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (_serial.available()) {
      byte b = _serial.read();
      if (b == ACK) return true;
      if (b == NAK) return false;
    }
  }
  return false;
}

String TransbankIM30::waitForResponse(unsigned long timeout) {
  unsigned long start = millis();
  String response = "";
  bool reading = false;
  
  while (millis() - start < timeout) {
    if (_serial.available()) {
      byte b = _serial.read();
      if (b == STX) {
        reading = true;
        response = "";
      } else if (b == ETX) {
        reading = false;
        // Leer y consumir el LRC (1 byte)
        unsigned long lrcStart = millis();
        while(!_serial.available() && millis() - lrcStart < 100);
        if (_serial.available()) {
           _serial.read(); 
        }
        return response;
      } else if (reading) {
        response += (char)b;
      } else if (b == ACK) {
        // En algunos flujos el POS manda ACK previo a la respuesta. 
        // Lo ignoramos aquí y seguimos esperando la trama STX...ETX
      }
    }
  }
  return ""; // Timeout
}

bool TransbankIM30::checkCommunication() {
  // Comando Polling: 0100
  sendCommand("0100");
  return waitForACK(2000);
}

bool TransbankIM30::initializePOS() {
  // Comando inicialización: 0070
  sendCommand("0070");
  if (!waitForACK(2000)) return false;
  return true;
}

int TransbankIM30::startSale(long amount, String ticket) {
  // Comando Venta: 0200|Monto|Ticket|Impresion|Mensajes
  // Impresión: 0 (Sin voucher formateado a la caja)
  // Mensajes: 0 (Sin mensajes intermedios, simplifica el flujo)
  String cmd = "0200|";
  cmd += String(amount) + "|";
  cmd += ticket + "|0|0";
  
  sendCommand(cmd);
  if (!waitForACK(2000)) {
    Serial.println("No se recibió ACK de la venta del POS.");
    return 2;
  }
  
  // Esperar respuesta de venta 0210
  Serial.println("POS listo. Esperando pago del cliente...");
  
  // Damos un timeout amplio (ej: 2 minutos) para que el cliente acerque la tarjeta y ponga su PIN
  String resp = waitForResponse(120000); 
  
  if (resp.length() > 0) {
    _serial.write(ACK); // Enviar ACK confirmando recepción al POS
    
    // Analizar la respuesta. Ejemplo respuesta OK: 0210|00|...
    if (resp.startsWith("0210|00")) {
      return 0; // Transacción Aprobada
    } else if (resp.startsWith("0210|")) {
      Serial.print("Transacción cancelada o rechazada por usuario. Trama: ");
      Serial.println(resp);
      return 1;
    } else {
      Serial.print("Error de trama. Trama: ");
      Serial.println(resp);
      return 2;
    }
  }
  
  Serial.println("Timeout esperando que el cliente pague.");
  return 2;
}
