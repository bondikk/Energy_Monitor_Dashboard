from datetime import datetime
from typing import Optional

from sqlalchemy import desc, func
from sqlalchemy.orm import Session

from app import models, schemas


def get_user_by_email(db: Session, email: str) -> Optional[models.User]:
    return db.query(models.User).filter(models.User.email == email).first()


def get_user_by_username(db: Session, username: str) -> Optional[models.User]:
    return db.query(models.User).filter(models.User.username == username).first()


def create_user(db: Session, email: str, username: str, password_hash: str) -> models.User:
    user = models.User(
        email=email,
        username=username,
        password_hash=password_hash
    )
    db.add(user)
    db.commit()
    db.refresh(user)
    return user


def get_device_by_device_id(db: Session, device_id: str) -> Optional[models.Device]:
    return db.query(models.Device).filter(models.Device.device_id == device_id).first()


def create_device(db: Session, device_id: str, name: str, channel: Optional[str] = None) -> models.Device:
    device = models.Device(
        device_id=device_id,
        name=name,
        channel=channel,
        status="offline"
    )
    db.add(device)
    db.commit()
    db.refresh(device)
    return device


def get_or_create_device(db: Session, device_id: str, name: str = "Unknown device", channel: Optional[str] = None) -> models.Device:
    device = get_device_by_device_id(db, device_id)
    if device:
        return device
    return create_device(db, device_id=device_id, name=name, channel=channel)


def update_device_status(
    db: Session,
    device: models.Device,
    status: Optional[str] = None,
    channel: Optional[str] = None,
    last_seen: Optional[datetime] = None
) -> models.Device:
    if status is not None:
        device.status = status
    if channel is not None:
        device.channel = channel
    if last_seen is not None:
        device.last_seen = last_seen

    db.commit()
    db.refresh(device)
    return device


def create_measurement(db: Session, payload: schemas.MeasurementCreate) -> models.Measurement:
    device = get_or_create_device(
        db,
        device_id=payload.device_id,
        name=payload.device_id,
        channel=payload.channel
    )

    ts = payload.timestamp or datetime.utcnow()

    measurement = models.Measurement(
        device_ref_id=device.id,
        timestamp=ts,
        current=payload.current,
        voltage=payload.voltage,
        power=payload.power,
        channel=payload.channel
    )

    db.add(measurement)

    device.status = "online"
    device.last_seen = ts
    if payload.channel is not None:
        device.channel = payload.channel

    db.commit()
    db.refresh(measurement)
    return measurement


def get_latest_measurement(db: Session, device_id: Optional[str] = None) -> Optional[models.Measurement]:
    query = db.query(models.Measurement).join(models.Device)

    if device_id:
        query = query.filter(models.Device.device_id == device_id)

    return query.order_by(desc(models.Measurement.timestamp)).first()


def get_measurements_history(db: Session, device_id: Optional[str] = None, limit: int = 100):
    query = db.query(models.Measurement, models.Device).join(models.Device)

    if device_id:
        query = query.filter(models.Device.device_id == device_id)

    return query.order_by(desc(models.Measurement.timestamp)).limit(limit).all()


def get_all_devices(db: Session):
    return db.query(models.Device).order_by(models.Device.device_id.asc()).all()


def get_dashboard_stats(db: Session, device_id: str):
    device = get_device_by_device_id(db, device_id)
    if not device:
        return None

    latest = (
        db.query(models.Measurement)
        .filter(models.Measurement.device_ref_id == device.id)
        .order_by(desc(models.Measurement.timestamp))
        .first()
    )

    stats = (
        db.query(
            func.count(models.Measurement.id),
            func.avg(models.Measurement.current),
            func.avg(models.Measurement.voltage),
            func.avg(models.Measurement.power),
        )
        .filter(models.Measurement.device_ref_id == device.id)
        .first()
    )

    count, avg_current, avg_voltage, avg_power = stats

    return {
        "device_id": device.device_id,
        "status": device.status,
        "last_seen": device.last_seen,
        "latest_current": latest.current if latest else None,
        "latest_voltage": latest.voltage if latest else None,
        "latest_power": latest.power if latest else None,
        "avg_current": float(avg_current) if avg_current is not None else None,
        "avg_voltage": float(avg_voltage) if avg_voltage is not None else None,
        "avg_power": float(avg_power) if avg_power is not None else None,
        "measurements_count": count or 0,
    }