from flask_openapi3 import APIBlueprint, Tag
from pydantic import BaseModel
from enum import Enum
from typing import Optional
from coveragejson_pydantic.coverage_json import CoverageJson, Domain

routeKDPApi = APIBlueprint('KDPAPI', __name__, url_prefix='/api')

book_tag = Tag(name='book', description='Some Book')
domain_tag = Tag(name='domain', description='domain')

class BookQuery(BaseModel):
    age: int
    author: str

class DataPath(BaseModel):
    collectionId: str

class DataQuery(BaseModel):
    domain: Domain
    parameters: Optional[str]

@routeKDPApi.get('/book', tags=[book_tag])
def create_book(query: BookQuery):
    return {"message": "success"}

@routeKDPApi.post(
    "/<collectionId>/data",
    responses={"200":   CoverageJson},
    extra_responses={"200": {"content": {"application/prs.coverage+json": {"schema": {"$ref": "#/components/schemas/CoverageJson"}}}}}
)
def get_data(path: DataPath, query:DataQuery) -> CoverageJson:
    print(CoverageJson())
    return CoverageJson()