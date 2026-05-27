#pragma once
#include <Arduino.h>

// =========================
static const int PIN_ADS_SCK  = 18;
static const int PIN_ADS_MOSI = 23;
static const int PIN_ADS_MISO = 19;
static const int PIN_ADS_CS   = 25;
static const int PIN_ADS_DRDY = 26;
static const int PIN_ADS_PDWN = -1;

static const bool ADS_INIT_AFTER_NETWORK = true;
// =========================
// Measurement settings
// =========================
static const float VREF        = 2.5f;
static const float PGA_GAIN    = 1.0f;
static const float BURDEN_OHMS = 10.0f;
static const float CT_RATIO    = 2000.0f;
static const float CURRENT_CAL = 1.0f;

static const uint16_t RMS_SAMPLES = 200;
static const uint16_t ADS_SPS     = 1000;

// Voltage channel is currently a stub. This value is used only
// to estimate apparent power (S = V_nominal * I_rms).
static const float MAINS_V_RMS_EST = 230.0f;