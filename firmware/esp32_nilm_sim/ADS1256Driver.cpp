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

static const uint32_t ADS_SPI_HZ = 1000000;
static const uint8_t ADS_SPI_MODE = SPI_MODE1;

static inline void adsSpiBegin(SPIClass& spi) {
  spi.beginTransaction(SPISettings(ADS_SPI_HZ, MSBFIRST, ADS_SPI_MODE));
}

static inline void adsSpiEnd(SPIClass& spi) {
  spi.endTransaction();
}

ADS1256Driver::ADS1256Driver()
  : _spi(VSPI),
    _isInitialized(false),
    _drdyTimeoutStreak(0),
    _lastInitFailReason(INIT_OK) {
}


bool ADS1256Driver::isAvailable() const {
  return _isInitialized;
}

ADS1256Driver::InitFailReason ADS1256Driver::getLastInitFailReason() const {
  return _lastInitFailReason;
}

const char* ADS1256Driver::getLastInitFailReasonStr() const {
  switch (_lastInitFailReason) {
    case INIT_OK: return "ok";
    case FAIL_DRDY_BEFORE_INIT: return "DRDY timeout before init";
    case FAIL_DRDY_SELFCAL: return "DRDY timeout during SELFCAL";
    case FAIL_REGS_ALL_FF: return "invalid register readback (all 0xFF)";
    case FAIL_DRATE_MISMATCH: return "DRATE readback mismatch";
    default: return "unknown";
  }
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
  const uint32_t t0 = millis();
  while (digitalRead(PIN_ADS_DRDY) == HIGH) {
   if ((millis() - t0) > timeout_ms) {
    Serial.print("[ADS1256][diag] DRDY wait timeout, pin level=");
      Serial.println(digitalRead(PIN_ADS_DRDY));
      return false;
    }
    delay(1);
  }
  return true;
}

void ADS1256Driver::writeRegister(uint8_t reg, uint8_t value) {
  if (!_isInitialized) {
    return;
  }
  adsSpiBegin(_spi);
  chipSelect(true);
  _spi.transfer(ADS_CMD_WREG | reg);
  _spi.transfer(0x00);   
  _spi.transfer(value);
  chipSelect(false);
  adsSpiEnd(_spi);
  delayMicroseconds(10);
}

uint8_t ADS1256Driver::readRegister(uint8_t reg) {
  if (!_isInitialized) {
    return 0xFF;
  }
  adsSpiBegin(_spi);
  chipSelect(true);
  _spi.transfer(ADS_CMD_RREG | reg);
  _spi.transfer(0x00);   // read 1 register
  delayMicroseconds(10);
  uint8_t val = _spi.transfer(0xFF);
  chipSelect(false);
  adsSpiEnd(_spi);
  return val;
}

uint8_t ADS1256Driver::readRegisterWithRetry(uint8_t reg, uint8_t attempts) {
  uint8_t last = 0xFF;
  for (uint8_t i = 0; i < attempts; i++) {
    last = readRegister(reg);
    Serial.print("[ADS1256][diag] RREG 0x"); Serial.print(reg, HEX);
    Serial.print(" attempt "); Serial.print(i + 1);
    Serial.print(" => 0x"); Serial.println(last, HEX);
    delayMicroseconds(20);
  }
  return last;
}

void ADS1256Driver::setChannelAIN0AIN1() {
  if (!_isInitialized) {
    return;
  }

  // POS = AIN0, NEG = AIN1
  writeRegister(ADS_REG_MUX, 0x01);
  delayMicroseconds(10);
}

int32_t ADS1256Driver::readRaw() {
  if (!_isInitialized) {
    return 0;
  }

  if (!waitDRDY(1000)) {
    Serial.println("[ADS1256] DRDY timeout during readRaw");
    _drdyTimeoutStreak++;
    if (_drdyTimeoutStreak >= 5) {
      Serial.println("[ADS1256] too many DRDY timeouts, disabling ADC until re-init");
      _isInitialized = false;
    }
    return 0;
  }
  _drdyTimeoutStreak = 0;

  adsSpiBegin(_spi);
  chipSelect(true);
  _spi.transfer(ADS_CMD_RDATA);
  delayMicroseconds(10);

  uint8_t b0 = _spi.transfer(0xFF);
  uint8_t b1 = _spi.transfer(0xFF);
  uint8_t b2 = _spi.transfer(0xFF);

  chipSelect(false);
  adsSpiEnd(_spi);

  int32_t value = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | b2;

  if (value & 0x800000) {
    value |= 0xFF000000; // sign extension 24 -> 32
  }

  return value;
}

