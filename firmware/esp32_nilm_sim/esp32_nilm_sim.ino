#include <Arduino.h>
#include "secrets.h"
#include "config.h"
#include "ADS1256Driver.h"
#include "CurrentSensor.h"
#include "VoltageSensor.h"
#include "PowerMetrics.h"
#include "Connectivity.h"

ADS1256Driver adc;
CurrentSensor currentSensor(adc);
VoltageSensor voltageSensor(adc);
Connectivity conn;

static bool gAdsReady = false;
static unsigned long gLastAdsInitAttemptMs = 0;

static const unsigned long ADS_REINIT_INTERVAL_MS = 15000;
static const unsigned long ADS_UNAVAILABLE_LOG_INTERVAL_MS = 10000;

static void initAdsSafely() {
  gLastAdsInitAttemptMs = millis();
  Serial.println("[ADS1256] begin safe initialization");
  Serial.print("[ADS1256][diag] fail reason before begin: ");
  Serial.print(adc.getLastInitFailReasonStr());
  Serial.print(" (code=");
  Serial.print(static_cast<uint8_t>(adc.getLastInitFailReason()));
  Serial.println(")");
  
  gAdsReady = adc.begin();

  Serial.print("[ADS1256][diag] fail reason after begin: ");
  Serial.print(adc.getLastInitFailReasonStr());
  Serial.print(" (code=");
  Serial.print(static_cast<uint8_t>(adc.getLastInitFailReason()));
  Serial.println(")");

  if (!gAdsReady) {
    Serial.print("[ADS1256] init failed: ");
    Serial.println(adc.getLastInitFailReasonStr());
    Serial.println("[ADS1256] running without ADC");
    return;
  }
  currentSensor.begin();
  voltageSensor.begin();
  Serial.println("[ADS1256] initialization successful");
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32 + ADS1256 CURRENT MONITOR START ===");

  conn.begin();

  if (!ADS_INIT_AFTER_NETWORK) {
    Serial.println("[ADS1256] init-before-network mode");
    initAdsSafely();
  }

  if (conn.connectWiFi()) {
    conn.syncTime();
  } else {
    Serial.println("[WARN] WiFi not connected at startup");
  }

  if (ADS_INIT_AFTER_NETWORK) {
    Serial.println("[ADS1256] safe mode: init after network startup");
    initAdsSafely();
  }
}

void loop() {
  static unsigned long tPub = 0;
  const unsigned long now = millis();

  conn.loop();

  if (!gAdsReady && (now - gLastAdsInitAttemptMs >= ADS_REINIT_INTERVAL_MS)) {
    Serial.println("[ADS1256] periodic re-init attempt...");
    initAdsSafely();
  }

  if (now - tPub >= 1000) {
    tPub = now;

    float i_rms = 0.0f;
    if (gAdsReady) {
      i_rms = currentSensor.readCurrentRMS(RMS_SAMPLES);
    } else {
      static unsigned long tAdsUnavailableLog = 0;
      if (now - tAdsUnavailableLog >= ADS_UNAVAILABLE_LOG_INTERVAL_MS) {
        tAdsUnavailableLog = now;
        Serial.print("[ADS1256] skipped sampling: ADC unavailable (reason: ");
        Serial.print(adc.getLastInitFailReasonStr());
        Serial.println(")");
      }
    }
    float v_rms = voltageSensor.readVoltageRMS(RMS_SAMPLES);
    if (v_rms <= 1.0f) {
      v_rms = MAINS_V_RMS_EST;
    }

    const float s_est = PowerMetrics::apparentPower(v_rms, i_rms);

    Serial.print("[MEAS] i_rms = ");
    Serial.print(i_rms, 6);
    Serial.print(" A | v_rms = ");
    Serial.print(v_rms, 2);
    Serial.print(" V | power = ");
    Serial.print(s_est, 2);
    Serial.println(" VA");

    conn.publishTelemetry(i_rms, v_rms, s_est);
  }
}
