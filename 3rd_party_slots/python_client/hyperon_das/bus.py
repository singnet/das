import enum


class BusCommand(str, enum.Enum):
    PATTERN_MATCHING_QUERY = "pattern_matching_query"


class Bus:
    def __init__(self, other: 'Bus' = None) -> None:
        if other is not None:
            self._command_owner = dict(other._command_owner)
        else:
            self._command_owner = {}

    def add(self, command: str) -> None:
        if command in self._command_owner:
            if self._command_owner[command] != "":
                raise RuntimeError(f"Bus: command <{command}> is already assigned to {self._command_owner[command]}")
        else:
            self._command_owner[command] = ""

    def set_ownership(self, command: str, node_id: str) -> None:
        if command not in self._command_owner:
            raise RuntimeError(f"Bus: command <{command}> is not defined")
        else:
            if self._command_owner[command] == "":
                self._command_owner[command] = node_id
            else:
                if self._command_owner[command] != node_id:
                    raise RuntimeError(f"Bus: command <{command}> is already assigned to {self._command_owner[command]}")

    def get_ownership(self, command: str) -> str:
        if command not in self._command_owner:
            raise RuntimeError(f"Bus: unknown command <{command}>")
        return self._command_owner[command]

    def contains(self, command: str) -> bool:
        return command in self._command_owner

    def to_string(self) -> str:
        entries = []
        for cmd, owner in self._command_owner.items():
            if owner:
                entries.append(f"{cmd}:{owner}")
            else:
                entries.append(cmd)
        return "{" + ", ".join(entries) + "}"

    def __eq__(self, other: 'Bus') -> bool:
        return self._command_owner == other._command_owner

    def __add__(self, command: str) -> 'Bus':
        self.add(command)
        return self
