from pydantic import BaseModel
from typing import Optional
from .link import Link


class landing_page(BaseModel):
    title: str
    description: str
    attribution: Optional[str] = None
    links: Optional[list[Link]]

    @classmethod
    def custom_init(
        cls,
        title: str,
        description: str,
        attribution: str = None,
        links: list[Link] = None,
    ):
        return cls(
            title=title, description=description, attribution=attribution, links=links
        )
