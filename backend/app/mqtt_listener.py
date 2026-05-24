import json
import logging
from datetime import datetime

import paho.mqtt.client as mqtt

from .config import MQTT_BROKER, MQTT_PORT, MQTT_TOPIC_MEASUREMENTS, MQTT_CLIENT_ID
from .database import SessionLocal
from .schemas import MeasurementCreate
from .crud import create_measurement

logger = logging.getLogger(__name__)

mqtt_status = "offline"


def get_mqtt_status() -> str:
    return mqtt_status


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
    global mqtt_status
    if rc == 0:
        mqtt_status = "connected"
        logger.info("[MQTT] Connected")
        client.subscribe(MQTT_TOPIC_MEASUREMENTS)
        logger.info("[MQTT] Subscribed to: %s", MQTT_TOPIC_MEASUREMENTS)
    else:
        mqtt_status = "offline"
        logger.warning("[MQTT] Connect failed rc=%s", rc)


def on_disconnect(client, userdata, rc):
    global mqtt_status
    mqtt_status = "offline"
    if rc != 0:
        logger.warning("[MQTT] Unexpected disconnect rc=%s", rc)


def on_message(client, userdata, msg):
    raw = msg.payload.decode("utf-8")
    logger.info("[MQTT] Received: %s", raw)

    try:
        payload = json.loads(raw)

        measurement = MeasurementCreate(
            device_id=payload.get("device_id") or payload.get("device") or "esp32-01",
            timestamp=parse_timestamp(payload.get("timestamp")),
            current=(payload.get("current") if payload.get("current") is not None
                     else payload.get("current_rms") if payload.get("current_rms") is not None
            else payload.get("i_rms")),
            voltage=(payload.get("voltage") if payload.get("voltage") is not None
                     else payload.get("voltage_rms") if payload.get("voltage_rms") is not None
            else payload.get("v_rms")),
            power=(payload.get("power") if payload.get("power") is not None
                   else payload.get("apparent_power") if payload.get("apparent_power") is not None
            else payload.get("s_est_va")),
            channel=payload.get("channel") or payload.get("channel_current")
        )


        db = SessionLocal()
        try:
            saved = create_measurement(db, measurement)
            logger.info("[MQTT] Measurement saved id=%s", saved.id)
        finally:
            db.close()

    except Exception as e:
        logger.exception("[MQTT] Error processing message: %s", e)

def start_mqtt_listener():
    global mqtt_status
    client = mqtt.Client(client_id=MQTT_CLIENT_ID)
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message
    client.reconnect_delay_set(min_delay=1, max_delay=30)

    logger.info("[MQTT] Connecting to %s:%s", MQTT_BROKER, MQTT_PORT)
    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
    except Exception as e:
        mqtt_status = "offline"
        logger.warning("[MQTT] Broker unavailable at startup (%s). Running in degraded mode.", e)

    return client

