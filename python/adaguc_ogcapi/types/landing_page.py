from pydantic import BaseModel
from ..types.link import Link


class landing_page(BaseModel):
    title: str
    description: str
    attribution: str | None = None
    links: list[Link] | None

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
