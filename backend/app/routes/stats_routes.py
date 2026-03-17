from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session

from app.database import get_db
from app import crud, schemas
from app.config import DEFAULT_DEVICE_ID

router = APIRouter(prefix="/stats", tags=["stats"])


@router.get("/dashboard", response_model=schemas.DashboardStats)
def get_dashboard_stats(
    device_id: str = DEFAULT_DEVICE_ID,
    db: Session = Depends(get_db)
):
    stats = crud.get_dashboard_stats(db, device_id)
    if not stats:
        raise HTTPException(status_code=404, detail="Device not found")
    return schemas.DashboardStats(**stats)