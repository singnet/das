UINT32_MAX = 2**32 - 1


class Properties(dict):
    def tokenize(self) -> list[str]:
        vec = []
        for k, v in self.items():
            vec.append(k)
            if isinstance(v, str):
                vec.extend(["string", v])
            elif isinstance(v, bool):
                vec.extend(["bool", str(v).lower()])
            elif isinstance(v, int):
                if 0 <= v <= UINT32_MAX:  # TODO: This doesn't work completely. Change it.
                    vec.extend(["unsigned_int", str(v)])
                else:
                    vec.extend(["long", str(v)])
            elif isinstance(v, float):
                vec.extend(["double", str(v)])
            else:
                raise TypeError(f"Unsupported property type: {type(v)}")
        return vec

    def untokenize(self, tokens: list[str]) -> None:
        if len(tokens) % 3 != 0:
            raise ValueError(f"Invalid tokens vector size: {len(tokens)}")

        cursor = 0
        n = len(tokens)
        while cursor < n:
            key = tokens[cursor]
            cursor += 1
            typ = tokens[cursor]
            cursor += 1
            val = tokens[cursor]
            cursor += 1

            if typ == "string":
                self[key] = val
            elif typ == "long":
                self[key] = int(val)
            elif typ == "unsigned_int":
                ival = int(val)
                if ival < 0:
                    raise ValueError(f"Invalid unsigned_int (negative) for key '{key}': {val}")
                self[key] = ival
            elif typ == "double":
                self[key] = float(val)
            elif typ == "bool":
                if val == "true":
                    self[key] = True
                elif val == "false":
                    self[key] = False
                else:
                    raise ValueError(f"Invalid 'bool' string value: {val}")
            else:
                raise ValueError(f"Invalid token type: {typ}")

    def to_string(self) -> str:
        if not self.keys():
            return "{}"

        parts = []
        for key in sorted(self.keys()):
            value = self[key]
            if isinstance(value, bool):
                val_str = "true" if value else "false"
            elif isinstance(value, str):
                val_str = f"'{value}'"
            elif isinstance(value, (int,)) and not isinstance(value, bool):
                val_str = str(value)
            elif isinstance(value, float):
                val_str = str(value)
            else:
                # fallback other types
                val_str = repr(value)

            parts.append(f"{key}: {val_str}")

        return "{" + ", ".join(parts) + "}"