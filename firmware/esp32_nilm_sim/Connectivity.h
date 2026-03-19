#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

class Connectivity {
public:
  Connectivity();

  void begin();
  bool connectWiFi(unsigned long timeout_ms = 15000);
  void loop();
  void syncTime();

  bool isWiFiConnected() const;
  bool isMQTTConnected() ;

  String iso8601();
  bool publishTelemetry(float iRms, float vRms, float sEstVA);

private:
  WiFiClient _espClient;
  PubSubClient _mqtt;

  unsigned long _lastMqttAttempt;
  unsigned long _lastWiFiRetry;

  void setupMQTT();
  void reconnectMQTT_nonBlocking();
};