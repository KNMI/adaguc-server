from pydantic import BaseModel


class conformance(BaseModel):
    conformsTo: list[str]

    @classmethod
    def custom_init(cls, _conformsTo: list[str]):
        return cls(conformsTo=_conformsTo)
