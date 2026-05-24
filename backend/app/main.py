from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from sqlalchemy import text

from app.database import SessionLocal
from app.init_db import init_db
from app.mqtt_listener import start_mqtt_listener, get_mqtt_status
from app.routes.auth_routes import router as auth_router
from app.routes.measurement_routes import router as measurement_router
from app.routes.device_routes import router as device_router
from app.routes.stats_routes import router as stats_router

mqtt_client = None


@asynccontextmanager
async def lifespan(app: FastAPI):
    global mqtt_client
    init_db()
    mqtt_client = start_mqtt_listener()
    yield
    if mqtt_client is not None:
        mqtt_client.loop_stop()
        mqtt_client.disconnect()


app = FastAPI(
    title="NILM Backend API",
    version="0.1.0",
    lifespan=lifespan
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "http://localhost:5173",
        "http://127.0.0.1:5173",
    ],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(auth_router)
app.include_router(measurement_router)
app.include_router(device_router)
app.include_router(stats_router)


@app.get("/")
def root():
    return {"message": "NILM backend is running"}


@app.get("/health")
def health():
    db_status = "ok"
    db = SessionLocal()
    try:
        db.execute(text("SELECT 1"))
    finally:
        db.close()

    return {
        "status": "ok",
        "mqtt": get_mqtt_status(),
        "database": db_status,
    }