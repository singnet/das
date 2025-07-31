import os
import re
from collections import defaultdict

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
            backend, op_type, method, batch_size = extract_backend_operation_type_and_batch_size(
                fname
            )
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


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Consolidate benchmarks.")
    parser.add_argument("directory", help="Directory with benchmark files")
    parser.add_argument("-o", "--output", default="consolidated_report.txt", help="Output file")
    parser.add_argument("--header", default="", help="Header to include in the report")
    args = parser.parse_args()

    results = consolidate_results(args.directory)
    write_report(args.header, results, args.output)
