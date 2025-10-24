

class Properties:
    def __init__(self):
        self.P = {}

    def tokenize(self) -> list[str]:
        vec = []
        for k, v in self.P.items():
            vec.append(k)
            if isinstance(v, str):
                vec.extend(["string", v])
            elif isinstance(v, bool):
                vec.extend(["bool", str(v).lower()])
            elif isinstance(v, int):
                vec.extend(["unsigned_int", str(v)])
            elif isinstance(v, float):
                vec.extend(["double", str(v)])
            else:
                raise TypeError(f"Unsupported property type: {type(v)}")
        vec.insert(0, str(len(vec)))
        return vec

    def insert(self, key: str, value: str | int | float | bool) -> None:
        self.P[key] = value
