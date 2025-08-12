import argparse
import os
import re
import sqlite3
import sys

from collections import defaultdict
from contextlib import contextmanager
from dataclasses import dataclass
from typing import Optional, DefaultDict, TextIO, Generator


BenchmarkEntry = dict[str, str]

BenchmarkData = list[BenchmarkEntry]

ConsolidatedResults = DefaultDict[str, dict[str, list[BenchmarkEntry] | str]]


class FileManager:
    DEFAULT_COL_ORDER = [
        "Backend",
        "Operation",
        "MED",
        "MIN",
        "MAX",
        "P50",
        "P90",
        "P99",
        "TT",
        "TPA",
        "TP",
    ]

    @staticmethod
    def _get_active_columns(entries: list[BenchmarkEntry]) -> list[str]:
        """
        Determines which columns to display based on available data.

        It takes the default column order and filters it to include only columns
        that are present in the provided benchmark entries.

        Args:
            entries: A list of benchmark data rows.

        Returns:
            A list of column names that should be included in the report.
        """
        present_columns = set()
        for entry in entries:
            present_columns.update(entry.keys())
        return [col for col in FileManager.DEFAULT_COL_ORDER if col in present_columns]

    @staticmethod
    def _calculate_column_widths(
        entries: list[BenchmarkEntry], columns: list[str]
    ) -> dict[str, int]:
        """
        Calculates the required width for each column for clean alignment.

        The width is determined by the longer of the column header or the longest
        value in that column across all entries.

        Args:
            entries: A list of benchmark data rows.
            columns: The list of column names to be included in the report.

        Returns:
            A dictionary mapping each column name to its calculated maximum width.
        """
        widths = {col: len(col) for col in columns}
        for entry in entries:
            for col in columns:
                widths[col] = max(widths[col], len(str(entry.get(col, ""))))
        return widths

    @staticmethod
    def write_report(results: ConsolidatedResults, output_file: TextIO, header_text: str = ""):
        """
        Writes the consolidated benchmark results to a specified output file/stream.

        Args:
            results: The consolidated benchmark data.
            output_file: The file or stream (like sys.stdout) to write the report to.
            header_text: An optional header to print at the beginning of the report.
        """
        if header_text:
            output_file.write(f"{header_text}\n\n")

        for op_type, data in sorted(results.items()):
            batch_size = data["batch_size"]
            entries = data["entries"]

            output_file.write(f"[{op_type}] - Batch Size: {batch_size}\n\n")

            columns = FileManager._get_active_columns(entries)
            col_widths = FileManager._calculate_column_widths(entries, columns)

            header_row = "  ".join(col.ljust(col_widths[col]) for col in columns)
            output_file.write(header_row + "\n")

            separator = "  ".join("-" * col_widths[col] for col in columns)
            output_file.write(separator + "\n")

            for entry in entries:
                row = "  ".join(str(entry.get(col, "")).ljust(col_widths[col]) for col in columns)
                output_file.write(row + "\n")
            output_file.write("\n")


@dataclass
class Scenario:
    """Represents a benchmark scenario with its parameters."""

    name: str
    database_size: str
    relationships: str
    concurrency: int
    cache_enabled: bool
    iterations: int


@dataclass
class ResultRow:
    """Represents a single row of benchmark results for database insertion."""

    backend: str
    operation: str
    batch_size: int
    median_ms: float
    min_ms: float
    max_ms: float
    p50_ms: float
    p90_ms: float
    p99_ms: float
    total_time_ms: float
    time_per_atom_ms: float
    throughput: float

    def to_tuple(self) -> tuple:
        """Converts the data class to a tuple for database insertion."""
        return (
            self.backend,
            self.operation,
            self.batch_size,
            self.median_ms,
            self.min_ms,
            self.max_ms,
            self.p50_ms,
            self.p90_ms,
            self.p99_ms,
            self.total_time_ms,
            self.time_per_atom_ms,
            self.throughput,
        )


