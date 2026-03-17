from typing import Optional

from fastapi import APIRouter, Depends
from sqlalchemy.orm import Session

from app.database import get_db
from app import crud, schemas

router = APIRouter(prefix="/devices", tags=["devices"])


@router.get("/", response_model=list[schemas.DeviceOut])
def get_devices(db: Session = Depends(get_db)):
    return crud.get_all_devices(db)


@router.get("/{device_id}", response_model=Optional[schemas.DeviceOut])
def get_device(device_id: str, db: Session = Depends(get_db)):
    return crud.get_device_by_device_id(db, device_id)