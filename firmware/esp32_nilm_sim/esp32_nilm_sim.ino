#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "time.h"
#include "secrets.h"

WiFiClient esp;
PubSubClient mqtt(esp);

// --- утилита для ISO времени (UTC) ---
String iso8601() {
  struct tm ti;
  if (!getLocalTime(&ti)) return "1970-01-01T00:00:00Z";
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &ti);
  return String(buf);
}

// --- Wi-Fi с логами и таймаутом ---
bool connectWiFi(unsigned long timeout_ms = 20000) {
  Serial.println("[BOOT] WiFi connect...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < timeout_ms) {
    Serial.print(".");
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n[ OK ] WiFi connected, IP = ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\n[ERR] WiFi timeout");
    return false;
  }
}

// --- MQTT с логами причин отказа ---
void connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  while (!mqtt.connected()) {
    Serial.print("[MQTT] Connect to ");
    Serial.print(MQTT_HOST); Serial.print(":"); Serial.print(MQTT_PORT);
    Serial.print(" ... ");
    if (mqtt.connect(DEVICE_ID)) {
      Serial.println("OK");
    } else {
      Serial.print("FAIL rc="); Serial.println(mqtt.state());
      // rc подсказки: -2=нет соединения/хост недоступен; 4/5=авторизация
      delay(1500);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== ESP32 NILM SIM START ===");

  if (!connectWiFi()) {
    // если Wi-Fi не поднялся — через 5с попробуем ещё раз
    delay(5000);
    (void)connectWiFi();
  }

  Serial.println("[BOOT] NTP sync...");
  configTime(0, 0, "pool.ntp.org");
  // дадим времени подтянуться:
  struct tm ti; int tries=0;
  while (!getLocalTime(&ti) && tries < 10) { delay(300); Serial.print("."); tries++; }
  Serial.println();

  connectMQTT();
}

void loop() {
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  static unsigned long t0 = 0;
  const unsigned long now = millis();
  if (now - t0 >= 1000) { // публикуем раз в секунду для наглядности
    t0 = now;

    // --- простая симуляция ---
    float v = 230.0f + 0.6f * sinf(now / 3000.0f);
    float i = 0.25f + 0.05f * sinf(now / 7000.0f);
    if ((now / 1000) % 30 < 5) i += 1.2f; // "чайник"
    float p = v * i;

    JsonDocument doc;
    doc["device"]      = DEVICE_ID;
    doc["timestamp"]   = iso8601();
    doc["v_rms"]       = roundf(v * 10) / 10.0f;
    doc["i_rms"]       = roundf(i * 100) / 100.0f;
    doc["p_active"]    = roundf(p * 10) / 10.0f;
    doc["sample_rate"] = 15000;

    char payload[256];
    serializeJson(doc, payload);
    Serial.print("[PUB] "); Serial.println(payload);
    mqtt.publish(MQTT_TOPIC, payload);
  }
}