from .landing_page import landing_page
from .link import Link
from pydantic import BaseModel


class maps_service_descr(BaseModel):
    service_root: str
    links: list[Link] = ["a", "b"]
    landing: landing_page = None

    @classmethod
    def custom_init(cls, service_root: str):

        links: list(Link) = []
        links.append(
            Link.custom_init(
                href=service_root + "/", rel="self", type="application/json"
            )
        )
        links.append(
            Link.custom_init(
                href=service_root + "/api",
                rel="service-doc",
                type="application/vnd.oai.openapi+json; version=3.0",
                title="OpenAPI definition",
            )
        )
        links.append(
            Link.custom_init(
                href=service_root + "/conformance",
                rel="conformance",
                title="OGCAPI conformance classes",
            )
        )
        links.append(
            Link.custom_init(
                href=service_root + "/collections",
                rel="data",
                title="OGCAPI collections",
            )
        )
        _landing_page = landing_page.custom_init(
            title="OGCAPI test",
            description="Test",
            attribution=None,
            links=links,
        )
        return cls(service_root=service_root, links=links, landing=_landing_page)
