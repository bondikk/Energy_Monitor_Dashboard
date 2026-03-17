from datetime import datetime
from typing import Optional, List

from pydantic import BaseModel, EmailStr


class UserCreate(BaseModel):
    email: EmailStr
    username: str
    password: str


class UserLogin(BaseModel):
    email: EmailStr
    password: str


class UserOut(BaseModel):
    id: int
    email: EmailStr
    username: str
    created_at: datetime

    class Config:
        from_attributes = True


class Token(BaseModel):
    access_token: str
    token_type: str = "bearer"


class TokenData(BaseModel):
    sub: Optional[str] = None


class DeviceOut(BaseModel):
    id: int
    device_id: str
    name: str
    status: str
    channel: Optional[str] = None
    last_seen: Optional[datetime] = None
    created_at: datetime

    class Config:
        from_attributes = True


class MeasurementBase(BaseModel):
    current: Optional[float] = None
    voltage: Optional[float] = None
    power: Optional[float] = None
    channel: Optional[str] = None


class MeasurementCreate(MeasurementBase):
    device_id: str
    timestamp: Optional[datetime] = None


class MeasurementOut(MeasurementBase):
    id: int
    timestamp: datetime

    class Config:
        from_attributes = True


class MeasurementWithDeviceOut(MeasurementOut):
    device_id: str
    device_name: str


class DashboardStats(BaseModel):
    device_id: str
    status: str
    last_seen: Optional[datetime] = None
    latest_current: Optional[float] = None
    latest_voltage: Optional[float] = None
    latest_power: Optional[float] = None
    avg_current: Optional[float] = None
    avg_voltage: Optional[float] = None
    avg_power: Optional[float] = None
    measurements_count: int


class MeasurementHistoryResponse(BaseModel):
    items: List[MeasurementWithDeviceOut]