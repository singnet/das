import argparse
import os
import sqlite3
import json
import requests
from datetime import datetime, timezone
from statistics import mean
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

TITLE_ALIAS_MAP = {
    "morkdb": "MorkDB",
    "redismongodb": "Redis + MongoDB",
    "queryagent": "Query Agent",
}


def run_sqlite_query(db_path, query, params=None):
    with sqlite3.connect(db_path) as conn:
        cursor = conn.cursor()
        cursor.execute(query, params or [])
        return cursor.fetchall()


def get_pr_info(repo, sha, token):
    url = f"https://api.github.com/repos/{repo}/commits/{sha}/pulls"
    headers = {"Authorization": f"token {token}", "Accept": "application/vnd.github.groot-preview+json"}
    r = requests.get(url, headers=headers, timeout=30)
    r.raise_for_status()
    return r.json()


def get_commit_info(repo, sha, token):
    url = f"https://api.github.com/repos/{repo}/commits/{sha}"
    headers = {"Authorization": f"token {token}"}
    r = requests.get(url, headers=headers, timeout=30)
    r.raise_for_status()
    return r.json()


def format_title(prefix):
    return TITLE_ALIAS_MAP.get(prefix, prefix.replace("_", " ").title())


def get_prefixes(results):
    return sorted({r["backend"] for r in results})


def get_history_for_operation(db_path, backend, operation, benchmark_type_id, window=10):
    query = """
    SELECT r.median_operation_time_ms,
           r.throughput,
           r.time_per_atom_ms,
           r.benchmark_execution_id
    FROM benchmark_result r
    JOIN benchmark_execution e ON e.id = r.benchmark_execution_id
    WHERE r.backend = ?
      AND r.operation = ?
      AND e.benchmark_type_id = ?
      AND e.status = 'COMPLETED'
    ORDER BY r.benchmark_execution_id DESC
    LIMIT ?
    """
    rows = run_sqlite_query(db_path, query, (backend, operation, benchmark_type_id, window))
    return rows[::-1]


def get_benchmark_result_by_id(db_path, benchmark_id):
    query = """
        SELECT r.id, r.benchmark_execution_id, r.backend, r.operation, r.batch_size,
               r.median_operation_time_ms, r.min_operation_time_ms, r.max_operation_time_ms,
               r.p50_operation_time_ms, r.p90_operation_time_ms, r.p99_operation_time_ms,
               r.total_time_ms, r.time_per_atom_ms, r.throughput,
               e.benchmark_type_id
        FROM benchmark_result r
        JOIN benchmark_execution e ON e.id = r.benchmark_execution_id
        WHERE r.benchmark_execution_id = ?;
    """
    rows = run_sqlite_query(db_path, query, (benchmark_id,))
    col_names = [d[1] for d in sqlite3.connect(db_path).execute("PRAGMA table_info(benchmark_result)").fetchall()]
    col_names.append("benchmark_type_id")
    return [dict(zip(col_names, row)) for row in rows]



def build_metadata_section(repo, sha, token, title):
    pr_info = get_pr_info(repo, sha, token)
    commit_info = get_commit_info(repo, sha, token)

    pr_url = pr_info[0]["html_url"] if pr_info else ""
    pr_number = pr_info[0]["number"] if pr_info else ""
    pr_title = pr_info[0]["title"] if pr_info else ""
    pr_base_branch = pr_info[0].get("base", {}).get("ref", "master") if pr_info else "master"

    commit_date = commit_info["commit"]["author"]["date"]
    commit_date_fmt = datetime.strptime(commit_date, "%Y-%m-%dT%H:%M:%SZ").strftime("%Y-%m-%d %H:%M UTC")
    commit_sha_short = sha[:7]

    md = [f"## {title}\n"]
    md.append(f"**Repository:** {repo}")
    if pr_url:
        md.append(f"**Source:** [#{pr_number} - {pr_title}]({pr_url})")
    md.append(f"**Date:** {commit_date_fmt}")

    commit_line = f"**Commit:** `{commit_sha_short}`"
    if pr_url:
        commit_line += f" ({pr_base_branch})"
    md.append(commit_line)

    return "\n".join(md)


def generate_chart_for_operation(db_path, backend, operation, benchmark_type_id, window=10, output_dir="charts"):
    history = get_history_for_operation(db_path, backend, operation, benchmark_type_id, window)
    if not history:
        return None

    medians = [h[0] for h in history]
    x = list(range(1, len(medians) + 1))  

    ma = pd.Series(medians).rolling(window=window, min_periods=1).mean()

    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)

    plt.figure(figsize=(4, 2), dpi=100)

    plt.plot(x, medians, marker="o", alpha=0.5, label="Median", linewidth=1)
    plt.plot(x, ma, linewidth=1.5, label=f"{window}-exec MA")
    plt.scatter(x[-1], medians[-1], color="red", s=20, zorder=5, label="Latest")

    plt.title(operation, fontsize=8)
    plt.tick_params(axis="both", which="major", labelsize=6)
    plt.grid(True, linewidth=0.3)

    plt.legend(fontsize=6, loc="upper center", bbox_to_anchor=(0.5, -0.2), ncol=3, frameon=False)

    img_name = f"{backend}_{operation}_type{benchmark_type_id}.png"
    img_path = output_path / img_name
    plt.tight_layout(pad=0.3)
    plt.savefig(img_path, bbox_inches="tight")
    plt.close()

    return str(img_path)



