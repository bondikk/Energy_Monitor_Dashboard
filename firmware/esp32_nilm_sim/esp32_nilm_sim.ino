#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <ADS1256.h>
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

// --- ADS1256 wiring (ESP32 VSPI pins by default) ---
const int PIN_DRDY = 26;
const int PIN_CS   = 25;

#ifndef ADS1256_VREF_VOLTS
#define ADS1256_VREF_VOLTS 2.500f
#endif

// Calibration helpers — tune to your front-end
static const float VOLTAGE_DIVIDER_RATIO = 1.0f;  // e.g. 11.0f if using 1:11 divider
static const float SHUNT_RESISTOR_OHMS   = 0.1f;  // adjust if using different shunt
static const uint8_t ADC_OVERSAMPLE      = 8;     // how many samples to average per publish
static const uint32_t SAMPLE_RATE_HZ     = 15000; // ADS1256 target sample rate

// ADS1256: DRDY, RESET(не используем), PDWN(не используем), CS, Vref, SPI
ADS1256 adc(
  PIN_DRDY,
  ADS1256::PIN_UNUSED,
  ADS1256::PIN_UNUSED,
  PIN_CS,
  ADS1256_VREF_VOLTS,
  &SPI
);

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
    Serial.println("\n[WiFi] Connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\n[WiFi] FAILED");
    return false;
  }
}

// --- NTP sync ---
bool syncTimeFromNtp(uint32_t timeout_ms = 10000) {
  Serial.print("[NTP] Sync using ");
  Serial.print(NTP_SERVER);
  Serial.println(" ...");

  configTime(NTP_GMT_OFFSET, NTP_DAYLIGHT_OFFSET, NTP_SERVER);

  unsigned long start = millis();
  struct tm ti;
  while (!getLocalTime(&ti) && (millis() - start) < timeout_ms) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (getLocalTime(&ti)) {
    ntpSynced = true;
    Serial.print("[NTP] OK: ");
    Serial.print(asctime(&ti));
  } else {
    Serial.println("[NTP] FAILED");
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
    Serial.print(MQTT_HOST);
    Serial.print(":");
    Serial.print(MQTT_PORT);
    Serial.print(" ... ");

    if (mqtt.connect(DEVICE_ID)) {
      Serial.println("OK");
    } else {
      Serial.print("FAIL rc=");
      Serial.println(mqtt.state());
      // rc подсказки: -2=нет соединения/хост недоступен; 4/5=авторизация
      delay(1500);
    }
  }
}

// --- чтение ADS1256 с усреднением ---
float readAdsVoltage(uint8_t mux, uint8_t oversample) {
  adc.setMUX(mux);
  long total = 0;
  for (uint8_t i = 0; i < oversample; ++i) {
    total += adc.readSingle();
  }
  long avgRaw = total / oversample;
  return adc.convertToVoltage(avgRaw);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== ESP32 NILM SIM START ===");

  Serial.println("[BOOT] ADS1256 init...");
  adc.InitializeADC();
  adc.setMUX(SING_0); // выбираем первый канал по умолчанию

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

    // --- реальные измерения с ADS1256 ---
    const float adcVoltage   = readAdsVoltage(SING_0, ADC_OVERSAMPLE) * VOLTAGE_DIVIDER_RATIO;
    const float adcCurrentV  = readAdsVoltage(SING_1, ADC_OVERSAMPLE);
    const float currentAmps  = SHUNT_RESISTOR_OHMS > 0.0f ? (adcCurrentV / SHUNT_RESISTOR_OHMS) : 0.0f;
    const float v_rms        = adcVoltage;   // при необходимости примените преобразование RMS
    const float i_rms        = currentAmps;  // при необходимости добавьте фильтрацию/смещение
    const float p            = v_rms * i_rms;

    StaticJsonDocument<256> doc;
    doc["device"]      = DEVICE_ID;
    doc["timestamp"]   = iso8601();
    doc["v_rms"]       = roundf(v_rms * 1000) / 1000.0f;
    doc["i_rms"]       = roundf(i_rms * 1000) / 1000.0f;
    doc["p_active"]    = roundf(p * 1000) / 1000.0f;
    doc["sample_rate"] = SAMPLE_RATE_HZ;
    doc["raw_adc_v"]   = adcVoltage;   // для отладки: напряжение на делителе
    doc["raw_adc_i"]   = adcCurrentV;  // для отладки: падение на шунте

    char payload[256];
    serializeJson(doc, payload);
    Serial.print("[PUB] ");
    Serial.println(payload);
    mqtt.publish(MQTT_TOPIC, payload);
  }
}
