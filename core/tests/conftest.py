# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    conftest.py
# Author:  Junior Deogracias
# Created: 2026-11-11
"""Shared pytest helpers for the core's tests.

Builds JSON-RPC messages (`request`, `notification`, `INIT`), frames them (`frames`),
runs the core as a real child process the way the desktop does (`run_core`), and
loads the protocol's JSON schemas from protocol/ (the `schemas` fixture).

From stage S1: `fake_rtos` builds a temporary EmbLinkRTOS checkout whose emb-hwgen is
fake_hwgen.py, with one generated board (acme/demo) and one not yet generated
(acme/blank); `config_dir` gives each test its own settings directory, so a test
never reads or writes the person's real trust file.
"""

from __future__ import annotations

import io
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any

import pytest

from emblink_studio_core.framing import encode, read_message

TESTS_DIR = Path(__file__).resolve().parent

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


def run_core(
    data: bytes, env: dict[str, str] | None = None
) -> tuple[int, list[dict[str, Any]], str]:
    """Run `python -m emblink_studio_core serve` as the desktop would, feeding it `data`."""
    env = dict(os.environ, **(env or {}), PYTHONPATH=str(CORE_SRC))
    proc = subprocess.run(
        [sys.executable, "-m", "emblink_studio_core", "serve"],
        input=data,
        capture_output=True,
        env=env,
        timeout=30,
    )
    return proc.returncode, read_all(proc.stdout), proc.stderr.decode()


@pytest.fixture(scope="session")
def schemas() -> dict[str, Any]:
    return {p.name: json.loads(p.read_text()) for p in PROTOCOL_DIR.glob("*.schema.json")}


DEMO_BOARD = """\
schema_version: 0
kind: board
name: demo
vendor: acme
soc: acme/acme1.yaml
clocks:
  cpu_hz: 48000000
"""


def write_board(root: Path, board_id: str, text: str) -> Path:
    board = root / "boards" / board_id
    board.mkdir(parents=True, exist_ok=True)
    (board / "board.yaml").write_text(text, encoding="utf-8")
    return board


def hwgen(root: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(root / "hw/gen/emb-hwgen.py"), *args],
        cwd=root,
        capture_output=True,
        text=True,
        check=False,
    )


@pytest.fixture
def fake_rtos(tmp_path: Path) -> Path:
    """A temporary checkout: acme/demo generated, acme/blank not yet."""
    root = tmp_path / "rtos"
    gen = root / "hw" / "gen"
    (gen / "emb_hwgen").mkdir(parents=True)
    (gen / "emb-hwgen.py").write_text((TESTS_DIR / "fake_hwgen.py").read_text(), encoding="utf-8")
    (gen / "emb_hwgen" / "__init__.py").write_text(
        '"""The fake generator\'s package."""\n\nVERSION = "0.1.0"\nSCHEMA_VERSION = 0\n',
        encoding="utf-8",
    )
    write_board(root, "acme/demo", DEMO_BOARD)
    write_board(root, "acme/blank", DEMO_BOARD.replace("name: demo", "name: blank"))
    assert hwgen(root, "generate", "--board", "boards/acme/demo").returncode == 0
    return root


@pytest.fixture
def config_dir(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    directory = tmp_path / "config"
    monkeypatch.setenv("EMBLINK_STUDIO_CONFIG_DIR", str(directory))
    return directory
