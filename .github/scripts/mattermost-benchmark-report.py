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
import subprocess
import tempfile
import html as html_lib

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
    headers = {
        "Authorization": f"token {token}",
        "Accept": "application/vnd.github.groot-preview+json",
    }
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


def get_history_for_operation(
    db_path, backend, operation, benchmark_type_id, window=10
):
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
    rows = run_sqlite_query(
        db_path, query, (backend, operation, benchmark_type_id, window)
    )
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
    col_names = [
        d[1]
        for d in sqlite3.connect(db_path)
        .execute("PRAGMA table_info(benchmark_result)")
        .fetchall()
    ]
    col_names.append("benchmark_type_id")
    return [dict(zip(col_names, row)) for row in rows]


def build_metadata_section(repo, sha, token, title, details=""):
    commit_info = get_commit_info(repo, sha, token)
    commit_iso_date = commit_info["commit"]["committer"]["date"]
    commit_date = datetime.fromisoformat(
        commit_iso_date.replace("Z", "+00:00")
    ).strftime("%Y-%m-%d %H:%M")

    commit_sha_short = sha[:7]

    pr_info = get_pr_info(repo, sha, token)

    if len(pr_info) > 0:
        pr_base_branch = pr_info[0].get("base", {}).get("ref", "master")
        pr_url = pr_info[0].get("html_url")
        pr_number = pr_info[0].get("number")
        pr_title = pr_info[0].get("title")
        return {
            "title": title,
            "details": details,
            "repo": repo,
            "commit_sha_short": commit_sha_short,
            "date": commit_date,
            "pr_url": pr_url,
            "pr_number": pr_number,
            "pr_title": pr_title,
            "pr_base_branch": pr_base_branch,
        }
    else:
        return {
            "title": title,
            "details": details,
            "repo": repo,
            "commit_sha_short": commit_sha_short,
            "date": commit_date,
            "pr_url": None,
            "pr_number": None,
            "pr_title": None,
            "pr_base_branch": None,
        }


def generate_chart_for_operation(
    db_path,
    backend,
    operation,
    benchmark_type_id,
    ssh,
    window=10,
    output_dir="charts",
    dry_run=False,
):
    history = get_history_for_operation(
        db_path,
        backend,
        operation,
        benchmark_type_id,
        window,
    )
    if not history:
        return None

    medians = [h[0] for h in history]
    x = list(range(1, len(medians) + 1))

    ma = pd.Series(medians).rolling(window=window, min_periods=1).mean()

    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)

    plt.figure(figsize=(3, 0.6), dpi=100)

    plt.plot(x, ma, linewidth=1)

    latest = medians[-1]
    plt.axhline(latest, linewidth=0.8)

    plt.yticks(fontsize=6)
    plt.xticks([])

    for spine in ["top", "right", "bottom"]:
        plt.gca().spines[spine].set_visible(False)

    img_name = f"{backend}_{operation}_type{benchmark_type_id}.png"
    img_path = output_path / img_name
    plt.tight_layout(pad=0.1)
    plt.savefig(img_path, bbox_inches="tight", pad_inches=0.05)
    plt.close()

    remote_path = upload_file_via_ssh(
        ssh["host"],
        ssh["key"],
        ssh["remote_dir"],
        img_path,
        ssh["public_url_base"],
        dry_run,
    )
    return remote_path


def build_table_for_prefix_html(
    db_path,
    results,
    prefix,
    benchmark_type_id,
    ssh,
    window=10,
    threshold=10,
    dry_run=False,
):
    rows_html = []
    title = format_title(prefix)
    rows_html.append(f"<h3>{html_lib.escape(title)}</h3>")
    rows_html.append(
        '<table class="bench">\n<thead><tr>\n<th>Operation</th><th>Median (ms)</th><th>Δ Median</th><th>Time Per Atom (ms)</th><th>Δ TPA</th><th>Throughput</th><th>Δ TP</th><th>Chart</th>\n</tr></thead>\n<tbody>'
    )

    for row in (
        r
        for r in results
        if r["backend"] == prefix and r["benchmark_type_id"] == benchmark_type_id
    ):
        op = row["operation"]
        median_val = row["median_operation_time_ms"]
        tpa_val = row["time_per_atom_ms"]
        tp_val = row["throughput"]

        delta_median = delta_tpa = delta_tp = "N/A"

        history = get_history_for_operation(
            db_path, prefix, op, benchmark_type_id, window
        )
        if history:
            hist_median_avg = mean(h[0] for h in history)
            hist_tpa_avg = mean(h[2] for h in history)
            hist_tp_avg = mean(h[1] for h in history)

            if hist_median_avg > 0:
                delta_median_val = (
                    (median_val - hist_median_avg) / hist_median_avg
                ) * 100
                delta_median = f"{delta_median_val:+.1f}%"
            if hist_tpa_avg > 0:
                delta_tpa_val = ((tpa_val - hist_tpa_avg) / hist_tpa_avg) * 100
                delta_tpa = f"{delta_tpa_val:+.1f}%"
            if hist_tp_avg > 0:
                delta_tp_val = ((tp_val - hist_tp_avg) / hist_tp_avg) * 100
                delta_tp = f"{delta_tp_val:+.1f}%"

            if median_val > hist_median_avg * (1 + threshold / 100):
                median_display = f'<span class="regress">{median_val}</span>'
            else:
                median_display = str(median_val)

            if tpa_val > hist_tpa_avg * (1 + threshold / 100):
                tpa_display = f'<span class="regress">{tpa_val}</span>'
            else:
                tpa_display = str(tpa_val)

            if tp_val < hist_tp_avg * (1 - threshold / 100):
                tp_display = f'<span class="regress">{tp_val}</span>'
            else:
                tp_display = str(tp_val)
        else:
            median_display = str(median_val)
            tpa_display = str(tpa_val)
            tp_display = str(tp_val)

        chart_url = generate_chart_for_operation(
            db_path,
            prefix,
            op,
            benchmark_type_id,
            ssh,
            window,
            "charts",
            dry_run,
        )
        chart_html = (
            f'<img src="{html_lib.escape(str(chart_url))}" alt="{html_lib.escape(str(op))}" class="thumb"/>'
            if chart_url
            else "N/A"
        )

        rows_html.append(
            f"<tr><td>{html_lib.escape(op)}</td><td>{median_display}</td><td>{delta_median}</td><td>{tpa_display}</td><td>{delta_tpa}</td><td>{tp_display}</td><td>{delta_tp}</td><td>{chart_html}</td></tr>"
        )

    rows_html.append("</tbody></table>")
    return "\n".join(rows_html)


