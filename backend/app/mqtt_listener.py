import json
from datetime import datetime

import paho.mqtt.client as mqtt

from .config import MQTT_BROKER, MQTT_PORT, MQTT_TOPIC_MEASUREMENTS, MQTT_CLIENT_ID
from .database import SessionLocal
from .schemas import MeasurementCreate
from .crud import create_measurement


def parse_timestamp(value):
    if not value:
        return datetime.utcnow()

    try:
        if value.endswith("Z"):
            value = value.replace("Z", "+00:00")
        return datetime.fromisoformat(value)
    except Exception:
        return datetime.utcnow()


def on_connect(client, userdata, flags, rc):
    print(f"[MQTT] Connected with result code {rc}")
    client.subscribe(MQTT_TOPIC_MEASUREMENTS)
    print(f"[MQTT] Subscribed to: {MQTT_TOPIC_MEASUREMENTS}")


def on_message(client, userdata, msg):
    raw = msg.payload.decode("utf-8")
    print(f"[MQTT] Received: {raw}")

    try:
        payload = json.loads(raw)

        measurement = MeasurementCreate(
            device_id=payload.get("device_id") or payload.get("device") or "esp32-01",
            timestamp=parse_timestamp(payload.get("timestamp")),
            current=payload.get("current") if payload.get("current") is not None else payload.get("i_rms"),
            voltage=payload.get("voltage"),
            power=payload.get("power") if payload.get("power") is not None else payload.get("s_est_va"),
            channel=payload.get("channel")
        )

        db = SessionLocal()
        try:
            create_measurement(db, measurement)
            print("[MQTT] Measurement saved to DB")
        finally:
            db.close()

    except Exception as e:
        print(f"[MQTT] Error processing message: {e}")


def start_mqtt_listener():
    client = mqtt.Client(client_id=MQTT_CLIENT_ID)
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"[MQTT] Connecting to {MQTT_BROKER}:{MQTT_PORT}")
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()

    return client