from pydantic import BaseModel
from .extent import Extent, Spatial, Temporal
from .link import Link
from .styles import Styles


class collection(BaseModel):
    id: str
    links: list[Link]
    title: str | None = None
    description: str | None = None
    attribution: str | None = None
    extent: Extent | None = None
    itemType: str | None = None
    crs: list[str] | None = None
    styles: Styles

    @classmethod
    def custom_init(
        cls,
        id: str,
        links: list[Link],
        styles: Styles,
        title: str = None,
        extent: Extent = None,
        crs: list[str]
        | None = [
            "http://www.opengis.net/def/crs/EPSG/0/4326",
            "http://www.opengis.net/def/crs/EPSG/0/3875",
        ],
    ):
        return cls(
            id=id, links=links, title=title, extent=extent, styles=styles, crs=crs
        )
