import os
import re
from collections import defaultdict
import sqlite3


DEFAULT_COL_ORDER = [
    "Backend",
    "Operation",
    "AVG",
    "MIN",
    "MAX",
    "P50",
    "P90",
    "P99",
    "TT",
    "TPA",
    "TP",
]


def parse_benchmark_file(filepath):
    results = []
    with open(filepath, "r") as f:
        lines = f.readlines()

    header = None
    for line in lines:
        if "|" in line and "Operation" in line:
            header = [h.strip() for h in line.split("|") if h.strip()]
            break

    if not header:
        return results

    for line in lines:
        if "|" in line and not set(line.strip()).issubset(set("-| ")):
            values = [v.strip() for v in line.split("|") if v.strip()]
            if len(values) == len(header) and values[0] != "Operation":
                entry = dict(zip(header, values))
                results.append(entry)
    return results


def extract_backend_operation_type_and_batch_size(filename):
    m = re.match(r"atomdb_([A-Za-z0-9]+)_([A-Za-z0-9]+)_([A-Za-z0-9_]+)_([0-9]+)\.txt", filename)
    if m:
        backend, op_type, method, batch_size = m.group(1), m.group(2), m.group(3), m.group(4)
        return backend, op_type, method, batch_size
    return None, None, None, None


def consolidate_results(directory):
    results = defaultdict(list)
    for fname in sorted(os.listdir(directory)):
        if fname.startswith("atomdb_") and fname.endswith(".txt"):
            backend, op_type, _, batch_size = extract_backend_operation_type_and_batch_size(fname)
            if backend and op_type:
                filepath = os.path.join(directory, fname)
                data = parse_benchmark_file(filepath)
                if data:
                    if batch_size not in results[op_type]:
                        results[op_type].append(batch_size)
                for entry in data:
                    entry["Backend"] = backend
                    results[op_type].append(entry)
    return results


# ---> Write report in a file
def get_columns(entries):
    cols = set()
    for entry in entries:
        cols.update(entry.keys())
    return [col for col in DEFAULT_COL_ORDER if col in cols]


def calc_col_widths(entries, columns):
    widths = {}
    for col in columns:
        max_len = len(col)
        for entry in entries:
            max_len = max(max_len, len(str(entry.get(col, ""))))
        widths[col] = max_len
    return widths


def write_report(header, results, output_file):
    with open(output_file, "w") as f:
        f.write(header + "\n\n" if header else "")

        for op_type, entries in results.items():
            batch_size = entries.pop(0)
            f.write(f"[{op_type}] - Batch Size: {batch_size}\n\n")
            columns = get_columns(entries)
            col_widths = calc_col_widths(entries, columns)

            header = "  ".join(col.ljust(col_widths[col]) for col in columns)
            f.write(header + "\n")

            sep = "  ".join("-" * col_widths[col] for col in columns)
            f.write(sep + "\n")

            for entry in entries:
                row = "  ".join(str(entry.get(col, "")).ljust(col_widths[col]) for col in columns)
                f.write(row + "\n")
            f.write("\n")
# <---


# ---> Save report in a database
def create_tables(database):
    conn = sqlite3.connect(database)
    cursor = conn.cursor()

    table_name = "benchmark_type"
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name=?", (table_name,))
    if not cursor.fetchone():
        cursor.execute("""
            CREATE TABLE benchmark_type (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL);
        """)
        conn.commit()

    table_name = "benchmark_scenario"
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name=?", (table_name,))
    if not cursor.fetchone():
        cursor.execute("""
            CREATE TABLE benchmark_scenario (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                scenario_name TEXT,
                database_size TEXT,
                atoms_relationships TEXT,
                concurrent_access INTEGER,
                cache_enabled BOOLEAN,
                iterations INTEGER);
        """)
        conn.commit()

    table_name = "benchmark_result"
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name=?", (table_name,))
    if not cursor.fetchone():
        cursor.execute("""
            CREATE TABLE benchmark_result (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                benchmark_scenario_id INTEGER NOT NULL,
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
                FOREIGN KEY (benchmark_scenario_id) REFERENCES benchmark_scenario(id));
        """)
        conn.commit()

    table_name = "benchmark_execution"
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name=?", (table_name,))
    if not cursor.fetchone():
        cursor.execute("""
            CREATE TABLE benchmark_execution (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                benchmark_type_id INTEGER NOT NULL,
                benchmark_scenario_id INTEGER NOT NULL,
                execution_timestamp TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                execution_type TEXT NOT NULL CHECK (execution_type IN ('PR', 'SCHEDULED')),
                pr_link TEXT,
                pr_title TEXT,
                FOREIGN KEY (benchmark_type_id) REFERENCES benchmark_type(id),
                FOREIGN KEY (benchmark_scenario_id) REFERENCES benchmark_scenario(id));
        """)
        conn.commit()

    conn.close()