def generate_html(
    db_path,
    results,
    repo,
    sha,
    token,
    title,
    ssh,
    report_type="daily",
    details="",
    window=10,
    threshold=10,
    dry_run=False,
):
    meta = build_metadata_section(repo, sha, token, title, details)

    prefixes = get_prefixes(results)

    body_chunks = []

    body_chunks.append(
        f"<h1>{html_lib.escape(report_type.capitalize())} report - {html_lib.escape(meta['title'])}</h1>"
    )

    body_chunks.append(
        f"<p><strong>Repository:</strong> {html_lib.escape(meta['repo'])}</p>"
    )

    if report_type == "merge" and meta.get("pr_url"):
        body_chunks.append(
            f"<p><strong>PR:</strong> "
            f"<a href=\"{html_lib.escape(meta['pr_url'])}\">"
            f"#{meta['pr_number']} - {html_lib.escape(meta['pr_title'])}</a></p>"
        )

    body_chunks.append(f"<p><strong>Date:</strong> {html_lib.escape(meta['date'])}</p>")
    body_chunks.append(
        f"<p><strong>Commit:</strong> "
        f"<code>{html_lib.escape(meta['commit_sha_short'])}</code> "
        f"({html_lib.escape(meta.get('pr_base_branch', 'master'))})</p>"
    )

    for prefix in prefixes:
        type_ids = sorted(
            {r["benchmark_type_id"] for r in results if r["backend"] == prefix}
        )
        for type_id in type_ids:
            body_chunks.append(
                build_table_for_prefix_html(
                    db_path,
                    results,
                    prefix,
                    type_id,
                    ssh,
                    window,
                    threshold,
                    dry_run,
                )
            )

    css = """
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial; font-size:14px; }
    .bench { border-collapse: collapse; width: 100%; margin-bottom: 20px; }
    .bench th, .bench td { border: 1px solid #ddd; padding: 6px; font-size:13px; }
    .bench th { background: #f7f7f7; text-align: left; }
    .thumb { height: 40px; }
    .regress { color: #c0392b; font-weight: 600; }
    """

    html_doc = (
        f'<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n'
        f'<meta name="viewport" content="width=device-width,initial-scale=1">\n'
        f"<title>{html_lib.escape(meta['title'])}</title>\n<style>{css}</style>\n</head>\n"
        f"<body>\n{''.join(body_chunks)}\n</body>\n</html>"
    )

    return html_doc


def generate_mattermost_message(
    repo,
    sha,
    token,
    title,
    details,
    report_url,
    report_type="daily",
):
    meta = build_metadata_section(repo, sha, token, title, details)

    header = f"## {report_type.capitalize()} report - {meta['title']}\n"

    pr_text = ""
    if report_type == "merge" and meta.get("pr_url"):
        pr_text = (
            f"**PR:** [#{meta['pr_number']} - {meta['pr_title']}]({meta['pr_url']})\n"
        )

    message_text = (
        f"{header}"
        f"{pr_text}"
        f"**Details:** {meta['details']}\n"
        f"**Date:** {meta['date']}\n"
        f"**Commit:** `{meta['commit_sha_short']}` ({meta.get('pr_base_branch', 'master')})\n"
        f"**Report:** {report_url}"
    )

    save_message_as_json(message_text)


def update_benchmark_execution(
    db_path,
    benchmark_id,
    repo,
    sha,
    token,
    dry_run=False,
):
    if dry_run:
        print("[dry-run] Skipping update of benchmark_execution in the database")
        return

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


