# NILM Host

## Overview

This repository contains three implemented parts of a small NILM-oriented telemetry stack:

## 1. **A FastAPI backend** that stores energy measurements in SQLite and exposes HTTP endpoints for authentication, device discovery, measurement history, and dashboard statistics.
2. **A minimal React frontend source tree** that polls the backend for the latest measurement and renders a simple dashboard.
3. **ESP32 firmware** for an `ESP32 + ADS1256` setup that reads current data, estimates voltage/apparent power, and publishes telemetry over MQTT.

Based on the code that is actually present, the repository is currently focused on **collecting telemetry from one or more MQTT-publishing devices, persisting it locally, and displaying recent values through a browser UI**. It is not yet a full NILM pipeline with appliance disaggregation or model training.## Purpose
Together they form a minimal testbed for NILM algorithm development without using real sensors.
You can use it as a starting point for integrating real hardware, building data-processing pipelines, or training NILM models.

## Purpose
The implemented code suggests the project is intended to act as a local host application for an energy-monitoring / NILM prototype:
- an embedded device measures electrical signals,
- telemetry is published to an MQTT broker,
- the backend subscribes to those MQTT messages,
- the backend stores normalized measurements in SQLite,
- and a frontend reads the backend API to display the latest values.

Where this README makes an inference about intent, it is based on code identifiers such as `NILM Backend API`, telemetry topics, the `ESP32 + ADS1256` device naming, and the measurement/statistics endpoints.

## What the code currently does

### Backend

The backend starts a FastAPI application with a lifespan hook that:

- initializes the SQLite schema, and
- starts a background MQTT client subscription loop.

On incoming MQTT messages, the backend:

1. decodes the JSON payload,
2. maps known telemetry field names into a `MeasurementCreate` schema,
3. creates the device row if it does not exist,
4. stores the measurement in SQLite,
5. and updates the device status to `online` with a `last_seen` timestamp.

The HTTP API provides routes for:

- user registration and login,
- listing devices,
- reading a device by `device_id`,
- reading the latest measurement,
- reading measurement history,
- and computing aggregate dashboard statistics for a device.

### Frontend

The frontend source code renders a browser dashboard that polls `http://127.0.0.1:8001/measurements/latest` every 2 seconds and displays:

- current,
- voltage,
- apparent power,
- measurement metadata such as device ID, timestamp, and channel.

The frontend codebase is very small and appears to be in a partially transitional state:

- `App.jsx` reads the backend response shape used by the FastAPI API.
- `src/components/Dashboard.jsx` expects an older/raw telemetry shape (`i_rms`, `voltage_rms`, `s_est_va`, `device`), but this component is not used by `main.jsx`.
- there is **no `package.json` or Vite configuration file in the repository**, so the frontend source is present but the build/run manifest is currently missing from version control.

### Firmware

The firmware configures an ESP32 to:

- initialize an ADS1256 ADC over SPI,
- compute RMS current using repeated ADC reads from `AIN0-AIN1`,
- call a voltage sensor stub that currently returns `0.0f`,
- fall back to a configured fixed mains voltage (`230.0 V`) when the stub returns a low value,
- estimate apparent power as `voltageRms * currentRms`,
- and publish a JSON telemetry payload via MQTT once per second.

The telemetry payload contains fields such as:

- `device`,
- `timestamp`,
- `i_rms`,
- `voltage_rms`,
- `s_est_va`,
- `adc_sample_sps`,
- `channel_current`,
- `channel_voltage`,
- and `note`.

## Main implemented features

- **FastAPI service bootstrap** with automatic DB initialization and MQTT listener startup.
- **SQLite persistence** through SQLAlchemy models for users, devices, and measurements.
- **MQTT ingestion** from a single configured topic.
- **Telemetry normalization** from either canonical fields (`current`, `voltage`, `power`) or firmware-style fields (`i_rms`, `voltage_rms`, `s_est_va`).
- **Automatic device creation** when telemetry arrives for an unknown device.
- **Device status tracking** using `status`, `channel`, and `last_seen`.
- **User registration and JWT login** endpoints.
- **Latest measurement/history/statistics** API routes.
- **Simple polling frontend** for the latest measurement.
- **ESP32 firmware** for periodic telemetry publication.

## System architecture / workflow

```text
ESP32 + ADS1256 firmware
    |
    | MQTT publish: nilm/esp32-01/telemetry
    v
MQTT broker
    |
    | subscribed by backend listener
    v
FastAPI backend
    |- parses JSON payload
    |- creates/updates device row
    |- inserts measurement row
    |- exposes REST API
    v
SQLite database (data/nilm.db)
    ^
    |
React frontend polls HTTP API every 2 seconds
```

