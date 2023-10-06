from typing import Optional

from pydantic import BaseModel


class Spatial(BaseModel):
    bbox: list[float]
    crs: str

    @classmethod
    def custom_init(cls, bbox: list[float], crs: str):
        return cls(bbox=bbox, crs=crs)


class Temporal(BaseModel):
    interval: list[str]
    trs: str

    @classmethod
    def custom_init(cls, interval: list[str], trs: str):
        return cls(interval=interval, trs=trs)


class Extent(BaseModel):
    spatial: Spatial
    temporal: Temporal = None

    @classmethod
    def custom_init(
        cls,
        bbox: list[float],
        crs: str,
        interval: Optional[list[str]] = None,
        trs: str = "ISO8601",
        other_dims: Optional[dict] = None,
    ):
        _spatial = Spatial.custom_init(bbox, crs)
        _temporal: Temporal = Temporal.custom_init(interval, trs)
        if interval and trs:
            _temporal = Temporal.custom_init(interval, trs)
        return cls(spatial=_spatial, temporal=_temporal)
