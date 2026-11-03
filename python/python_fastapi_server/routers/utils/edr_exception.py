"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

from fastapi import HTTPException
from enum import Enum


class AdagucErrorCode(str, Enum):
    OperationNotSupported = "OperationNotSupported"
    InvalidDimensionValue = "InvalidDimensionValue"
    InvalidDataset = "InvalidDataset"
    InvalidLayer = "InvalidLayer"
    UnprocessableEntity = "UnprocessableEntity"


class EdrException(HTTPException):
    """
    Exception class for EDR
    """

    def __init__(self, code: int, description: str):
        super().__init__(int(code), description)


def exc_unknown_collection(collection: str) -> EdrException:
    return EdrException(code=404, description=f"Collection {collection} unknown")


def exc_no_datasets() -> EdrException:
    return EdrException(code=404, description="No datasets available")


def exc_incorrect_instance(collection_name: str, instance: str) -> EdrException:
    return EdrException(
        code=404,
        description=f"Incorrect instance {instance} for collection {collection_name}",
    )


def exc_unknown_parameter(collection_name: str, param: str) -> EdrException:
    return EdrException(
        code=404,
        description=f"Incorrect parameter {param} requested for collection {collection_name}",
    )


def exc_failed_call(mess: str) -> EdrException:
    return EdrException(
        code=400,
        description=mess,
    )


def exc_invalid_point(coords: str) -> EdrException:
    return EdrException(
        code=400,
        description=f"Could not parse WKT POINT, received coords={coords}",
    )


def native_json_response(status, body, operation):
    """Distinguish native request rejection from execution and response failures."""
    import json

    # CGIRunner returns process codes; test/native HTTP wrappers may translate them.
    if status in (32, 404):
        raise EdrException(404, f"{operation} data request found no matching data")
    if status in (33, 422):
        raise EdrException(422, f"{operation} data request is unsupported for the selected data or exceeds its limits")
    if status in (35, 400):
        raise EdrException(400, f"{operation} data request is invalid")
    if status != 0:
        raise EdrException(500, f"{operation} data request failed")
    try:
        data = json.loads(body)
    except (json.JSONDecodeError, UnicodeDecodeError, TypeError) as exc:
        raise EdrException(500, f"{operation} data request returned an invalid JSON response") from exc
    if not isinstance(data, list):
        raise EdrException(500, f"{operation} data request returned an invalid data response")
    return data