### Practical request / data flow

1. The firmware publishes one JSON message per second to the configured MQTT topic.
2. The backend MQTT client subscribes to that topic on startup.
3. Each MQTT message is converted into a database record.
4. The frontend requests `/measurements/latest` over HTTP.
5. The backend joins measurement and device data and returns a JSON response for display.
6. Optional API consumers can also query history, device lists, and aggregate dashboard stats.

## Repository structure

```text
.
├── README.md
├── data/
│   └── nilm.db                  # SQLite database file already present in the repo
├── backend/
│   ├── requirements.txt         # Python dependencies
│   ├── run.py                   # uvicorn launcher
│   ├── tests/
│   │   └── test_auth_passwords.py
│   └── app/
│       ├── auth.py              # password hashing and JWT creation
│       ├── config.py            # hard-coded app configuration
│       ├── crud.py              # DB access helpers
│       ├── database.py          # SQLAlchemy engine/session/base
│       ├── init_db.py           # schema creation
│       ├── main.py              # FastAPI app and startup lifecycle
│       ├── models.py            # SQLAlchemy models
│       ├── mqtt_listener.py     # MQTT subscription and DB ingestion
│       ├── schemas.py           # Pydantic schemas
│       ├── routes/
│       │   ├── auth_routes.py
│       │   ├── device_routes.py
│       │   ├── measurement_routes.py
│       │   └── stats_routes.py
│       └── services/
│           ├── measurement_service.py  # currently empty
│           └── stats_service.py        # currently empty
└── frontend/
    └── nilm-frontend/
        └── src/
            ├── App.jsx
            ├── api.js
            ├── components/
            │   └── Dashboard.jsx
            └── main.jsx
firmware/
└── esp32_nilm_sim/
    ├── esp32_nilm_sim.ino       # main firmware loop
    ├── ADS1256Driver.*          # ADC driver
    ├── CurrentSensor.*          # RMS current calculation
    ├── VoltageSensor.*          # voltage stub
    ├── PowerMetrics.*           # apparent power helper
    ├── Connectivity.*           # Wi-Fi, NTP, MQTT publishing
    ├── config.h                 # hardware and measurement constants
    └── secrets.h                # Wi-Fi / MQTT credentials and topic
```

## Requirements / dependencies

### Backend dependencies

`backend/requirements.txt` lists:

- `fastapi`
- `uvicorn`
- `sqlalchemy`
- `pydantic`
- `python-jose`
- `passlib[bcrypt]`
- `email-validator`
- `python-multipart`
- `paho-mqtt`
- `pandas`
- `matplotlib`
- `bcrypt==4.3.0`

Notes based on the code:

- `fastapi`, `uvicorn`, `sqlalchemy`, `pydantic`, `python-jose`, `passlib`, `email-validator`, `python-multipart`, and `paho-mqtt` are used directly.
- `pandas` and `matplotlib` are listed but are **not referenced by the current backend source tree**.
- `paho-mqtt` appears twice in `requirements.txt`.

### Frontend dependencies

The source code clearly uses:

- React,
- ReactDOM,
- browser `fetch`.

However, because `package.json` is missing, the exact npm package versions and scripts are not defined in the repository.

### Firmware / Arduino dependencies

The firmware includes:

- `Arduino.h`
- `SPI.h`
- `WiFi.h`
- `PubSubClient.h`
- `ArduinoJson.h`
- ESP32 time/NTP functions via `time.h`

### External runtime components

To run the system as implemented, you also need:

- **Python 3** for the backend,
- **an MQTT broker** reachable by both the backend and ESP32,
- **SQLite** (embedded through Python, no separate server required),
- **an ESP32 environment** such as Arduino IDE or PlatformIO for firmware compilation/upload,
- and a browser for the frontend.

## Installation

### 1. Clone the repository

```bash
git clone <your-repo-url>
cd nilm_host
```

### 2. Set up the backend

