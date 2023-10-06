from pydantic import BaseModel

from .collection import collection
from .link import Link


class collections(BaseModel):
    links: list[Link]
    collections: list[collection]

    @classmethod
    def custom_init(cls, colls: list[collection], links: list[Link] = None):
        return cls(collections=colls, links=links)
