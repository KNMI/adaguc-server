from typing import Optional

from pydantic import BaseModel

from .extent import Extent, Spatial, Temporal
from .link import Link
from .styles import Styles


class collection(BaseModel):
    id: str
    links: list[Link]
    title: Optional[str] = None
    description: Optional[str] = None
    attribution: Optional[str] = None
    extent: Optional[Extent] = None
    itemType: Optional[str] = None
    crs: Optional[list[str]] = None
    styles: Styles

    @classmethod
    def custom_init(
        cls,
        id: str,
        links: Optional[list[Link]],
        styles: Optional[Styles],
        title: Optional[str] = None,
        extent: Optional[Extent] = None,
        crs: Optional[list[str]] = [
            "http://www.opengis.net/def/crs/EPSG/0/4326",
            "http://www.opengis.net/def/crs/EPSG/0/3875",
        ],
    ):
        return cls(
            id=id, links=links, title=title, extent=extent, styles=styles, crs=crs
        )
