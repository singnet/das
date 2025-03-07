from copy import copy

MAX_NUMBER_OF_VARIABLES_IN_QUERY = 100
MAX_VARIABLE_NAME_SIZE = 100
HANDLE_HASH_SIZE = 33
MAX_NUMBER_OF_OPERATION_CLAUSES = 100


class Assignment:
    def __init__(self):
        self.size = 0
        self.assignments = {}
        self.labels = []
        self.values = []

    def assign(self, label: str, value: str) -> bool:
        if self.assignments.get(label):
            return self.assignments.get(label) == value
        self.assignments[label] = value
        return True

    def get(self, label: str) -> str:
        return self.assignments.get(label)

    def is_compatible(self, other) -> bool:
        for k in self.assignments.keys():
            for other_k in other.assignments.keys():
                if (
                    k in other
                    and other_k in self.assignments
                    and other.get(k) != self.assignments.get(k)
                ):
                    return False
        # for i in range(self.size):
        #     for j in range(other.size):
        #         if self.labels[i] == other.labels[j] and self.values[i] != other.values[j]:
        #             return False
        return True

    def copy_from(self, other):
        self.assignments = copy(other.assignments)

    def add_assignments(self, other):
        for k, v in other.assignments.items():
            if v in self.assignments:
                break
            self.assignments[k] = v
        # for j in range(other.size):
        #     already_contains = False
        #     for i in range(self.size):
        #         if self.labels[i] == other.labels[j]:
        #             already_contains = True
        #             break
        #     if not already_contains:
        #         self.labels[self.size] = other.labels[j]
        #         self.values[self.size] = other.values[j]
        #         self.size += 1

    def variable_count(self) -> int:
        return len(self.assignments)

    def to_string(self) -> str:
        return "{" + ", ".join([f"({k}: {v})" for k, v in self.assignments.items()]) + "}"

    @staticmethod
    def read_token(token_string: str, cursor: int, token_size: int) -> str:
        token = []
        while cursor < len(token_string) and token_string[cursor] != ' ':
            if len(token) >= token_size:
                raise ValueError("Invalid token string")
            token.append(token_string[cursor])
            cursor += 1
        return ''.join(token), cursor + 1


class QueryAnswer:
    def __init__(self, handle: str = None, importance: float = 0.0):
        self.handles = []
        self.handles_size = 0
        self.importance = importance
        self.assignment = Assignment()

        if handle:
            self.add_handle(handle)

    def add_handle(self, handle: str):
        self.handles[self.handles_size] = handle
        self.handles_size += 1

    def merge(self, other, merge_handles: bool = True) -> bool:
        if self.assignment.is_compatible(other.assignment):
            self.assignment.add_assignments(other.assignment)
            if merge_handles:
                self.importance = max(self.importance, other.importance)
                for j in range(other.handles_size):
                    if self.handles_size < MAX_NUMBER_OF_OPERATION_CLAUSES:
                        if other.handles[j] not in self.handles[: self.handles_size]:
                            self.handles[self.handles_size] = other.handles[j]
                            self.handles_size += 1
            return True
        else:
            return False

    @staticmethod
    def copy(base):
        new_copy = QueryAnswer(importance=base.importance)
        new_copy.assignment.copy_from(base.assignment)
        new_copy.handles = base.handles[: base.handles_size]
        new_copy.handles_size = base.handles_size
        return new_copy

    def tokenize(self) -> str:
        importance_str = f"{self.importance:.10f}"
        token_representation = f"{importance_str} {self.handles_size} "

        token_representation += " ".join(self.handles[: self.handles_size]) + " "
        token_representation += f"{self.assignment.size} "

        for i in range(self.assignment.size):
            token_representation += f"{self.assignment.labels[i]} {self.assignment.values[i]} "

        return token_representation.strip()

    def untokenize(self, tokens: str):
        cursor = 0
        token_string = tokens.split(' ')

        importance = token_string[cursor]
        self.importance = float(importance)
        cursor += 1

        num_handles = int(token_string[cursor])
        self.handles_size = num_handles
        cursor += 1

        self.handles = token_string[cursor : cursor + num_handles]
        cursor += num_handles

        num_assignments = int(token_string[cursor])
        self.assignment.size = num_assignments
        cursor += 1

        for i in range(num_assignments):
            label = token_string[cursor]
            value = token_string[cursor + 1]
            self.assignment.assign(label, value)
            cursor += 2

    def to_string(self) -> str:
        handles_str = ", ".join(self.handles)
        return f"QueryAnswer<{self.handles_size},{self.assignment.variable_count()}> [{handles_str}] {self.assignment.to_string()} {self.importance}"

    def get_handles(self):
        return self.handles[: self.handles_size]

    def __str__(self):
        return self.to_string()
