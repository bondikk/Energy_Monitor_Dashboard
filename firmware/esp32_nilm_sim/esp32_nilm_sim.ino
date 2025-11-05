#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "time.h"
#include "secrets.h"

#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org"
#endif

#ifndef NTP_GMT_OFFSET
#define NTP_GMT_OFFSET 0
#endif

#ifndef NTP_DAYLIGHT_OFFSET
#define NTP_DAYLIGHT_OFFSET 0
#endif

#ifndef NTP_RESYNC_INTERVAL_MS
#define NTP_RESYNC_INTERVAL_MS (6UL * 60UL * 60UL * 1000UL)
#endif

#ifndef NTP_RETRY_INTERVAL_MS
#define NTP_RETRY_INTERVAL_MS (30UL * 1000UL)
#endif


WiFiClient esp;
PubSubClient mqtt(esp);

static unsigned long lastNtpAttemptMs = 0;
static bool ntpSynced = false;

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
    ntpSynced = false;
    lastNtpAttemptMs = 0;
    return true;
  } else {
    Serial.println("\n[ERR] WiFi timeout");
    return false;
  }
}

// --- синхронизация времени по NTP ---
bool syncTimeFromNtp(uint32_t timeout_ms = 10000) {
  Serial.print("[TIME] Sync via NTP (" NTP_SERVER ")...");
  configTime(NTP_GMT_OFFSET, NTP_DAYLIGHT_OFFSET, NTP_SERVER);

  struct tm ti;
  if (getLocalTime(&ti, timeout_ms)) {
    Serial.printf(" OK -> %04d-%02d-%02d %02d:%02d:%02d\n",
                  ti.tm_year + 1900,
                  ti.tm_mon + 1,
                  ti.tm_mday,
                  ti.tm_hour,
                  ti.tm_min,
                  ti.tm_sec);
    ntpSynced = true;
  } else {
    Serial.println(" FAIL (timeout)");
    ntpSynced = false;
  }

  lastNtpAttemptMs = millis();
  return ntpSynced;
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
   if (!syncTimeFromNtp()) {
    Serial.println("[WARN] Initial NTP sync failed, will retry in background");
  }
  connectMQTT();
}

void loop() {
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  if (WiFi.status() == WL_CONNECTED) {
    const unsigned long nowMs = millis();
    const unsigned long interval = ntpSynced ? NTP_RESYNC_INTERVAL_MS : NTP_RETRY_INTERVAL_MS;
    if (nowMs - lastNtpAttemptMs >= interval) {
      syncTimeFromNtp(ntpSynced ? 2000 : 5000);
    }
  }

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