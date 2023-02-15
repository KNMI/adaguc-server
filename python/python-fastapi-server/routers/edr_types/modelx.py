from pydantic import BaseModel


class modelx(BaseModel):
    x: int
    y: str = None

    @classmethod
    def custom_init(cls, x: int, y: str = None):
        return cls(x=x, y=y)
