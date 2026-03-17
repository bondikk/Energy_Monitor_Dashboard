#pragma once
#include <Arduino.h>
#include <SPI.h>

class ADS1256Driver {
public:
  ADS1256Driver();

  bool begin();

  void setChannelAIN0AIN1();
  int32_t readRaw();

  uint8_t readRegister(uint8_t reg);
  void writeRegister(uint8_t reg, uint8_t value);

private:
  SPIClass _spi;

  void chipSelect(bool en);
  void sendCommand(uint8_t cmd);
  bool waitDRDY(uint32_t timeout_ms = 1000);
};