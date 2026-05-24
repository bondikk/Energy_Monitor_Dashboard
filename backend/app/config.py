from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent.parent
DATA_DIR = BASE_DIR / "data"
DATA_DIR.mkdir(exist_ok=True)

DATABASE_URL = f"sqlite:///{DATA_DIR / 'nilm.db'}"

MQTT_BROKER = "127.0.0.1"
MQTT_PORT = 1883
MQTT_TOPIC_MEASUREMENTS = "nilm/esp32-01/measurements"
MQTT_CLIENT_ID = "nilm_backend_listener"

JWT_SECRET_KEY = "change_this_to_a_long_random_secret"
JWT_ALGORITHM = "HS256"
ACCESS_TOKEN_EXPIRE_MINUTES = 60

DEFAULT_DEVICE_ID = "esp32-01"
DEFAULT_DEVICE_NAME = "ESP32 + ADS1256"