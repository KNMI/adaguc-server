import logging
from typing import List
from typing import Optional
from typing import Union

from fastapi import HTTPException, Query
from pydantic import BaseModel, Field
from pydantic import ValidationError
from pydantic import validator

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

BBox = List[float]


class ItemsForCollectionQueryModel(BaseModel):
    bbox: BBox = Query(default=None)
    time: Union[str, None] = Query(default=None)

    @classmethod
    def depends(
        cls,
        bbox: str = Query(
            default=None,
            description="Only features that have a geometry that intersects the bounding box are selected. "
            "The bounding box is provided as four numbers. The vertical axis (height or depth) should "
            "be provided in the z parameter. "
            "Example: bbox=4.7,52.3,4.8,52.4",
            example="3,50,7,54",
        ),
        time: Optional[str] = Query(
            None,
            description="Either a date-time or an interval, open or closed. Date and time expressions adhere "
            "to RFC 3339. Open intervals are expressed using double-dots.",
            example="2022-07-22T00:00:00Z/2022-07-23T00:00:00Z",
        ),
    ):
        try:
            return cls(bbox=bbox, time=time)
        except ValidationError as e:
            errors = e.errors()
            raise HTTPException(400, detail=errors)
