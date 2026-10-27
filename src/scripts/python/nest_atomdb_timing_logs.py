#!/usr/bin/env python3
"""Rebuild DAS query_for_pattern DEBUG logs as a top-down timing tree.

Logs are written after inner work finishes (post-order). This script nests
those lines under the enclosing operation, using `| time: Xms` / `| total: Xms`
and the other `| key: Nms` fields.

The default root is:
  RemoteAtomDB.cc query_for_pattern | [peerN] returned ... | time: Xms

Usage:
  python3 nest_atomdb_timing_logs.py QE.log
  python3 nest_atomdb_timing_logs.py < QE.log
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from datetime import datetime, timedelta
from typing import Optional


LINE_RE = re.compile(
    r"^(?P<ts>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}) \| (?P<level>\w+) \| "
    r"(?P<pid>\d+) (?P<file>[^:]+):(?P<line>\d+) (?P<func>\S+) \| (?P<msg>.*)$"
)
MS_RE = re.compile(r"(?P<key>[A-Za-z0-9_]+):\s*(?P<ms>\d+)ms")
HANDLE_RE = re.compile(r"query_for_pattern\(([0-9a-fA-F]+)\)")
PEER_RETURN_RE = re.compile(r"\[([^\]]+)\] returned .*\| time:\s*(\d+)ms")


@dataclass
class Event:
    seq: int
    ts: datetime
    function: str
    source: str
    message: str
    duration_ms: Optional[int]
    fields: dict[str, int]
    children: list["Event"] = field(default_factory=list)
    assigned: bool = False

    @property
    def start_ts(self) -> datetime:
        if self.duration_ms:
            return self.ts - timedelta(milliseconds=self.duration_ms)
        return self.ts

    @property
    def label(self) -> str:
        return f"{self.function} | {self.message}"


def parse_fields(message: str) -> dict[str, int]:
    return {m.group("key"): int(m.group("ms")) for m in MS_RE.finditer(message)}


STAGE_SUM_KEYS = (
    "remote",
    "feed_cache",
    "merge_remote",
    "merge_memory",
    "local_persistence",
)
BREAKDOWN_KEYS = {
    "backend",
    "next",
    "can_read",
    "add_handle",
    "leftover",
    "remote",
    "feed_cache",
    "merge_remote",
    "merge_memory",
    "local_persistence",
    "match",
    "_next",
    "other",
    "get_atom",
    "is_granted",
    "atom_exists",
    "is_unrestricted",
    "acquire",
    "find_one",
}


def parse_duration(fields: dict[str, int]) -> Optional[int]:
    if "time" in fields:
        return fields["time"]
    if "total" in fields:
        return fields["total"]
    if "filter_handle_set" in fields:
        return fields["filter_handle_set"]
    if "remote" in fields and "feed_cache" in fields:
        return sum(fields.get(key, 0) for key in STAGE_SUM_KEYS)
    return None


def parse_log(lines: list[str]) -> list[Event]:
    events: list[Event] = []
    for raw in lines:
        raw = raw.rstrip("\n")
        match = LINE_RE.match(raw)
        if not match:
            continue
        if match.group("level") != "DEBUG":
            continue
        message = match.group("msg").strip()
        fields = parse_fields(message)
        events.append(
            Event(
                seq=len(events),
                ts=datetime.strptime(match.group("ts"), "%Y-%m-%d %H:%M:%S"),
                function=match.group("func"),
                source=f"{match.group('file')}:{match.group('line')}",
                message=message,
                duration_ms=parse_duration(fields),
                fields=fields,
            )
        )
    return events


def contains(parent: Event, child: Event) -> bool:
    """Child logged before parent and its interval fits in the parent interval."""
    if child.seq >= parent.seq or not parent.duration_ms:
        return False
    slack = timedelta(milliseconds=max(parent.duration_ms * 0.02, 500))
    return child.ts <= parent.ts + timedelta(seconds=1) and child.start_ts >= parent.start_ts - slack


def attach_field_children(event: Event) -> None:
    """Turn `| next: 12ms | can_read: 10ms` into synthetic nested rows."""
    extras = [
        (key, ms)
        for key, ms in event.fields.items()
        if key in BREAKDOWN_KEYS and ms > 0
    ]
    if not extras:
        return
    existing = {child.message for child in event.children}
    for key, ms in extras:
        label = f"{key}: {ms}ms"
        if label in existing:
            continue
        event.children.append(
            Event(
                seq=event.seq,
                ts=event.ts,
                function=event.function,
                source=event.source,
                message=label,
                duration_ms=ms,
                fields={key: ms},
            )
        )


def nest(events: list[Event]) -> list[Event]:
    """Later completion lines parent earlier lines they enclose."""
    for parent in events:
        if parent.duration_ms is None:
            continue
        for child in events:
            if child is parent or child.assigned:
                continue
            if contains(parent, child):
                # Prefer the tightest later parent: skip if a smaller later
                # duration event also contains this child.
                tighter = False
                for other in events:
                    if other is parent or other is child or other.duration_ms is None:
                        continue
                    if other.seq > child.seq and other.seq < parent.seq and contains(other, child):
                        if other.duration_ms <= parent.duration_ms:
                            tighter = True
                            break
                if tighter:
                    continue
                child.assigned = True
                parent.children.append(child)
        attach_field_children(parent)
        rehome_details(parent)

    for event in events:
        if event.duration_ms is None:
            attach_field_children(event)

    return [event for event in events if not event.assigned]


def field_child(event: Event, key: str) -> Optional[Event]:
    prefix = f"{key}:"
    for child in event.children:
        if child.message.startswith(prefix):
            return child
    return None


def rehome_details(event: Event) -> None:
    """Put iterator/can_read detail logs under the exclusive field they explain."""
    for child in event.children:
        rehome_details(child)

    kept: list[Event] = []
    for child in event.children:
        target = None
        if "next exhausted" in child.message:
            target = field_child(event, "next")
        elif "can_read breakdown" in child.message:
            target = field_child(event, "can_read")
        if target is not None and target is not child:
            target.children.append(child)
        else:
            kept.append(child)
    event.children = kept


def format_event(event: Event, indent: int = 0) -> list[str]:
    pad = "  " * indent
    duration = f"{event.duration_ms}ms" if event.duration_ms is not None else ""
    line = f"{pad}{event.function}: {event.message}"
    if duration and f"{event.duration_ms}ms" not in event.message:
        line = f"{line}  [{duration}]"
    lines = [line]
    for child in event.children:
        lines.extend(format_event(child, indent + 1))
    return lines


def query_handle(message: str) -> Optional[str]:
    match = HANDLE_RE.search(message)
    return match.group(1) if match else None


def select_roots(events: list[Event], handle: Optional[str]) -> list[Event]:
    roots = []
    for event in events:
        if PEER_RETURN_RE.search(event.message):
            if handle and query_handle(event.message) not in (None, handle):
                # peer-return line has the handle in query_for_pattern(...) on
                # earlier lines; the return line itself may omit it.
                pass
            roots.append(event)
    if handle:
        # Keep trees whose subtree mentions the handle.
        filtered = []
        for root in roots:
            blob = root.message + " " + " ".join(child.message for child in walk(root))
            if handle in blob or query_handle(root.message) == handle:
                filtered.append(root)
        if filtered:
            return filtered
    return roots


def walk(event: Event) -> list[Event]:
    out = []
    for child in event.children:
        out.append(child)
        out.extend(walk(child))
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logfile", nargs="?", help="DEBUG log file (default: stdin)")
    parser.add_argument("--handle", help="Only trees for this query_for_pattern handle")
    args = parser.parse_args()

    raw = open(args.logfile, encoding="utf-8", errors="replace") if args.logfile else sys.stdin
    try:
        events = parse_log(raw.readlines())
    finally:
        if args.logfile:
            raw.close()

    if not events:
        print("No DEBUG log lines matched.", file=sys.stderr)
        return 1

    trees = nest(events)
    roots = select_roots(trees, args.handle)
    if not roots:
        roots = trees

    for i, root in enumerate(roots):
        if i:
            print()
        print("=" * 80)
        print("\n".join(format_event(root)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
