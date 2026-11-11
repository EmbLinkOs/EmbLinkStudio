# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
from __future__ import annotations

import io
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any

import pytest

from embstudio_core.framing import encode, read_message

CORE_SRC = Path(__file__).resolve().parents[1] / "src"
PROTOCOL_DIR = Path(__file__).resolve().parents[2] / "protocol"


def frames(*messages: dict[str, Any]) -> bytes:
    return b"".join(encode(m) for m in messages)


def request(msg_id: int, method: str, params: Any = None) -> dict[str, Any]:
    message: dict[str, Any] = {"jsonrpc": "2.0", "id": msg_id, "method": method}
    if params is not None:
        message["params"] = params
    return message


def notification(method: str, params: Any = None) -> dict[str, Any]:
    message: dict[str, Any] = {"jsonrpc": "2.0", "method": method}
    if params is not None:
        message["params"] = params
    return message


INIT = request(1, "initialize", {"protocol_version": "0.1.0", "client": {"name": "test"}})


def read_all(data: bytes) -> list[dict[str, Any]]:
    stream = io.BytesIO(data)
    out = []
    while (message := read_message(stream)) is not None:
        out.append(message)
    return out


def run_core(data: bytes) -> tuple[int, list[dict[str, Any]], str]:
    """Run `python -m embstudio_core serve` as the desktop would, feeding it `data`."""
    env = dict(os.environ, PYTHONPATH=str(CORE_SRC))
    proc = subprocess.run(
        [sys.executable, "-m", "embstudio_core", "serve"],
        input=data,
        capture_output=True,
        env=env,
        timeout=30,
    )
    return proc.returncode, read_all(proc.stdout), proc.stderr.decode()


@pytest.fixture(scope="session")
def schemas() -> dict[str, Any]:
    return {p.name: json.loads(p.read_text()) for p in PROTOCOL_DIR.glob("*.schema.json")}
