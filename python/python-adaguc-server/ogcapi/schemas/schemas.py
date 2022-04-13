import uuid

from apispec import APISpec
from apispec.ext.marshmallow import MarshmallowPlugin
from apispec_webframeworks.flask import FlaskPlugin
from flask import Flask
from marshmallow import Schema, fields
from marshmallow.validate import OneOf, Range, Length
from marshmallow_oneofschema import OneOfSchema

# Optional marshmallow support
class ExceptionSchema(Schema):
    code = fields.Str(required=True)
    description = fields.Str()

class LinkSchema(Schema):
    href = fields.Str(required=True)
    rel = fields.Str(example="prev")
    type = fields.Str(example="application/json")
    hreflang = fields.Str(example="en", description="Language")

class ExtentSchema(Schema):
    crs = fields.Str()
    spatial = fields.Number()
    trs = fields.Str()
    temporal = fields.DateTime()

class CollectionInfoSchema(Schema):
    id = fields.Str(required=True)
    links = fields.List(fields.Nested(LinkSchema), required=True)
    title = fields.Str()
    description = fields.Str()
    extent = fields.Nested(ExtentSchema)
    crs = fields.List(fields.Str())

class ContentSchema(Schema):
    links = fields.List(fields.Nested(LinkSchema), required=True)
    collections = fields.List(fields.Nested(CollectionInfoSchema))

class RootSchema(Schema):
    links = fields.List(fields.Nested(LinkSchema))

class ReqClassesSchema(Schema):
    example = ['http://www.opengis.net/spec/wfs-1/3.0/req/core',
               'http://www.opengis.net/spec/wfs-1/3.0/req/oas30',
               'http://www.opengis.net/spec/wfs-1/3.0/req/html',
               'http://www.opengis.net/spec/wfs-1/3.0/req/geojson']
    conformsTo = fields.List(fields.Str(), example=example, required=True)

class GeometryGeoJSONSchema(Schema):
    type = fields.Str(validate=OneOf([
        "Point",
        "MultiPoint",
        "LineString",
        "MultiLineString",
        "Polygon",
        "MultiPolygon",
        "GeometryCollection"
        ]), required=True)

class FeaturePropertiesSchema(Schema):
    observationType = fields.Str()
    id = fields.Str()
    datetime = fields.Str()
    phenomenontime = fields.Str()
    observedPropertyName = fields.Str()
    result = fields.Number()
    resultTime = fields.Str()

class FeatureId(fields.Field):
    """Field that serializes to a string of numbers and deserializes
    to a list of numbers.
    """
    def _serialize(self, value, attr, obj, **kwargs):
        if value is None:
            return ""
        elif isinstance(value, str):
            return value
        elif isinstance(value, int):
            return str(value)

    def _deserialize(self, value, attr, data, **kwargs):
        try:
            return int(value)
        except ValueError as error:
            return value

class FeatureGeoJSONSchema(Schema):
    type = fields.Str(required=True, validate=OneOf(["Feature"]))
    geometry = fields.Nested(GeometryGeoJSONSchema)
    properties = fields.Nested(FeaturePropertiesSchema)
    id = fields.Str(required=True)
    storageCrs = fields.Str()

class FeatureCollectionGeoJSONSchema(Schema):
    type = fields.Str(required=True, validate=OneOf(["FeatureCollection"]))
    features = fields.List(fields.Nested(FeatureGeoJSONSchema))
    links = fields.List(fields.Nested(LinkSchema))
    crs = fields.List(fields.Str())
    storageCrs = fields.Str()
    timestamp = fields.Str()
    numberMatched = fields.Int()
    numberReturned = fields.Int()

class CollectionParameter(Schema):
    coll = fields.Str()

class LimitParameter(Schema):
    limit = fields.Int(validate=Range(1, 10000), metadata={"style": "form"})

class BboxParameter(Schema):
    bbox = fields.List(fields.Number(), validate=Length(min=4, max=6), metadata={"explode": False, "style": "form"})

class DatetimeParameter(Schema):
    datetime = fields.Str(metadata={"style": "form"})

class ResultTimeParameter(Schema):
    resultTime = fields.Str(metadata={"style": "form"})

class PhenomenonTimeParameter(Schema):
    phenomenonTime = fields.Str(metadata={"style": "form"})

class LonLatParameter(Schema):
    lonlat = fields.Str()

class LatLonParameter(Schema):
    latlon = fields.Str()

class NPointsParameter(Schema):
    npoints = fields.Str()

class ObservedPropertyNameParameter(Schema):
      observedPropertyName = fields.Str()

def create_apispec(title, version, openapi_version, settings):
    # Create an APISpec
    spec = APISpec(
        title=title,
        version=version,
        openapi_version=openapi_version,
        plugins=[FlaskPlugin(), MarshmallowPlugin()],
        **settings
    )

    # Optional security scheme support
#    api_key_scheme = {"type": "apiKey", "in": "header", "name": "X-API-Key"}
#    spec.components.security_scheme("ApiKeyAuth", api_key_scheme)
    spec.components.schema("link", schema=LinkSchema).\
        schema("extent", schema=ExtentSchema).\
        schema("root", schema = RootSchema).\
        schema("exception", schema=ExceptionSchema).\
        schema("collectioninfo", schema=CollectionInfoSchema).\
        schema("content", schema=ContentSchema).\
        schema("req-classes", schema=ReqClassesSchema).\
        schema("geometryGeoJSON", schema=GeometryGeoJSONSchema).\
        schema("featureProperties", schema=FeaturePropertiesSchema)

    spec.components.schema("hirlamFeatureGeoJSON", schema=FeatureGeoJSONSchema)
    spec.components.schema("hirlamFeatureCollectionGeoJSON", schema=FeatureCollectionGeoJSONSchema)

    #spec.components.parameter("coll", CollectionParameter)

    return spec

