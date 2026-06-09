#include "Connectivity.h"
#include <ArduinoJson.h>
#include "time.h"
#include "secrets.h"
#include "config.h"

Connectivity::Connectivity()
  : _mqtt(_espClient),
    _lastMqttAttempt(0),
    _lastWiFiRetry(0) {
}

void Connectivity::begin() {
  setupMQTT();
}

void Connectivity::setupMQTT() {
  _mqtt.setServer(MQTT_HOST, MQTT_PORT);
  _mqtt.setKeepAlive(30);
  _mqtt.setSocketTimeout(10);
}

bool Connectivity::connectWiFi(unsigned long timeout_ms) {
  Serial.println("[BOOT] WiFi connect...");
  Serial.print("[WIFI] SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect(false, false);
    delay(200);
  }

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < timeout_ms) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[OK] WiFi connected");
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.print("[ERR] WiFi failed, status = ");
  Serial.println(WiFi.status());
  return false;
}

void Connectivity::reconnectMQTT_nonBlocking() {
  unsigned long now = millis();

  if (_mqtt.connected()) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (now - _lastMqttAttempt < 3000) return;

  _lastMqttAttempt = now;

  Serial.print("[MQTT] Connect to ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.print(MQTT_PORT);
  Serial.print(" ... ");

  if (_mqtt.connect(DEVICE_ID)) {
    Serial.println("OK");
  } else {
    Serial.print("FAIL rc=");
    Serial.println(_mqtt.state());
  }
}

void Connectivity::loop() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if (now - _lastWiFiRetry >= 10000) {
      _lastWiFiRetry = now;
      Serial.println("[WIFI] Reconnect...");
      if (WiFi.getMode() != WIFI_STA) {
        WiFi.mode(WIFI_STA);
      }
      wl_status_t st = WiFi.status();
      if (st == WL_NO_SHIELD || st == WL_STOPPED || st == WL_IDLE_STATUS) {
        WiFi.begin(WIFI_SSID, WIFI_PASS);
      } else {
        WiFi.reconnect();
      }
    }
    return;
  }

  reconnectMQTT_nonBlocking();
  _mqtt.loop();
}

void Connectivity::syncTime() {
  Serial.println("[BOOT] NTP sync...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  struct tm ti;
  int tries = 0;
  while (!getLocalTime(&ti) && tries < 10) {
    delay(300);
    Serial.print(".");
    tries++;
  }
  Serial.println();

  if (getLocalTime(&ti)) {
    Serial.println("[OK] NTP synced");
  } else {
    Serial.println("[WARN] NTP sync failed");
  }
}

bool Connectivity::isWiFiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool Connectivity::isMQTTConnected() {
  return _mqtt.connected();
}

String Connectivity::iso8601() {
  struct tm ti;
  if (!getLocalTime(&ti)) return "";

  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &ti);
  return String(buf);
}

bool Connectivity::publishTelemetry(float iRms, float vRms, float power) {
  if (!isWiFiConnected()) {
    Serial.println("[PUB] skip: WiFi not connected");
    return false;
  }

  if (!isMQTTConnected()) {
    Serial.println("[PUB] skip: MQTT not connected");
    return false;
  }

  StaticJsonDocument<256> doc;

  doc["device_id"] = DEVICE_ID;

  String ts = iso8601();
  if (ts.length() > 0) {
    doc["timestamp"] = ts;
  }

  doc["i_rms"] = roundf(iRms * 1000.0f) / 1000.0f;
  doc["v_rms"] = roundf(vRms * 100.0f) / 100.0f;

  doc["s_est_va"] = roundf(power * 10.0f) / 10.0f;

  doc["sample_rate"] = ADS_SPS;
doc["source"] = "esp32_ads1256";

  char payload[256];
  serializeJson(doc, payload, sizeof(payload));

  Serial.print("[PUB] topic = ");
  Serial.println(MQTT_TOPIC);
  Serial.print("[PUB] payload = ");
  Serial.println(payload);

  bool ok = _mqtt.publish(MQTT_TOPIC, payload);
  if (!ok) {
    Serial.println("[MQTT] publish failed");
  } else {
    Serial.println("[MQTT] publish OK");
  }

  return ok;
}