import argparse
from dataclasses import dataclass
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import threading
import time
from urllib.parse import quote

import requests

MORK_SERVER_ADDRESS = f"http://{os.getenv('MORK_SERVER_ADDR')}:{os.getenv('MORK_SERVER_PORT')}"


class FileServer:
    """Start a simple HTTP server in a background thread to serve a directory."""

    def __init__(self, dir: str = "/tmp", bind: str = "0.0.0.0", port_range=(38800, 38900)):
        os.chdir(dir)
        handler = SimpleHTTPRequestHandler
        self.port = None
        self.server = None
        for port in range(port_range[0], port_range[1] + 1):
            try:
                self.server = ThreadingHTTPServer((bind, port), handler)
                self.port = port
                break
            except OSError as e:
                if e.errno == 98:  # Address already in use
                    continue
                else:
                    raise
        if self.server is None:
            raise RuntimeError(f"No free port found in range {port_range[0]}-{port_range[1]}")
        thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        thread.start()
        print(f"Serving {dir} at http://{bind}:{self.port}/")

    def __enter__(self) -> "FileServer":
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        self.server.shutdown()
        self.server.server_close()
        print("Static server stop.")

    def get_port(self):
        return self.port


@dataclass
class MorkClient:
    """
    Simplified client for interacting with a MORK server.
    """

    base_url: str
    session: requests.Session = requests.Session()

    def _poll(
        self,
        status_loc: str,
        max_attempts: int = 16,
        initial_delay: float = 0.005,
        backoff: float = 2.0,
    ) -> dict:
        """
        Polls the server until the requested path is clear or raises after timeout.
        """
        for attempt in range(max_attempts):
            resp = self.session.get(f"{self.base_url}/status/{quote(status_loc)}")
            resp.raise_for_status()
            info = resp.json()
            status = info.get("status")

            if status == "pathClear":
                return info
            if status == "pathForbiddenTemporary":
                time.sleep(initial_delay * (backoff**attempt))
                continue

            raise RuntimeError(f"Unexpected status '{status}': {info}")

        raise TimeoutError(f"Polling '{status_loc}' exceeded {max_attempts} attempts.")

    def import_uri(self, pattern: str, template: str, uri: str) -> dict:
        """
        Sends an import request and blocks until completion.
        """
        url = f"{self.base_url}/import/{quote(pattern)}/{quote(template)}"
        resp = self.session.get(url, params={"uri": uri})
        resp.raise_for_status()
        return self._poll(template)


def get_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Serve directory and upload MeTTa file URI for MORK server"
    )
    parser.add_argument(
        "--file", type=Path, required=True, help="Path to the MeTTa file to upload."
    )
    return parser.parse_args()


def copy_to_temp(args: argparse.Namespace) -> Path:
    try:
        tmp_dir = Path("/tmp")
        dest = tmp_dir / args.file.name
        shutil.copy2(args.file, dest)
        return dest
    except FileNotFoundError as e:
        print(f"Error: {e}")
        raise e


def load_s_expressions(
    uri: str,
    mork_server_address: str = MORK_SERVER_ADDRESS,
) -> dict:
    """
    Loads S-expressions from the given URI via the MORK server.
    """
    client = MorkClient(base_url=mork_server_address)
    return client.import_uri(pattern="$x", template="$x", uri=uri)


def main():
    args = get_args()
    dest = copy_to_temp(args)
    try:
        with FileServer() as file_server:
            print("Using port: ", file_server.get_port())
            file_uri = f"http://{file_server.server.server_address[0]}:{file_server.server.server_address[1]}/{dest.name}"
            load_s_expressions(file_uri)
            print("Done!")
    except Exception:
        raise RuntimeError("Failed to load the MeTTa file. Please check the file and try again.")


if __name__ == "__main__":
    main()