```bash
cd backend
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

If your shell does not use `source`, activate the environment using the appropriate command for your platform.

### 3. Prepare an MQTT broker

The backend is hard-coded to connect to:

- host: `127.0.0.1`
- port: `1883`
- topic: `nilm/esp32-01/telemetry`

Unless you modify the code, you should run a broker locally on the same machine as the backend.

### 4. Prepare the frontend

The frontend source is present under `frontend/nilm-frontend/src`, but the repository does **not** currently include `package.json`, a bundler config, or npm scripts. A new developer can inspect and reuse the source, but cannot reproduce the frontend build exactly from this repository alone without adding the missing manifest/config files.

### 5. Prepare the firmware

Open `firmware/esp32_nilm_sim/esp32_nilm_sim.ino` in Arduino IDE or PlatformIO, and review `secrets.h` and `config.h` before flashing.

## Configuration

### Backend configuration

The backend does **not currently read environment variables**. All runtime settings are hard-coded in `backend/app/config.py`.

Current backend settings:

- database path: `data/nilm.db`
- MQTT broker: `127.0.0.1`
- MQTT port: `1883`
- MQTT topic: `nilm/esp32-01/telemetry`
- MQTT client ID: `nilm_backend_listener`
- default dashboard device ID: `esp32-01`
- default device name: `ESP32 + ADS1256`
- JWT secret and algorithm are hard-coded

### Firmware configuration

The firmware stores runtime configuration in header files:

- `config.h` for SPI pins, ADC settings, CT calibration, sample count, and fallback mains voltage.
- `secrets.h` for Wi-Fi SSID/password, MQTT host/port/topic, and device ID.

Important practical note: `secrets.h` in the repository already contains concrete Wi-Fi and MQTT values rather than placeholders. For real use, those values should normally be replaced or moved out of version control.

## Running the project

### Run the backend API

From the repository root:

```bash
cd backend
python run.py
```

This starts Uvicorn on:
- `http://127.0.0.1:8001`
On startup, the backend should:

1. create database tables if needed,
2. connect to the MQTT broker,
3. subscribe to the configured topic,
4. and begin serving HTTP requests.

### Run the frontend
Because no frontend package manifest is included, there is no repository-defined command such as `npm install` / `npm run dev` that can be guaranteed from the current codebase alone.

What can be verified from the code is that the frontend expects the backend at:

- `http://127.0.0.1:8001`

and polls:

- `GET /measurements/latest`

If you restore or create the missing frontend tooling, the frontend source should work as a small polling dashboard.

### Run the firmware

1. Open `firmware/esp32_nilm_sim/esp32_nilm_sim.ino`.
2. Adjust `secrets.h` to match your Wi-Fi and MQTT broker.
3. Build and flash to an ESP32.
4. Open the serial monitor at `115200` baud.
5. Confirm Wi-Fi connection, NTP sync, MQTT connection, and telemetry publishing logs.

## Running individual services / modules

### Backend only

If you only need the API + MQTT ingestion:

```bash
cd backend
python run.py
```

### Database initialization only

```bash
cd backend
python -m app.init_db
```

This only creates the schema; it does not start the HTTP server.

### Authentication tests

```bash
cd backend
python -m unittest tests.test_auth_passwords
```

### MQTT ingestion without hardware

You can publish a test payload manually to the broker using a tool such as `mosquitto_pub`.
A payload matching the implemented parser would look like:

```json
{
  "device": "esp32-01",
  "timestamp": "2026-03-20T12:00:00Z",
  "i_rms": 0.42,
  "voltage_rms": 230.0,
  "s_est_va": 96.6,
  "channel_current": "AIN0-AIN1"
}
```

Because the parser also accepts `device_id`, `current`, `voltage`, `power`, and `channel`, those alternative field names should also be ingested.

## API / endpoints overview

### `GET /`

Health-style root endpoint:

```json
{ "message": "NILM backend is running" }
```

### Authentication

#### `POST /auth/register`

Registers a user with:

```json
{
  "email": "user@example.com",
  "username": "user1",
  "password": "secret"
}
```

Returns the created user record.
#### `POST /auth/login`
Logs in with JSON credentials:
```json
{ 
   "email": "user@example.com",
  "password": "secret"
}
```

Returns a bearer token.

#### `POST /auth/login-form`

Login variant using `OAuth2PasswordRequestForm` fields.

### Devices

#### `GET /devices/`

Returns all known devices ordered by `device_id`.

#### `GET /devices/{device_id}`

Returns one device or `null` if it does not exist.

### Measurements

#### `GET /measurements/latest`

Optional query parameter:

- `device_id`

Returns the newest measurement overall, or the newest one for a specific device.

#### `GET /measurements/history`

Query parameters:

- `device_id` (optional)
- `limit` (default `50`, max `500`)

Returns a measurement history array sorted by descending timestamp.

### Stats

#### `GET /stats/dashboard`

Query parameter:

- `device_id` (defaults to `esp32-01`)

Returns:

- device status,
- last seen time,
- latest current/voltage/power,
- average current/voltage/power,
- total measurement count.

## Messaging / MQTT flow

### Backend subscription

The backend subscribes to exactly one configured topic:

```text
nilm/esp32-01/telemetry
```

### Incoming payload handling

