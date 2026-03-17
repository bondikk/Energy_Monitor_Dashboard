#pragma once
#include <Arduino.h>

// =========================
// ADS1256 pins
// =========================
static const int PIN_ADS_SCK  = 18;
static const int PIN_ADS_MOSI = 23;
static const int PIN_ADS_MISO = 19;
static const int PIN_ADS_CS   = 5;
static const int PIN_ADS_DRDY = 34;
static const int PIN_ADS_PDWN = -1;

// =========================
// Measurement settings
// =========================
static const float VREF        = 2.5f;      // module reference
static const float PGA_GAIN    = 1.0f;      // gain = 1
static const float BURDEN_OHMS = 10.0f;     // external burden resistor
static const float CT_RATIO    = 2000.0f;   // SCT-013-000 => 100A : 50mA
static const float CURRENT_CAL = 1.0f;      // calibration factor

static const uint16_t RMS_SAMPLES = 200;
static const uint16_t ADS_SPS     = 1000;

// For apparent power estimate
static const float MAINS_V_RMS_EST = 230.0f;