from pydantic import BaseModel
from ..types.link import Link


class Style(BaseModel):
    id: str
    title: str
    links: list[Link]

    @classmethod
    def custom_init(cls, id: str, title: str, links: list[Link]):
        return cls(id=id, title=title, links=links)
