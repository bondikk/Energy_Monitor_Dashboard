#include "CurrentSensor.h"
#include <math.h>
#include "config.h"

CurrentSensor::CurrentSensor(ADS1256Driver& adc)
  : _adc(adc) {
}

bool CurrentSensor::begin() {
  _adc.setChannelAIN0AIN1();
  return true;
}

float CurrentSensor::rawToVolts(int32_t raw) {
  const float fullScale = (2.0f * VREF) / PGA_GAIN;
  return ((float)raw / 8388607.0f) * fullScale;
}

float CurrentSensor::voltsToPrimaryCurrent(float vDiff) {
  return (vDiff / BURDEN_OHMS) * CT_RATIO * CURRENT_CAL;
}

float CurrentSensor::readCurrentRMS(uint16_t nSamples) {
  double sumSq = 0.0;

  _adc.setChannelAIN0AIN1();

  for (uint16_t i = 0; i < nSamples; i++) {
    int32_t raw = _adc.readRaw();
    float vdiff = rawToVolts(raw);
    float ipri  = voltsToPrimaryCurrent(vdiff);
    sumSq += (double)ipri * (double)ipri;
  }

  return sqrt(sumSq / (double)nSamples);
}