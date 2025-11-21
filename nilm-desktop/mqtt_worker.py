import json
import threading
import time
import queue
from datetime import datetime, timezone

import paho.mqtt.client as mqtt

class MqttWorker(threading.Thread):
    def __init__(self, broker="localhost", port=1883, topic="nilm/+/telemetry"):
        super().__init__(daemon=True)
        self.broker = broker
        self.port = port
        self.topic = topic
        self.q = queue.Queue()
        self._stop = False

    def run(self):
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

        def on_connect(c, u, f, rc, properties=None):
            self.q.put(("status", f"[MQTT] Connected rc={rc}"))
            c.subscribe(self.topic, qos=1)

        def on_message(c, u, msg, properties=None):
            try:
                j = json.loads(msg.payload.decode("utf-8"))
                if "timestamp" not in j:
                    j["timestamp"] = datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")
                self.q.put(("data", j))
            except Exception as e:
                self.q.put(("status", f"[ERR] Bad JSON: {e}"))

        client.on_connect = on_connect
        client.on_message = on_message

        while not self._stop:
            try:
                client.connect(self.broker, self.port, keepalive=30)
                client.loop_forever()
            except Exception as e:
                self.q.put(("status", f"[MQTT] Reconnect in 3s: {e}"))
                time.sleep(3)

    def get(self):
        try:
            return self.q.get_nowait()
        except queue.Empty:
            return None

    def stop(self):
        self._stop = True