bool ADS1256Driver::begin() {
  _isInitialized = false;
  _drdyTimeoutStreak = 0;
  _lastInitFailReason = INIT_OK;

  Serial.println("[ADS1256] begin: configure GPIO");
  pinMode(PIN_ADS_CS, OUTPUT);
  digitalWrite(PIN_ADS_CS, HIGH);

 pinMode(PIN_ADS_DRDY, INPUT);


  if (PIN_ADS_PDWN >= 0) {
    pinMode(PIN_ADS_PDWN, OUTPUT);
    digitalWrite(PIN_ADS_PDWN, HIGH);
    delay(10);
  }

Serial.println("[ADS1256] begin: start VSPI");
Serial.print("[ADS1256][diag] pins SCK="); Serial.print(PIN_ADS_SCK);
Serial.print(" MOSI="); Serial.print(PIN_ADS_MOSI);
Serial.print(" MISO="); Serial.print(PIN_ADS_MISO);
Serial.print(" CS="); Serial.print(PIN_ADS_CS);
Serial.print(" DRDY="); Serial.print(PIN_ADS_DRDY);
Serial.print(" PDWN="); Serial.println(PIN_ADS_PDWN);
  _spi.begin(
    PIN_ADS_SCK,
    PIN_ADS_MISO,
    PIN_ADS_MOSI,
    PIN_ADS_CS
  );

Serial.println("[ADS1256] begin: wait DRDY ready");
  Serial.println(digitalRead(PIN_ADS_DRDY));
  if (!waitDRDY(500)) {
     _lastInitFailReason = FAIL_DRDY_BEFORE_INIT;
     Serial.println("[ADS1256] DRDY not low before init, continuing with ADS1256 config");
  }

  Serial.println("[ADS1256] begin: send SDATAC");
  sendCommand(ADS_CMD_SDATAC);
  delay(5);
 _isInitialized = true;
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
  Serial.println("[ADS1256] begin: SELFCAL");
  sendCommand(ADS_CMD_SELFCAL);
  if (!waitDRDY(2000)) {
    _lastInitFailReason = FAIL_DRDY_SELFCAL;
    Serial.println("[ADS1256] not responding / DRDY timeout during SELFCAL");
    _isInitialized = false;
    return false;
  }

  // set mux again after calibration
  setChannelAIN0AIN1();
  delayMicroseconds(20);

 uint8_t status = readRegisterWithRetry(ADS_REG_STATUS);
  uint8_t mux    = readRegisterWithRetry(ADS_REG_MUX);
  uint8_t adcon  = readRegisterWithRetry(ADS_REG_ADCON);
  uint8_t drate  = readRegisterWithRetry(ADS_REG_DRATE);

  Serial.println("[ADS1256] init done");
  Serial.print("[ADS1256] STATUS=0x"); Serial.println(status, HEX);
  Serial.print("[ADS1256] MUX   =0x"); Serial.println(mux, HEX);
  Serial.print("[ADS1256] ADCON =0x"); Serial.println(adcon, HEX);
  Serial.print("[ADS1256] DRATE =0x"); Serial.println(drate, HEX);

  // Floating SPI or missing device often reads 0xFF for every register.
  // Treat this as "ADC unavailable" so upper layers fail gracefully.
  if (status == 0xFF && mux == 0xFF && adcon == 0xFF && drate == 0xFF) {
    _lastInitFailReason = FAIL_REGS_ALL_FF;
    Serial.println("[ADS1256] invalid register readback (all 0xFF), ADC not detected");
    _isInitialized = false;
    return false;
  }

  if (drate != 0xA1) {
    _lastInitFailReason = FAIL_DRATE_MISMATCH;
    Serial.println("[ADS1256] DRATE readback mismatch, ADC init failed");
    _isInitialized = false;
    return false;
  }

  _lastInitFailReason = INIT_OK;
  return true;
}