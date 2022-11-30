from pydantic import BaseModel

from .extent import Extent
from .style import Style
from .link import Link


class Styles(BaseModel):
    links: list[Link]
    styles: list[Style]
    defaultStyle: str | None = None
    extent: Extent | None = None

    @classmethod
    def custom_init(
        cls,
        styles: list[Style],
        links: list[Link] = None,
        defaultStyle=None,
        extent=extent,
    ):
        return cls(styles=styles, links=links, defaultStyle=defaultStyle, extent=extent)
