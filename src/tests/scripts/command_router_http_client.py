#!/usr/bin/env python3
"""
Simple test client for CommandRouterHttpAPI.

Usage:
  pip install requests websocket-client
  python command_router_http_client.py
  python command_router_http_client.py --host localhost --port 40009
"""

import argparse
import json
import sys
import time

import requests
import websocket


def wait_for_status(base_url: str, execution_id: str, target: str, timeout: float = 10.0) -> dict:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        response = requests.get(f"{base_url}/command-router/executions/{execution_id}", timeout=10)
        body = response.json()
        if body.get("status") == target:
            return body
        time.sleep(0.1)
    raise TimeoutError(f"status never became {target!r}")


def read_ws_events(ws, stop_when, timeout: float = 30.0) -> list[dict]:
    events = []
    deadline = time.monotonic() + timeout
    ws.settimeout(1.0)

    while time.monotonic() < deadline:
        try:
            message = ws.recv()
        except websocket.WebSocketTimeoutException:
            continue
        except websocket.WebSocketConnectionClosedException:
            break

        if not message:
            continue

        event = json.loads(message)
        events.append(event)
        print(f"  WS -> {json.dumps(event)}")

        if stop_when(event, events):
            break

    return events


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=40009)
    args = parser.parse_args()

    base_url = f"http://{args.host}:{args.port}"
    ws_base = f"ws://{args.host}:{args.port}"

    print(f"Testing {base_url}\n")

    # GET /ping
    response = requests.get(f"{base_url}/ping", timeout=10)
    print(f"GET /ping -> {response.status_code} {response.text!r}")
    assert response.status_code == 200 and response.text == "PONG!", "ping failed"

    # POST /command-router/executions
    response = requests.post(
        f"{base_url}/command-router/executions",
        json={"command_type": "query", "command_text": '(Similarity "human" %V)'},
        timeout=10,
    )
    print(f"POST /command-router/executions -> {response.status_code} {response.text}")
    assert response.status_code == 202, "create execution failed"
    execution_id = response.json()["execution_id"]

    # WS /command-router/ws/{id} — stream events before cancel
    print(f"WS /command-router/ws/{execution_id}")
    ws = websocket.create_connection(
        f"{ws_base}/command-router/ws/{execution_id}",
        timeout=10,
    )

    saw_running = False
    saw_chunk = False

    def before_cancel(event: dict, _events: list[dict]) -> bool:
        nonlocal saw_running, saw_chunk
        if event.get("status") == "running":
            saw_running = True
        if event.get("type") == "chunk":
            saw_chunk = True
        return saw_running and saw_chunk

    read_ws_events(ws, before_cancel, timeout=30.0)
    assert saw_running, "websocket never received running event"
    assert saw_chunk, "websocket never received chunk event"

    # POST /command-router/executions/{id}/cancel
    response = requests.post(
        f"{base_url}/command-router/executions/{execution_id}/cancel",
        timeout=10,
    )
    print(
        f"POST /command-router/executions/{execution_id}/cancel "
        f"-> {response.status_code} {response.text}"
    )
    assert response.status_code == 200, "cancel failed"

    def until_aborted(event: dict, _events: list[dict]) -> bool:
        return event.get("status") == "aborted"

    aborted_events = read_ws_events(ws, until_aborted, timeout=30.0)
    assert any(event.get("status") == "aborted" for event in aborted_events), (
        "websocket never received aborted event"
    )
    ws.close()

    # GET /command-router/executions/{id}
    status = wait_for_status(base_url, execution_id, "aborted")
    print(f"GET /command-router/executions/{execution_id} -> {status['status']}")

    # GET unknown execution -> 404
    response = requests.get(
        f"{base_url}/command-router/executions/exec-unknown",
        timeout=10,
    )
    print(f"GET /command-router/executions/exec-unknown -> {response.status_code} {response.text}")
    assert response.status_code == 404, "expected 404 for unknown execution"

    print("\nAll endpoints OK")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print(f"\nFAILED: {exc}", file=sys.stderr)
        sys.exit(1)
