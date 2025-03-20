"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""


class EdrException(Exception):
    """
    Exception class for EDR
    """

    def __init__(self, code: str, description: str):
        self.code = code
        self.description = description


def exc_unknown_collection(collection: str) -> EdrException:
    return EdrException(code=400, description=f"Collection {collection} unknown")


def exc_no_datasets() -> EdrException:
    return EdrException(code=404, description="No datasets available")
