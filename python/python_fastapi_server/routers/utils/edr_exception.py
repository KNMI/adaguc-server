"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

from fastapi import HTTPException


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


def exec_unknown_parameter(collection_name: str, param: str) -> EdrException:
    return EdrException(
        code=404,
        description=f"Incorrect parameter {param} requested for collection {collection_name}",
    )


def exc_failed_call(mess: str) -> EdrException:
    return EdrException(
        code=500,
        description=mess,
    )


def exc_invalid_point(coords: str) -> EdrException:
    return EdrException(
        code=400,
        description=f"Could not parse WKT POINT, received coords={coords}",
    )
