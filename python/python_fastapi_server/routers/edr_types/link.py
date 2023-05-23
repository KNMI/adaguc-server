from pydantic import BaseModel
from typing import Optional

class Link(BaseModel):
    href: str
    rel: str
    description: Optional[str] = None
    type: Optional[str] = None
    hreflang: Optional[str] = None
    title: Optional[str] = None

    @classmethod
    def custom_init(
        cls,
        href: str,
        rel: str,
        description: str = None,
        type: str = None,
        hreflang: str = None,
        title: str = None,
    ):
        return cls(
            href=href,
            rel=rel,
            type=type,
            description=description,
            hreflang=hreflang,
            title=title,
        )
