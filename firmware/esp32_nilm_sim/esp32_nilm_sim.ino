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

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32 + ADS1256 CURRENT MONITOR START ===");

  if (!adc.begin()) {
    Serial.println("[FATAL] ADS1256 init failed");
    while (1) {
      delay(1000);
    }
  }

  currentSensor.begin();
  voltageSensor.begin(); // пока просто заглушка

  conn.begin();

  if (conn.connectWiFi()) {
    conn.syncTime();
  } else {
    Serial.println("[WARN] WiFi not connected at startup");
  }
}

void loop() {
  static unsigned long tPub = 0;
  unsigned long now = millis();

  conn.loop();

  if (now - tPub >= 1000) {
    tPub = now;

    float i_rms = currentSensor.readCurrentRMS(RMS_SAMPLES);
    float s_est = PowerMetrics::apparentPower(MAINS_V_RMS_EST, i_rms);

    Serial.print("[MEAS] i_rms = ");
    Serial.print(i_rms, 6);
    Serial.print(" A | s_est = ");
    Serial.print(s_est, 2);
    Serial.println(" VA");

    conn.publishCurrent(i_rms, s_est);
  }
}