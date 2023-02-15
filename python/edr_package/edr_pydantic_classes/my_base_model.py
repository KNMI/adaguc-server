import orjson
from pydantic import BaseModel
from pydantic import Extra


def orjson_dumps(v, *, default):
    # orjson.dumps returns bytes, to match standard json.dumps we need to decode
    return orjson.dumps(
        v, default=default, option=orjson.OPT_NON_STR_KEYS | orjson.OPT_UTC_Z | orjson.OPT_NAIVE_UTC
    ).decode()


class MyBaseModel(BaseModel):
    class Config:
        allow_population_by_field_name = True
        anystr_strip_whitespace = True
        extra = Extra.forbid
        min_anystr_length = 1
        smart_union = True
        validate_all = True
        validate_assignment = True

        json_loads = orjson.loads
        json_dumps = orjson_dumps
