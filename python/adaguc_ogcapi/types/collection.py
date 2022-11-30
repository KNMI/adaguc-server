from pydantic import BaseModel
from ..types.extent import Extent, Spatial, Temporal
from ..types.link import Link
from ..types.styles import Styles


class collection(BaseModel):
    id: str
    links: list[Link]
    title: str | None = None
    description: str | None = None
    attribution: str | None = None
    extent: Extent | None = None
    itemType: str | None = None
    crs: list[str] = [
        "http://www.opengis.net/def/crs/EPSG/0/4326",
        "http://www.opengis.net/def/crs/EPSG/0/3875",
    ]
    styles: Styles

    @classmethod
    def custom_init(
        cls,
        id: str,
        links: list[Link],
        styles: Styles,
        title: str = None,
        extent: Extent = None,
    ):
        return cls(id=id, links=links, title=title, extent=extent, styles=styles)
