from typing import List
from typing import Optional

from covjson_pydantic.domain import DomainType
from pydantic import Field

from .my_base_model import MyBaseModel


class Link(MyBaseModel):
    href: str = Field(
        ...,
        example="http://data.example.com/collections/monitoringsites/locations/1234",
    )
    rel: str = Field(..., example="alternate")
    type: Optional[str] = Field(None, example="application/geo+json")
    hreflang: Optional[str] = Field(None, example="en")
    title: Optional[str] = Field(None, example="Monitoring site name")
    length: Optional[int] = None
    templated: Optional[bool] = Field(
        False,
        description="defines if the link href value is a template with values requiring replacement",
    )


class SupportedQueries(MyBaseModel):
    domain_types: List[DomainType] = Field(
        description="A list of domain types from which can be determined what endpoints are allowed.",
        example="When [DomainType.point_series] is returned, "
        "the /position endpoint is allowed but /cube is not allowed.",
    )
    has_locations: bool = Field(
        description="A boolean from which can be determined if the backend has the /locations endpoint.",
        example="When True is returned, the backend has the /locations endpoint.",
    )
