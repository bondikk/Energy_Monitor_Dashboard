# NILM Host – ESP32 Telemetry Sandbox

*Lightweight ESP32 + Python sandbox for Non-Intrusive Load Monitoring experiments.*

---

## About the Project

This repository combines two parts of an experimental **Non-Intrusive Load Monitoring (NILM)** system:

1. **ESP32 Firmware** — generates synthetic energy data, synchronizes time via NTP, and publishes telemetry to MQTT.
2. **Python Host Application** — subscribes to MQTT topics, visualizes live data, and saves measurements to CSV.

Together they form a minimal testbed for NILM algorithm development without using real sensors.
You can use it as a starting point for integrating real hardware, building data-processing pipelines, or training NILM models.

---

## Repository Structure

```
README.md                   — main project guide  
requirements.txt            — Python dependencies  
host/
  mqtt_listener.py          — MQTT subscriber with plotting and logging  
firmware/
  esp32_nilm_sim/
    esp32_nilm_sim.ino      — ESP32 telemetry simulator firmware  
    secrets.h               — Wi-Fi, MQTT, and NTP configuration  
```

---

## Requirements

### Hardware

* **ESP32 DevKit V4** board with Wi-Fi support.

### Software

* **Python 3.9+** on your computer.
* An **MQTT broker** (e.g., Mosquitto) reachable from both ESP32 and the host PC.
* **Arduino IDE** (or PlatformIO) with the **ESP32** board package installed.

> ℹ️ The host uses Matplotlib with the `Qt5Agg` backend.
> On Linux, install `PyQt5` or `PySide2` if missing:
>
> ```bash
> pip install pyqt5
> ```

---

## Quick Start (Host)

1. Create and activate a Python virtual environment (optional):

   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   ```
2. Install dependencies:

   ```bash
   pip install -r requirements.txt
   pip install pyqt5  # if not installed automatically
   ```
3. Edit `host/mqtt_listener.py` and set your MQTT broker IP:

   ```python
   BROKER = "172.20.10.2"
   ```
4. Run the host:

   ```bash
   python host/mqtt_listener.py
   ```
5. Once telemetry starts arriving, a live window will appear showing three plots:
   voltage, current, and active power. Every 60 samples are auto-appended to
   `log_auto.csv`. On exit (`Ctrl+C`), the full buffer is saved to `log.csv`.

> To test without hardware, you can publish a fake JSON packet:
>
> ```bash
> mosquitto_pub -t nilm/demo/telemetry -m '{"v_rms":230,"i_rms":0.3,"p_active":69}'
> ```

---

## Building and Flashing ESP32

1. Edit `firmware/esp32_nilm_sim/secrets.h` with your credentials:

   ```cpp
   #define WIFI_SSID "MyWiFi"
   #define WIFI_PASS "SuperSecret"
   #define MQTT_HOST "192.168.1.10"
   #define MQTT_PORT 1883
   #define DEVICE_ID "esp32-nilm-sim"
   #define MQTT_TOPIC "nilm/sim/telemetry"
   // Optional: override NTP servers below if needed
   ```
2. Open `esp32_nilm_sim.ino` in **Arduino IDE**.
3. Select **Board → ESP32 Dev Module** and the correct **COM/tty port**.
4. Upload the firmware.

In the Serial Monitor you should see logs showing Wi-Fi, NTP, and MQTT connection steps, followed by published JSON messages.

The firmware generates sinusoidal voltage/current profiles with small “load events,” synchronizes time via NTP, and publishes one data packet per second.
If Wi-Fi or MQTT disconnects, the ESP32 automatically retries.

---

## Telemetry Format

Example MQTT payload:

```json
{
  "device": "esp32-nilm-sim",
  "timestamp": "2024-05-02T12:30:45Z",
  "v_rms": 229.7,
  "i_rms": 0.34,
  "p_active": 78.1,
  "sample_rate": 15000
}
```

| Field         | Description                                               |
| ------------- | --------------------------------------------------------- |
| `device`      | Device ID (from `DEVICE_ID`).                             |
| `timestamp`   | ISO8601 UTC time. If missing, the host fills current UTC. |
| `v_rms`       | RMS voltage in volts.                                     |
| `i_rms`       | RMS current in amperes.                                   |
| `p_active`    | Active power in watts.                                    |
| `sample_rate` | Sampling rate of the simulator (Hz).                      |

The Python host keeps the latest **600 points** in a circular buffer (~10 min at 1 Hz), updates live plots, and writes logs automatically.

---

## Development and Extension

* Adjust the buffer window size in `mqtt_listener.py` (`deque(maxlen=600)`).
* To save every packet, remove the condition `len(WIN) % 60 == 0`.
* Subscribe to multiple devices via `nilm/+/telemetry` and use the `device` column for filtering.
* Replace the simulation block in the firmware with real sensor readings (e.g., **ADE7953**, **PZEM-004T**, **SCT-013**) while keeping the same JSON format.

---

## Useful Links

* [PubSubClient for Arduino](https://pubsubclient.knolleary.net/)
* [Matplotlib Interactive Figures](https://matplotlib.org/stable/users/explain/interactive.html)
* [Eclipse Paho MQTT Client (Python)](https://www.eclipse.org/paho/index.php?page=clients/python/index.php)