class DatabaseManager:
    """A class to manage SQLite database operations for benchmark results."""

    def __init__(self, db_name: str):
        """
        Initializes the DatabaseManager with the specified database name.

        Args:
            db_name: The name of the SQLite database file.
        """
        self.db_name = db_name

    @contextmanager
    def cursor(self) -> Generator[sqlite3.Cursor, None, None]:
        """
        A context manager for safe SQLite database connections.

        It handles connection, commit, rollback, and closing.

        Args:
            db_name: The name of the SQLite database file.

        Yields:
            A database cursor object.
        """
        conn = sqlite3.connect(self.db_name)
        try:
            cursor = conn.cursor()
            yield cursor
            conn.commit()
        except sqlite3.Error as e:
            print(f"SQLite error: {e}")
            conn.rollback()
            raise
        finally:
            conn.close()

    def create_tables(self):
        """
        Creates all necessary tables in the database if they don't already exist.
        """
        with self.cursor() as cursor:
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS benchmark_type (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    name TEXT NOT NULL UNIQUE
                );
            """)

            # Table for benchmark scenarios
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS benchmark_scenario (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    scenario_name TEXT UNIQUE,
                    database_size TEXT,
                    atoms_relationships TEXT,
                    concurrent_access INTEGER,
                    cache_enabled BOOLEAN,
                    iterations INTEGER
                );
            """)

            # Table to track each execution of a benchmark
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS benchmark_execution (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    benchmark_type_id INTEGER NOT NULL,
                    benchmark_scenario_id INTEGER NOT NULL,
                    execution_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                    status TEXT NOT NULL CHECK (status IN ('IN_PROGRESS', 'COMPLETED', 'FAILED')),
                    pr_execution_type BOOLEAN NOT NULL DEFAULT 0,
                    pr_link TEXT,
                    pr_title TEXT,
                    completed_at TEXT,
                    FOREIGN KEY (benchmark_type_id) REFERENCES benchmark_type(id),
                    FOREIGN KEY (benchmark_scenario_id) REFERENCES benchmark_scenario(id)
                );
            """)

            # Table to store detailed results for each execution
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS benchmark_result (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    benchmark_execution_id INTEGER NOT NULL,
                    backend TEXT NOT NULL,
                    operation TEXT NOT NULL,
                    batch_size INTEGER NOT NULL,
                    median_operation_time_ms REAL NOT NULL,
                    min_operation_time_ms REAL NOT NULL,
                    max_operation_time_ms REAL NOT NULL,
                    p50_operation_time_ms REAL NOT NULL,
                    p90_operation_time_ms REAL NOT NULL,
                    p99_operation_time_ms REAL NOT NULL,
                    total_time_ms REAL NOT NULL,
                    time_per_atom_ms REAL NOT NULL,
                    throughput REAL NOT NULL,
                    FOREIGN KEY (benchmark_execution_id) REFERENCES benchmark_execution(id)
                );
            """)

    def get_or_create_ids(self, benchmark_type: str, scenario: Scenario) -> tuple[int, int]:
        """
        Gets or creates the IDs for a given benchmark type and scenario.

        If the type or scenario doesn't exist, it will be created.

        Args:
            benchmark_type: The name of the benchmark type (e.g., 'atomdb').
            scenario: A Scenario data object.

        Returns:
            A tuple containing (benchmark_type_id, benchmark_scenario_id).
        """
        with self.cursor() as cursor:
            # Get or create benchmark_type_id
            cursor.execute("SELECT id FROM benchmark_type WHERE name = ?", (benchmark_type,))
            row = cursor.fetchone()
            if row:
                type_id = row[0]
            else:
                cursor.execute("INSERT INTO benchmark_type (name) VALUES (?)", (benchmark_type,))
                type_id = cursor.lastrowid

            # Get or create benchmark_scenario_id
            cursor.execute(
                "SELECT id FROM benchmark_scenario WHERE scenario_name = ?", (scenario.name,)
            )
            row = cursor.fetchone()
            if row:
                scenario_id = row[0]
            else:
                cursor.execute(
                    """
                    INSERT INTO benchmark_scenario (scenario_name, database_size, atoms_relationships, concurrent_access, cache_enabled, iterations)
                    VALUES (?, ?, ?, ?, ?, ?)
                """,
                    (
                        scenario.name,
                        scenario.database_size,
                        scenario.relationships,
                        scenario.concurrency,
                        scenario.cache_enabled,
                        scenario.iterations,
                    ),
                )
                scenario_id = cursor.lastrowid

        return type_id, scenario_id

    def insert_results(self, type_id: int, scenario_id: int, results_data: list[ResultRow]) -> int:
        """
        Inserts a new execution record and all its associated result rows.

        This is done in a single transaction.

        Args:
            type_id: The ID of the benchmark type.
            scenario_id: The ID of the benchmark scenario.
            results_data: A list of ResultRow objects.

        Returns:
            The ID of the new benchmark_execution record.
        """
        with self.cursor() as cursor:
            # Create the main execution record
            cursor.execute(
                """
                INSERT INTO benchmark_execution (benchmark_type_id, benchmark_scenario_id, status)
                VALUES (?, ?, 'IN_PROGRESS')
            """,
                (type_id, scenario_id),
            )
            execution_id = cursor.lastrowid

            # Prepare data for bulk insertion
            records_to_insert = [(execution_id, *result.to_tuple()) for result in results_data]

            # Insert all result rows
            cursor.executemany(
                """
                INSERT INTO benchmark_result (
                    benchmark_execution_id, backend, operation, batch_size,
                    median_operation_time_ms, min_operation_time_ms, max_operation_time_ms,
                    p50_operation_time_ms, p90_operation_time_ms, p99_operation_time_ms,
                    total_time_ms, time_per_atom_ms, throughput
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
                records_to_insert,
            )

        return execution_id


def parse_benchmark_file(filepath: str) -> BenchmarkData:
    """
    Parses a single benchmark file and extracts the data.

    The file is expected to have a header row with column names separated by '|'
    and data rows with values also separated by '|'.

    Args:
        filepath: The path to the benchmark file.

    Returns:
        A list of dictionaries, where each dictionary represents a row.
        Returns an empty list if no valid data is found.
    """
    with open(filepath, "r") as f:
        lines = [line.strip() for line in f if line.strip()]

    header: Optional[list[str]] = None
    for i, line in enumerate(lines):
        if "|" in line and "Operation" in line:
            # Found the header, the next lines should be data.
            header = [h.strip() for h in line.split("|") if h.strip()]
            data_lines = lines[i + 1 :]
            break
    else:
        # No header found in the file.
        return []

    results: BenchmarkData = []
    for line in data_lines:
        # A valid data line must contain '|' and not be a separator line (e.g., |---|---
        is_separator = all(c in "-| " for c in line)
        if "|" in line and not is_separator:
            values = [v.strip() for v in line.split("|") if v.strip()]
            if len(values) == len(header):
                entry = dict(zip(header, values))
                results.append(entry)
    return results


def extract_metadata_from_filename(filename: str) -> Optional[tuple[str, str, str, str, str]]:
    """
    Extracts metadata from the benchmark filename using a regular expression.

    The filename is expected to follow the pattern:
    '{benchmark_type}_{backend}_{op_type}_{method}_{batch_size}.txt'

    Args:
        filename: The name of the benchmark file.

    Returns:
        A tuple containing (benchmark_type, backend, op_type, method, batch_size) if the
        pattern matches, otherwise None.
    """
    # This regex captures everything before _morkdb_ or _redismongodb_ as benchmark_type
    pattern = r"^(.*?)_(morkdb|redismongodb)_([A-Za-z0-9]+)_([A-Za-z0-9_]+)_([0-9]+)\.txt$"
    match = re.match(pattern, filename)
    if match:
        benchmark_type, backend, op_type, method, batch_size = match.groups()
        return benchmark_type, backend, op_type, method, batch_size
    return None


def consolidate_results_from_directory(benchmark_type: str, directory: str) -> ConsolidatedResults:
    """
    Scans a directory for benchmark files, parses them, and consolidates the results.

    Args:
        directory: The path to the directory containing benchmark .txt files.

    Returns:
        A dictionary with consolidated results, grouped by operation type.
    """
    results: ConsolidatedResults = defaultdict(lambda: {"entries": [], "batch_size": "N/A"})

    for filename in sorted(os.listdir(directory)):
        if not (filename.startswith(f"{benchmark_type}_") and filename.endswith(".txt")):
            continue

        metadata = extract_metadata_from_filename(filename)
        if not metadata:
            print(f"Warning: Could not parse metadata from filename: {filename}", file=sys.stderr)
            continue

        _, backend, op_type, _, batch_size = metadata

        filepath = os.path.join(directory, filename)
        parsed_data = parse_benchmark_file(filepath)
        if not parsed_data:
            print(f"Warning: No data parsed from file: {filepath}", file=sys.stderr)
            continue

        results[op_type]["batch_size"] = batch_size
        for entry in parsed_data:
            entry["Backend"] = backend  # Add backend info to each result
            results[op_type]["entries"].append(entry)

    return results


def parse_scenario_string(scenario_str: str) -> Scenario:
    """
    Parses a space-separated string into a Scenario object.

    Args:
        scenario_str: A string with 6 parts: name, database, relationships,
                      concurrency, cache enabled, iterations.

    Returns:
        A Scenario data object.
    """
    parts = scenario_str.split()
    if len(parts) != 6:
        raise ValueError(
            "Scenario string must contain exactly 6 parts: "
            "name, database, relationships, concurrency, cache, iterations."
        )
    name, db_size, rels, conc, cache, iters = parts
    return Scenario(
        name=name,
        database_size=db_size,
        relationships=rels,
        concurrency=int(conc),
        cache_enabled=cache.lower() == "enabled",
        iterations=int(iters),
    )


def prepare_results_for_db(results: ConsolidatedResults) -> list[ResultRow]:
    """
    Converts the consolidated results into a list of ResultRow objects
    for database insertion.

    Args:
        results: The consolidated benchmark data.

    Returns:
        A list of ResultRow objects.
    """
    db_rows = []
    for _, data in results.items():
        batch_size = int(data["batch_size"])
        for entry in data["entries"]:
            try:
                row = ResultRow(
                    backend=entry["Backend"],
                    operation=entry["Operation"],
                    batch_size=batch_size,
                    median_ms=float(entry["MED"]),
                    min_ms=float(entry["MIN"]),
                    max_ms=float(entry["MAX"]),
                    p50_ms=float(entry["P50"]),
                    p90_ms=float(entry["P90"]),
                    p99_ms=float(entry["P99"]),
                    total_time_ms=float(entry["TT"]),
                    time_per_atom_ms=float(entry["TPA"]),
                    throughput=float(entry["TP"]),
                )
                db_rows.append(row)
            except (KeyError, ValueError) as e:
                print(
                    f"Warning: Skipping row due to missing/invalid data: {entry}. Error: {e}",
                    file=sys.stderr,
                )
    return db_rows


def main():
    parser = argparse.ArgumentParser(description="Consolidate and store benchmark results.")
    parser.add_argument("directory", help="Directory with benchmark files.")
    parser.add_argument("--scenario", required=True, help="Test scenario data string.")
    parser.add_argument("--type", required=True, help="Benchmark type (e.g., 'atomdb').")
    parser.add_argument(
        "--output-dir", required=False, default=".", help="Directory to save database file."
    )
    args = parser.parse_args()

    # Consolidate benchmark results from files
    print(f"Consolidating results from: {args.directory}")
    results = consolidate_results_from_directory(args.type, args.directory)
    if not results:
        print("No valid benchmark data found. Exiting.")
        return

    # Prepare database and scenario data
    base_dir = os.path.abspath(args.output_dir)
    if not os.path.exists(base_dir):
        print(f"Creating output directory: {base_dir}")
        os.makedirs(base_dir)
    db_name = f"{base_dir}/{args.type}_benchmark.db"
    print(f"Using database: {db_name}")
    db = DatabaseManager(db_name)
    db.create_tables()

    try:
        scenario = parse_scenario_string(args.scenario)
    except ValueError as e:
        print(f"Error: Invalid scenario string. {e}", file=sys.stderr)
        sys.exit(1)

    # Get or create IDs for the current run
    type_id, scenario_id = db.get_or_create_ids(args.type, scenario)

    # Prepare result data for insertion
    results_for_db = prepare_results_for_db(results)
    if not results_for_db:
        print("No valid results to insert into the database. Exiting.")
        return

    # Insert data into the database
    try:
        execution_id = db.insert_results(type_id, scenario_id, results_for_db)
        print(f"Successfully loaded {len(results_for_db)} results into the database.")
        print(f"New execution ID: {execution_id}")
    except sqlite3.Error as e:
        print(f"Failed to insert data into the database. Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
