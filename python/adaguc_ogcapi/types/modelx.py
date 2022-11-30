from pydantic import BaseModel


class modelx(BaseModel):
    x: int

    @classmethod
    def custom_init(cls, x: int):
        return cls(x=x)
