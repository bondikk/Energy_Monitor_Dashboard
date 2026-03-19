from datetime import datetime

from sqlalchemy import Column, Integer, String, Float, DateTime, ForeignKey
from sqlalchemy.orm import relationship

from .database import Base


class User(Base):
    __tablename__ = "users"

    id = Column(Integer, primary_key=True, index=True)
    email = Column(String, unique=True, nullable=False, index=True)
    username = Column(String, unique=True, nullable=False, index=True)
    password_hash = Column(String, nullable=False)
    created_at = Column(DateTime, default=datetime.utcnow, nullable=False)


class Device(Base):
    __tablename__ = "devices"

    id = Column(Integer, primary_key=True, index=True)
    device_id = Column(String, unique=True, nullable=False, index=True)
    name = Column(String, nullable=False)
    status = Column(String, default="offline", nullable=False)
    channel = Column(String, nullable=True)
    last_seen = Column(DateTime, nullable=True)
    created_at = Column(DateTime, default=datetime.utcnow, nullable=False)

    measurements = relationship("Measurement", back_populates="device")


class Measurement(Base):
    __tablename__ = "measurements"

    id = Column(Integer, primary_key=True, index=True)
    device_ref_id = Column(Integer, ForeignKey("devices.id"), nullable=False)

    timestamp = Column(DateTime, default=datetime.utcnow, nullable=False, index=True)
    current = Column(Float, nullable=True)
    voltage = Column(Float, nullable=True)
    power = Column(Float, nullable=True)
    channel = Column(String, nullable=True)

    device = relationship("Device", back_populates="measurements")