def create_remote_dir(ssh_host, ssh_key, ssh_path):
    ts = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S")
    remote_dir = os.path.join(ssh_path, ts)
    subprocess.run(
        ["ssh", "-o", "StrictHostKeyChecking=no", "-i", ssh_key, ssh_host, "mkdir", "-p", remote_dir],
        check=True,
    )
    return remote_dir, ts


def build_public_url(ssh_host, ssh_path, ts):
    hostname = ssh_host.split("@")[-1]
    last_dir = os.path.basename(ssh_path.rstrip("/"))
    return f"http://{hostname}/{last_dir}/{ts}"


def upload_file_via_ssh(
    ssh_host,
    ssh_key,
    remote_dir,
    file_path,
    public_url_base=None,
    dry_run=False,
):
    remote_path = os.path.join(remote_dir, os.path.basename(file_path))

    if dry_run:
        print(f"[dry-run] Skipping upload of {file_path} to {ssh_host}:{remote_path}")
        if public_url_base:
            return f"{public_url_base}/{os.path.basename(file_path)}"
        return os.path.basename(file_path)

    subprocess.run(
        ["scp", "-o", "StrictHostKeyChecking=no", "-i", ssh_key, str(file_path), f"{ssh_host}:{remote_path}"],
        check=True,
    )

    if public_url_base:
        return f"{public_url_base}/{os.path.basename(file_path)}"
    return os.path.basename(file_path)


def save_html_and_upload(html_content, ssh, dry_run=False):
    tmp_dir = tempfile.gettempdir()
    tmp_path = os.path.join(tmp_dir, "index.html")

    with open(tmp_path, "w", encoding="utf-8") as f:
        f.write(html_content)

    remote_html = upload_file_via_ssh(
        ssh["host"],
        ssh["key"],
        ssh["remote_dir"],
        tmp_path,
        ssh["public_url_base"],
        dry_run,
    )

    os.unlink(tmp_path)

    return remote_html


def main():
    parser = argparse.ArgumentParser(description="Generate benchmark report (HTML)")
    parser.add_argument(
        "--benchmark-id",
        required=True,
        type=int,
        help="ID of the benchmark execution",
    )
    parser.add_argument("--title", required=True, help="Report title")
    parser.add_argument("--details", required=True, help="Report details")
    parser.add_argument(
        "--report-type",
        choices=["daily", "merge"],
        default="daily",
        help="Type of report: daily or merge",
    )
    parser.add_argument(
        "--db-path",
        default=f"/home/{os.getenv('USER')}/.cache/shared/benchmark.db",
        help="Path to SQLite DB",
    )
    parser.add_argument(
        "--repo",
        default=os.getenv("GITHUB_REPOSITORY", ""),
        help="GitHub repository",
    )
    parser.add_argument(
        "--sha",
        default=os.getenv("GITHUB_SHA", ""),
        help="Git commit SHA",
    )
    parser.add_argument(
        "--token",
        default=os.getenv("GITHUB_TOKEN", ""),
        help="GitHub token",
    )
    parser.add_argument("--window", type=int, default=10, help="History window size")
    parser.add_argument(
        "--threshold",
        type=int,
        default=10,
        help="Deviation threshold in percent",
    )
    parser.add_argument(
        "--output",
        default="index.html",
        help="Output HTML file name (local)",
    )
    parser.add_argument("--ssh-host", required=True, help="SSH host (ex: user@host)")
    parser.add_argument(
        "--ssh-path",
        required=True,
        help="Remote path (ex: /var/www/html/reports/)",
    )
    parser.add_argument(
        "--ssh-key",
        required=True,
        help="Path to private key (PEM file)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Do not upload files, just generate HTML locally",
    )
    args = parser.parse_args()

    ssh = {
        "host": args.ssh_host,
        "path": args.ssh_path,
        "key": args.ssh_key,
    }

    results = get_benchmark_result_by_id(args.db_path, args.benchmark_id)
    if not results:
        print("Benchmark result could not be found in the database")
        return

    remote_dir, ts = create_remote_dir(ssh["host"], ssh["key"], ssh["path"])
    public_url_base = build_public_url(ssh["host"], ssh["path"], ts)

    ssh["remote_dir"] = remote_dir
    ssh["public_url_base"] = public_url_base

    html = generate_html(
        args.db_path,
        results,
        args.repo,
        args.sha,
        args.token,
        args.title,
        ssh,
        args.report_type,
        args.details,
        args.window,
        args.threshold,
        args.dry_run,
    )

    local_out = Path(args.output)
    local_out.write_text(html, encoding="utf-8")

    remote_html_url = save_html_and_upload(html, ssh, dry_run=args.dry_run)

    print("Report generated and uploaded:", remote_html_url)

    generate_mattermost_message(
        args.repo,
        args.sha,
        args.token,
        args.title,
        args.details,
        remote_html_url,
        args.report_type,
    )

    update_benchmark_execution(
        args.db_path,
        args.benchmark_id,
        args.repo,
        args.sha,
        args.token,
        args.dry_run,
    )


if __name__ == "__main__":
    main()
