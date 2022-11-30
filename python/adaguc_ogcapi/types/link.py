from pydantic import BaseModel


class Link(BaseModel):
    href: str
    rel: str
    description: str | None = None
    type: str | None = None
    hreflang: str | None = None
    title: str | None = None

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
