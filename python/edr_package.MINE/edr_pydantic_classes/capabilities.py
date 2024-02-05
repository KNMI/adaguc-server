from __future__ import annotations

from typing import List
from typing import Optional

from pydantic import Field

from .generic_models import Link
from .my_base_model import MyBaseModel


class Provider(MyBaseModel):
    name: str
    url: Optional[str]


class Contact(MyBaseModel):
    email: Optional[str]
    phone: Optional[str]
    fax: Optional[str]
    hours: Optional[str]
    instructions: Optional[str]
    address: Optional[str]
    postalCode: Optional[str]  # noqa: N815
    city: Optional[str]
    stateorprovince: Optional[str]
    country: Optional[str]


class LandingPageModel(MyBaseModel):
    links: List[Link] = Field(
        ...,
        example=[
            {
                "href": "http://www.example.org/edr",
                "hreflang": "en",
                "rel": "service-desc",
                "type": "application/vnd.oai.openapi+json;version=3.0",
                "title": "",
            },
            {
                "href": "http://www.example.org/edr/conformance",
                "hreflang": "en",
                "rel": "data",
                "type": "application/json",
                "title": "",
            },
            {
                "href": "http://www.example.org/edr/collections",
                "hreflang": "en",
                "rel": "data",
                "type": "application/json",
                "title": "",
            },
        ],
    )
    title: Optional[str]
    description: Optional[str]
    keywords: Optional[List[str]]
    provider: Optional[Provider]
    contact: Optional[Contact]


class ConformanceModel(MyBaseModel):
    conformsTo: List[str]  # noqa: N815
