from typing import Optional

from pydantic import BaseModel

from .extent import Extent
from .link import Link
from .style import Style


class Styles(BaseModel):
    links: list[Link]
    styles: list[Style]
    defaultStyle: Optional[str] = None
    extent: Optional[Extent] = None
    maxWidth: int = 4000
    maxHeight: int = 3000

    @classmethod
    def custom_init(
        cls,
        styles: list[Style],
        links: list[Link] = None,
        defaultStyle=None,
        extent=extent,
    ):
        if not defaultStyle:
            _defaultStyle = styles[0].id
        else:
            _defaultStyle = defaultStyle
        return cls(
            styles=styles, links=links, defaultStyle=_defaultStyle, extent=extent
        )
