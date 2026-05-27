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
VoltageSensor voltageSensor(adc);   // пока не используется
Connectivity conn;

static bool gAdsReady = false;

static void initAdsSafely() {
  Serial.println("[ADS1256] begin safe initialization");
  gAdsReady = adc.begin();
  if (!gAdsReady) {
    Serial.println("[ADS1256] not responding / DRDY timeout (running without ADC)");
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

  if (now - tPub >= 1000) {
    tPub = now;

    float i_rms = 0.0f;
    if (gAdsReady) {
      i_rms = currentSensor.readCurrentRMS(RMS_SAMPLES);
    } else {
      Serial.println("[ADS1256] skipped sampling: ADC unavailable");
    }

    float v_rms = voltageSensor.readVoltageRMS(RMS_SAMPLES);
    if (v_rms <= 1.0f) {
      v_rms = MAINS_V_RMS_EST;   // временный fallback, пока VoltageSensor = заглушка
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