def parse_benchmark_scenario(scenario_str: str) -> tuple:
    scenario_parts = scenario_str.split()
    if len(scenario_parts) != 6:
        raise ValueError("Scenario must contain exactly 6 parts: name, database, relationships, concurrency, cache, iterations.")

    name, db, rel, concurrency, cache, iterations = scenario_parts

    return (name, db, rel, int(concurrency), cache.lower() == "enabled", int(iterations))


def insert_test_scenario_data(type: str, scenario: tuple[str]) -> int:
    conn = sqlite3.connect("atomdb_benchmark.db")
    cursor = conn.cursor()

    cursor.execute("SELECT 1 FROM benchmark_type WHERE name = ?", (type[0],))
    exists = cursor.fetchone()

    if not exists:
        cursor.execute(
            """
            INSERT INTO benchmark_type (name)
            VALUES (?);
        """,
            (type,),
        )
        conn.commit()

    cursor.execute("SELECT id FROM benchmark_scenario WHERE scenario_name = ?", (scenario[0],))
    exists = cursor.fetchone()

    if not exists:
        cursor.execute(
            """
            INSERT INTO benchmark_scenario (scenario_name, database_size, atoms_relationships, concurrent_access, cache_enabled, iterations)
            VALUES (?, ?, ?, ?, ?, ?);
        """,
            (scenario[0], scenario[1], scenario[2], scenario[3], scenario[4], scenario[5]),
        )
        conn.commit()
        last_row_id = cursor.lastrowid
    else:
        last_row_id = exists[0]

    conn.close()

    return last_row_id


def insert_results_data(scenario_id: int, results_data: list[tuple[str]]):
    conn = sqlite3.connect("atomdb_benchmark.db")
    cursor = conn.cursor()
    for result in results_data:
        if len(result) != 12:
            raise ValueError("Each result must contain exactly 12 fields.")
        cursor.execute(
            """
            INSERT INTO benchmark_result (benchmark_scenario_id, backend, operation, batch_size, median_operation_time_ms, min_operation_time_ms, max_operation_time_ms, p50_operation_time_ms, p90_operation_time_ms, p99_operation_time_ms, total_time_ms, time_per_atom_ms, throughput)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
        """,
            (scenario_id, *result),
        )
    conn.commit()
    conn.close()
# <---


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Consolidate benchmarks.")
    parser.add_argument("directory", help="Directory with benchmark files")
    parser.add_argument("--scenario", help="Test scenario data")
    parser.add_argument("--type", help="Benchmark type")
    args = parser.parse_args()

    results = consolidate_results("/tmp/atomdb_benchmark/20250731085453/")

    create_tables(database="atomdb_benchmark.db")

    scenario = parse_benchmark_scenario(args.scenario)
    scenario_id = insert_test_scenario_data(args.type, scenario)

    results_data: list[tuple[str]] = []
    for op_type, entries in results.items():
        batch_size = entries.pop(0)
        for entry in entries:
            backend = entry["Backend"]
            operation = entry["Operation"]
            median_operation_time_ms = entry["MED"]
            min_operation_time_ms = entry["MIN"]
            max_operation_time_ms = entry["MAX"]
            p50_operation_time_ms = entry["P50"]
            p90_operation_time_ms = entry["P90"]
            p99_operation_time_ms = entry["P99"]
            total_time_ms = entry["TT"]
            time_per_atom_ms = entry["TPA"]
            throughput = entry["TP"]
            results_data.append(
                (
                    backend,
                    operation,
                    batch_size,
                    median_operation_time_ms,
                    min_operation_time_ms,
                    max_operation_time_ms,
                    p50_operation_time_ms,
                    p90_operation_time_ms,
                    p99_operation_time_ms,
                    total_time_ms,
                    time_per_atom_ms,
                    throughput,
                )
            )

    insert_results_data(scenario_id=scenario_id, results_data=results_data)
