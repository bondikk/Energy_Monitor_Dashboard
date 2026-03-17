from fastapi import APIRouter, Depends, Query
from sqlalchemy.orm import Session
from typing import Optional

from app.database import get_db
from app import crud, schemas

router = APIRouter(prefix="/measurements", tags=["measurements"])


@router.get("/latest", response_model=Optional[schemas.MeasurementWithDeviceOut])
def get_latest(device_id: Optional[str] = None, db: Session = Depends(get_db)):
    item = crud.get_latest_measurement(db, device_id=device_id)
    if not item:
        return None

    return schemas.MeasurementWithDeviceOut(
        id=item.id,
        timestamp=item.timestamp,
        current=item.current,
        voltage=item.voltage,
        power=item.power,
        channel=item.channel,
        device_id=item.device.device_id,
        device_name=item.device.name
    )


@router.get("/history", response_model=schemas.MeasurementHistoryResponse)
def get_history(
    device_id: Optional[str] = None,
    limit: int = Query(default=50, le=500),
    db: Session = Depends(get_db)
):
    rows = crud.get_measurements_history(db, device_id=device_id, limit=limit)

    items = []
    for measurement, device in rows:
        items.append(
            schemas.MeasurementWithDeviceOut(
                id=measurement.id,
                timestamp=measurement.timestamp,
                current=measurement.current,
                voltage=measurement.voltage,
                power=measurement.power,
                channel=measurement.channel,
                device_id=device.device_id,
                device_name=device.name
            )
        )

    return schemas.MeasurementHistoryResponse(items=items)