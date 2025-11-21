#pragma once
#define WIFI_SSID "ESPTEST"
#define WIFI_PASS "27100910"
#define MQTT_HOST "172.20.10.2"
#define MQTT_PORT 1883
#define DEVICE_ID "esp32-01"
#define MQTT_TOPIC "nilm/esp32-01/telemetry"

// Optional overrides for NTP behaviour
// #define NTP_SERVER "pool.ntp.org"
// #define NTP_GMT_OFFSET (3 * 3600)
// #define NTP_DAYLIGHT_OFFSET 3600
// #define NTP_RESYNC_INTERVAL_MS (3UL * 60UL * 60UL * 1000UL)
// #define NTP_RETRY_INTERVAL_MS  (15UL * 1000UL)