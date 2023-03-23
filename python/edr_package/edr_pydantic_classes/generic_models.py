from typing import List
from typing import Optional
from enum import Enum

from datetime import datetime

from covjson_pydantic.domain import DomainType
from pydantic import Field

from .my_base_model import MyBaseModel


class CRSOptions(str, Enum):
    wgs84 = "WGS84"


class Spatial(MyBaseModel):
    bbox: list[list[float]]
    crs: CRSOptions
    name: Optional[str]


class Temporal(MyBaseModel):
    interval: list[list[datetime]]
    values: list[str]
    trs: str
    name: Optional[str]


class Vertical(MyBaseModel):
    interval: List[List[float]]
    vrs: str
    name: Optional[str]


class Custom(MyBaseModel):
    interval: List[str]
    id: str
    values: List[str]
    reference: Optional[str] = None


class Extent(MyBaseModel):
    spatial: Optional[Spatial]
    temporal: Optional[Temporal]
    vertical: Optional[Vertical]
    custom: Optional[List[Custom]]


class CrsObject(MyBaseModel):
    crs: str
    wkt: str


class ObservedPropertyCollection(MyBaseModel):
    id: Optional[str] = None
    label: str
    description: Optional[str] = None
    # categories


class Units(MyBaseModel):
    label: Optional[str]
    symbol: Optional[str]
    id: Optional[str] = None


class ParameterName(MyBaseModel):
    id: Optional[str] = None
    type: str = "Parameter"
    label: Optional[str] = None
    description: Optional[str] = None
    data_type: Optional[str] = None
    observedProperty: ObservedPropertyCollection
    extent: Optional[Extent] = None
    unit: Optional[Units] = None


class Variables(MyBaseModel):
    crs_details: list[CrsObject]
    default_output_format: Optional[str] = None
    output_formats: list[str] = []
    query_type: str = ""
    title: str = ""


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
    variables: Optional[Variables]


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