def build_table_for_prefix(db_path, results, prefix, benchmark_type_id, window=10, threshold=10):
    md = [f"\n\n### {format_title(prefix)}\n"]
    md.append("| Operation | Median | Δ Median | Time Per Atom | Δ TPA | Throughput | Δ TP | Chart |")
    md.append("|-----------|--------|----------|---------------|-------|------------|------|-------|")

    for row in (r for r in results if r["backend"] == prefix and r["benchmark_type_id"] == benchmark_type_id):
        op = row["operation"]
        median_val = row["median_operation_time_ms"]
        tpa_val = row["time_per_atom_ms"]
        tp_val = row["throughput"]

        delta_median = delta_tpa = delta_tp = "N/A"

        history = get_history_for_operation(db_path, prefix, op, benchmark_type_id, window)
        if history:
            hist_median_avg = mean(h[0] for h in history)
            hist_tpa_avg = mean(h[2] for h in history)
            hist_tp_avg = mean(h[1] for h in history)

            if hist_median_avg > 0:
                delta_median_val = ((median_val - hist_median_avg) / hist_median_avg) * 100
                delta_median = f"{delta_median_val:+.1f}%"
            if hist_tpa_avg > 0:
                delta_tpa_val = ((tpa_val - hist_tpa_avg) / hist_tpa_avg) * 100
                delta_tpa = f"{delta_tpa_val:+.1f}%"
            if hist_tp_avg > 0:
                delta_tp_val = ((tp_val - hist_tp_avg) / hist_tp_avg) * 100
                delta_tp = f"{delta_tp_val:+.1f}%"

            if median_val > hist_median_avg * (1 + threshold / 100):
                median_val = f":red_circle: {median_val}"
            if tpa_val > hist_tpa_avg * (1 + threshold / 100):
                tpa_val = f":red_circle: {tpa_val}"
            if tp_val < hist_tp_avg * (1 - threshold / 100):
                tp_val = f":red_circle: {tp_val}"

        chart_path = ".github/scripts/" + generate_chart_for_operation(db_path, prefix, op, benchmark_type_id, window)
        chart_md = f"![{op}]({chart_path})" if chart_path else "N/A"

        md.append(f"| {op} | {median_val} | {delta_median} | {tpa_val} | {delta_tpa} | {tp_val} | {delta_tp} | {chart_md} |")

    return "\n".join(md)



def generate_message(db_path, results, repo, sha, token, title, window=10, threshold=10):
    message = build_metadata_section(repo, sha, token, title)
    prefixes = get_prefixes(results)
    for prefix in prefixes:
        type_ids = sorted({r["benchmark_type_id"] for r in results if r["backend"] == prefix})
        for type_id in type_ids:
            message += build_table_for_prefix(db_path, results, prefix, type_id, window, threshold)
    return message


def update_benchmark_execution(db_path, benchmark_id, repo, sha, token):
    pr_info = get_pr_info(repo, sha, token)
    pr_link = pr_info[0]["html_url"] if pr_info else ""
    pr_title = pr_info[0]["title"] if pr_info else ""
    completed_at = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    with sqlite3.connect(db_path) as conn:
        conn.execute(
            """
            UPDATE benchmark_execution
            SET status = 'COMPLETED',
                pr_link = ?,
                pr_title = ?,
                completed_at = ?
            WHERE id = ?;
            """,
            (pr_link, pr_title, completed_at, benchmark_id),
        )
        conn.commit()


def save_message_as_json(msg, output_file="mattermost.json"):
    with open(output_file, "w", encoding="utf-8") as f:
        json.dump({"text": msg}, f, ensure_ascii=False, indent=2)


def main():
    parser = argparse.ArgumentParser(description="Generate benchmark report")
    parser.add_argument("--benchmark-id", required=True, type=int, help="ID of the benchmark execution")
    parser.add_argument("--title", required=True, help="Report title")
    parser.add_argument("--db-path", default=f"/home/{os.getenv('USER')}/.cache/shared/benchmark.db", help="Path to SQLite DB")
    parser.add_argument("--repo", default=os.getenv("GITHUB_REPOSITORY", ""), help="GitHub repository")
    parser.add_argument("--sha", default=os.getenv("GITHUB_SHA", ""), help="Git commit SHA")
    parser.add_argument("--token", default=os.getenv("GITHUB_TOKEN", ""), help="GitHub token")
    parser.add_argument("--window", type=int, default=10, help="History window size")
    parser.add_argument("--threshold", type=int, default=10, help="Deviation threshold in percent")
    parser.add_argument("--output", default="mattermost.json", help="Output JSON file")
    args = parser.parse_args()

    results = get_benchmark_result_by_id(args.db_path, args.benchmark_id)
    if not results:
        print("Benchmark result could not be found in the database")
        return

    message = generate_message(args.db_path, results, args.repo, args.sha, args.token, args.title, args.window, args.threshold)
    print(message)
    save_message_as_json(message, args.output)
    update_benchmark_execution(args.db_path, args.benchmark_id, args.repo, args.sha, args.token)


if __name__ == "__main__":
    main()