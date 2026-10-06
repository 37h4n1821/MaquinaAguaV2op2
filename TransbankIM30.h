#ifndef TRANSBANKIM30_H
#define TRANSBANKIM30_H

#include <Arduino.h>
#include <HardwareSerial.h>

#define STX 0x02
#define ETX 0x03
#define ACK 0x06
#define NAK 0x15

class TransbankIM30 {
public:
  TransbankIM30(HardwareSerial& serial);
  void begin();
  bool checkCommunication();
  bool initializePOS();
  int startSale(long amount, String ticket);

private:
  HardwareSerial& _serial;
  void sendCommand(String data);
  byte calculateLRC(String data);
  bool waitForACK(unsigned long timeout = 2000);
  String waitForResponse(unsigned long timeout = 60000);
};

#endif
