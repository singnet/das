from hyperon_das.logger import log


class Assignment:
    """
    Represents a set of variable assignments (label -> value).
    """

    def __init__(self) -> None:
        self._mapping = {}
        self.max_size = 100

    def assign(self, label: str, value: str) -> bool:
        """
        Adds a new assignment or verifies an existing one.

        If `label` already exists, returns True only if its existing value matches `value`.
        Otherwise, adds the (label, value) pair.

        Args:
            label (str): The variable label.
            value (str): The value to assign to the label.

        Returns:
            bool: True if the assignment was added or matches the existing value.

        Raises:
            ValueError: If the assignment size exceeds `max_size`.
        """
        if label in self._mapping:
            return self._mapping[label] == value
        else:
            if len(self._mapping) >= self.max_size:
                raise ValueError(f"Assignment size exceeds the maximal number of allowed variables in a query: {self.max_size}")
        self._mapping[label] = value
        return True

    def get(self, label: str) -> str | None:
        """
        Retrieves the value assigned to a label.

        Args:
            label (str): The variable label to look up.

        Returns:
            str: The assigned value if present, otherwise None.
        """
        return self._mapping.get(label)

    def is_compatible(self, other: "Assignment") -> bool:
        """
        Checks compatibility with another assignment.

        Two assignments are compatible if they never assign the same label to two different values.

        Args:
            other (Assignment): The other assignment to compare against.

        Returns:
            bool: True if compatible, False otherwise.
        """
        for label, value in self._mapping.items():
            if label in other._mapping and other._mapping[label] != value:
                return False
        return True

    def copy_from(self, other: "Assignment") -> None:
        """
        Replaces this assignment's contents with a copy of another's contents.

        Args:
            other (Assignment): The assignment to copy from.
        """
        self._mapping = other._mapping.copy()

    def add_assignments(self, other: "Assignment") -> None:
        """
        Adds assignments from another assignment that are not already present.

        Args:
            other (Assignment): The assignment whose entries to add.
        """
        for label, value in other._mapping.items():
            if label not in self._mapping:
                self._mapping[label] = value

    def variable_count(self) -> int:
        """
        Return the number of assigned variables.
        """
        return len(self._mapping)

    def to_string(self) -> str:
        """
        Return a string of the form: {(label1: value1), (label2: value2), ...}
        """
        pairs = [f"({label}: {value})" for label, value in self._mapping.items()]
        return "{" + ", ".join(pairs) + "}"

    def __eq__(self, other: object) -> bool:
        """
        Checks equality with another Assignment.

        Equality means same set of labels and values, order-independent.

        Args:
            other (object): The object to compare against.

        Returns:
            bool: True if `other` is Assignment with identical mappings.
        """
        if not isinstance(other, Assignment):
            return False
        return self._mapping == other._mapping


class QueryAnswer:
    """
    Encapsulates a set of handles, an importance score, and an associated Assignment.
    Provides methods for merging, tokenizing, and reconstructing.
    """

    def __init__(self, handle: str | None = None, importance: float = 0.0) -> None:
        self.handles = []
        self.metta_expression = {}
        if handle is not None:
            self.handles.append(handle)
        self.importance = importance
        self.strength = 0.0
        self.assignment = Assignment()

    @classmethod
    def copy(cls, other: "QueryAnswer") -> "QueryAnswer":
        """
        Creates a deep copy of another QueryAnswer.

        Args:
            other (QueryAnswer): The QueryAnswer to copy.

        Returns:
            QueryAnswer: A new instance with duplicated data.
        """
        q = cls(importance=other.importance)
        q.strength = other.strength
        q.handles = other.handles
        q.assignment.copy_from(other.assignment)
        return q

    def add_handle(self, handle: str) -> None:
        """
        Append a new handle to the list.
        """
        self.handles.append(handle)

    def merge(self, other: "QueryAnswer", merge_handles: bool = True) -> bool:
        """
        Merges another QueryAnswer into this one if assignments are compatible.

        Args:
            other (QueryAnswer): The other QueryAnswer to merge.
            merge_handles (bool): Whether to merge handle lists and importance.

        Returns:
            bool: True if merge succeeded, False if assignments were incompatible.
        """
        if not self.assignment.is_compatible(other.assignment):
            return False

        self.assignment.add_assignments(other.assignment)

        if merge_handles:
            self.importance = max(self.importance, other.importance)
            self.strength = self.strength * other.strength
            existing = set(self.handles)
            for handle in other.handles:
                if handle not in existing:
                    self.handles.append(handle)
                    existing.add(handle)

        return True

    def tokenize(self) -> str:
        """
        Produces a space-delimited token string representation.

        Format: "importance handles_count handles... assignment_count label value...".

        Returns:
            str: Tokenized string encoding this QueryAnswer.
        """
        tokens = [f"{self.strength:.10f} {self.importance:.10f}"]
        tokens.append(str(len(self.handles)))
        tokens.extend(self.handles)
        tokens.append(str(self.assignment.variable_count()))
        for label, value in self.assignment._mapping.items():
            tokens.append(label)
            tokens.append(value)
        return " ".join(tokens)

    def untokenize(self, token_str: str) -> None:
        """
        Parses a token string to populate this QueryAnswer's fields.

        Args:
            token_str (str): String produced by `tokenize()`.

        Raises:
            ValueError: If the token string format is invalid.
        """
        log.debug(f"Untokenizing QueryAnswer from: {token_str}")
        tokens = token_str.strip().split()
        cursor = 0

        def next_token():
            nonlocal cursor
            if cursor >= len(tokens):
                raise ValueError("Invalid token string: unexpected end of tokens")
            token = tokens[cursor]
            cursor += 1
            return token

        try:
            self.strength = float(next_token())
            self.importance = float(next_token())

            handles_size = int(next_token())
            if handles_size < 0:
                raise ValueError(f"Handles size cannot be negative: {handles_size}")

            self.handles = [next_token() for _ in range(handles_size)]

            assignment_size = int(next_token())
            if assignment_size < 0:
                raise ValueError(f"Assignment size cannot be negative: {assignment_size}")

            self.assignment = Assignment()
            for _ in range(assignment_size):
                label = next_token()
                value = next_token()
                self.assignment.assign(label, value)
                log.debug(f"Parsed assignment: ({label}: {value})")

            metta_expression_size = int(next_token())

            for _ in range(metta_expression_size + 1):
                key = next_token()
                self.metta_expression[key] = next_token()

            log.debug(f"Metta expression: {self.metta_expression}")

            if cursor != len(tokens):
                log.error(f"cursor: {cursor}, tokens: {tokens}")
                raise ValueError("Invalid token string: extra data after parsing")

            log.debug(f"QueryAnswer untokenized successfully: {self.to_string()}")

        except (ValueError, IndexError) as e:
            log.error(str(e))
            raise

    def to_string(self) -> str:
        """
        Returns a human-readable string representation of the QueryAnswer.

        Format:
            QueryAnswer<handles_count,assignment_count> [handles...] {assignments...} importance
        """
        handles_str = ", ".join(self.handles)

        return f"QueryAnswer<{len(self.handles)},{self.assignment.variable_count()}> [{handles_str}] {self.assignment.to_string()} ({self.strength:.6f}, {self.importance:.6f})"
