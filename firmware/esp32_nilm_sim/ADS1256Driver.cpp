#include "ADS1256Driver.h"
#include "config.h"

// =========================
// ADS1256 commands
// =========================
#define ADS_CMD_WAKEUP   0x00
#define ADS_CMD_RDATA    0x01
#define ADS_CMD_RDATAC   0x03
#define ADS_CMD_SDATAC   0x0F
#define ADS_CMD_RREG     0x10
#define ADS_CMD_WREG     0x50
#define ADS_CMD_SELFCAL  0xF0
#define ADS_CMD_SYNC     0xFC
#define ADS_CMD_STANDBY  0xFD
#define ADS_CMD_RESET    0xFE

#define ADS_REG_STATUS   0x00
#define ADS_REG_MUX      0x01
#define ADS_REG_ADCON    0x02
#define ADS_REG_DRATE    0x03

ADS1256Driver::ADS1256Driver()
  : _spi(VSPI) {
}

void ADS1256Driver::chipSelect(bool en) {
  digitalWrite(PIN_ADS_CS, en ? LOW : HIGH);
}

void ADS1256Driver::sendCommand(uint8_t cmd) {
  chipSelect(true);
  _spi.transfer(cmd);
  chipSelect(false);
  delayMicroseconds(5);
}

bool ADS1256Driver::waitDRDY(uint32_t timeout_ms) {
  uint32_t t0 = millis();
  while (digitalRead(PIN_ADS_DRDY) == HIGH) {
    if ((millis() - t0) > timeout_ms) return false;
  }
  return true;
}

void ADS1256Driver::writeRegister(uint8_t reg, uint8_t value) {
  chipSelect(true);
  _spi.transfer(ADS_CMD_WREG | reg);
  _spi.transfer(0x00);   // write 1 register
  _spi.transfer(value);
  chipSelect(false);
  delayMicroseconds(10);
}

uint8_t ADS1256Driver::readRegister(uint8_t reg) {
  chipSelect(true);
  _spi.transfer(ADS_CMD_RREG | reg);
  _spi.transfer(0x00);   // read 1 register
  delayMicroseconds(10);
  uint8_t val = _spi.transfer(0xFF);
  chipSelect(false);
  return val;
}

void ADS1256Driver::setChannelAIN0AIN1() {
  // POS = AIN0, NEG = AIN1
  writeRegister(ADS_REG_MUX, 0x01);
  delayMicroseconds(10);
}

int32_t ADS1256Driver::readRaw() {
  if (!waitDRDY()) {
    Serial.println("[ADS] DRDY timeout");
    return 0;
  }

  chipSelect(true);
  _spi.transfer(ADS_CMD_RDATA);
  delayMicroseconds(10);

  uint8_t b0 = _spi.transfer(0xFF);
  uint8_t b1 = _spi.transfer(0xFF);
  uint8_t b2 = _spi.transfer(0xFF);

  chipSelect(false);

  int32_t value = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | b2;

  if (value & 0x800000) {
    value |= 0xFF000000; // sign extension 24 -> 32
  }

  return value;
}

bool ADS1256Driver::begin() {
  pinMode(PIN_ADS_CS, OUTPUT);
  digitalWrite(PIN_ADS_CS, HIGH);

  pinMode(PIN_ADS_DRDY, INPUT);

  if (PIN_ADS_PDWN >= 0) {
    pinMode(PIN_ADS_PDWN, OUTPUT);
    digitalWrite(PIN_ADS_PDWN, HIGH);
    delay(10);
  }

  _spi.begin(
    PIN_ADS_SCK,
    PIN_ADS_MISO,
    PIN_ADS_MOSI,
    PIN_ADS_CS
  );

  sendCommand(ADS_CMD_SDATAC);
  delay(5);

  // STATUS: MSB first, ACAL off, buffer off
  writeRegister(ADS_REG_STATUS, 0x00);

  // MUX: AIN0 - AIN1
  setChannelAIN0AIN1();

  // ADCON: gain = 1
  writeRegister(ADS_REG_ADCON, 0x00);

  // DRATE: ~1000 SPS
  writeRegister(ADS_REG_DRATE, 0xA1);

  delay(5);

  // self calibration
  sendCommand(ADS_CMD_SELFCAL);
  if (!waitDRDY(2000)) {
    Serial.println("[ADS] SELFCAL timeout");
    return false;
  }

  // set mux again after calibration
  setChannelAIN0AIN1();
  delayMicroseconds(20);

  uint8_t status = readRegister(ADS_REG_STATUS);
  uint8_t mux    = readRegister(ADS_REG_MUX);
  uint8_t adcon  = readRegister(ADS_REG_ADCON);
  uint8_t drate  = readRegister(ADS_REG_DRATE);

  Serial.println("[ADS] Init done");
  Serial.print("[ADS] STATUS=0x"); Serial.println(status, HEX);
  Serial.print("[ADS] MUX   =0x"); Serial.println(mux, HEX);
  Serial.print("[ADS] ADCON =0x"); Serial.println(adcon, HEX);
  Serial.print("[ADS] DRATE =0x"); Serial.println(drate, HEX);

  return true;
}