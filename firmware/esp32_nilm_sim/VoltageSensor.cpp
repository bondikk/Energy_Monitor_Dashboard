#include "VoltageSensor.h"

VoltageSensor::VoltageSensor(ADS1256Driver& adc)
  : _adc(adc) {
}

bool VoltageSensor::begin() {
  return true;
}

float VoltageSensor::readVoltageRMS(uint16_t nSamples) {
  (void)nSamples;
  return 0.0f;
}