On each message, the backend maps fields as follows:

- `device_id` or `device` -> internal `device_id`
- `timestamp` -> parsed ISO timestamp, or current UTC if parsing fails/missing
- `current` or `i_rms` -> measurement current
- `voltage` or `voltage_rms` -> measurement voltage
- `power` or `s_est_va` -> measurement power
- `channel` or `channel_current` -> channel label

### Persistence effect

Each accepted message:

- inserts one `measurements` row,
- ensures a `devices` row exists,
- sets the device status to `online`,
- updates `last_seen`,
- and optionally updates the device channel.

### Firmware publish behavior

The firmware publishes once every second, using values derived from:

- measured RMS current,
- stub/fallback RMS voltage,
- calculated apparent power.

## Example run workflow

1. Start an MQTT broker on `127.0.0.1:1883` for the backend, or change the backend config to your broker.
2. Install backend dependencies and run `python run.py` from `backend/`.
3. Flash the ESP32 firmware after updating `secrets.h` so the firmware points to the broker reachable from the board.
4. Watch the backend logs for lines showing MQTT connection, subscription, receipt, parsing, and DB insertion.
5. Call `GET /measurements/latest` or open a frontend built from the provided source.
6. Query `GET /devices/`, `GET /measurements/history`, or `GET /stats/dashboard` to inspect stored data.

For a software-only check, skip the ESP32 and publish JSON manually to the MQTT topic.

## Limitations based on current implementation

These limitations are directly visible in the codebase:

1. **No actual NILM/disaggregation logic is implemented.** The code stores and displays telemetry, but does not identify appliances or run energy inference models.
2. **Frontend build files are missing.** The source tree exists, but `package.json` and related tooling files are not versioned.
3. **Frontend code is partially inconsistent.** `App.jsx` and `components/Dashboard.jsx` expect different API payload shapes.
4. **Backend configuration is hard-coded.** There is no environment-variable-based configuration layer.
5. **JWT configuration is duplicated/inconsistent.** `backend/app/config.py` defines JWT settings, but `backend/app/auth.py` uses its own hard-coded `SECRET_KEY`, `ALGORITHM`, and expiration values.
6. **No authorization enforcement is implemented on API routes.** Login issues tokens, but protected endpoints are not present.
7. **Only one MQTT topic is subscribed by default.** Multi-device scaling would require config changes or wildcard topic support.
8. **Device offline detection is not implemented.** Devices become `online` on message receipt, but there is no background timeout that switches them back to `offline`.
9. **Voltage measurement is not implemented in firmware.** `VoltageSensor::readVoltageRMS()` returns `0.0f`, so the firmware uses a fixed fallback voltage.
10. **Firmware appears to contain a compile-time typo.** `Connectivity.cpp` declares `bbool Connectivity::publishTelemetry(...)` instead of `bool`.
11. **Sensitive data is committed in firmware config.** `secrets.h` contains real-looking Wi-Fi/MQTT values rather than placeholders.
12. **Service modules are placeholders.** `backend/app/services/measurement_service.py` and `stats_service.py` are empty.
13. **Database migrations are not implemented.** Schema creation uses `Base.metadata.create_all()` only.
14. **CORS is limited to localhost frontend origins.** This is fine for development, but not production-ready by itself.

## Possible next steps based on the codebase
Reasonable next steps supported by the current structure would be:

1. **Add environment-variable configuration** for DB path, MQTT broker/topic, JWT secret, and CORS origins.
2. **Restore the frontend manifest** (`package.json`, build config, scripts) so the UI is reproducible.
3. **Unify frontend/backend response contracts** and remove the unused divergent dashboard component.
4. **Fix the firmware compile typo** in `Connectivity.cpp`.
5. **Implement real voltage sampling** in `VoltageSensor` instead of using a fixed fallback.
6. **Add protected API routes** that actually validate bearer tokens.
7. **Add offline status logic** based on `last_seen` thresholds.
8. **Support multiple MQTT topics/devices** through configuration or topic wildcards.
9. **Introduce database migrations** (for example via Alembic) if the schema will evolve.
10. **Move secrets out of source control** and provide example config templates.
11. **Populate the empty service modules** or remove them if unnecessary.
12. **Add automated tests** for MQTT parsing, CRUD functions, and API routes.
13. **Add historical charts to the frontend** using `/measurements/history` and `/stats/dashboard`.
14. **Implement actual NILM analysis modules** if appliance-level inference is a planned goal.v

## License
No license file is present in the repository at the time of writing.

If this project is intended for public reuse, add an explicit license such as MIT, Apache-2.0, or GPL, depending on the intended distribution model.