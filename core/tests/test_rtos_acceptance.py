# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    test_rtos_acceptance.py
# Author:  Junior Deogracias
# Created: 2026-11-12
"""Stage S1's acceptance, against a real EmbLinkRTOS checkout (spec section 8).

"Opening the RTOS's nucleo_f446re and pico2_w boards shows their model; validation
reports the generator's errors with their locations."

EMBLINK_STUDIO_TEST_RTOS names the checkout; CI checks out the RTOS at a pinned commit
for these tests, and they are skipped where it is not set. The model is checked
against the board's own hw.json and the facts of its datasheet; the errors are made
in a copy of the NUCLEO-F446RE's description, inside a copy of the files the
generator reads, and each is expected at its line and its path in the YAML.
"""

import json
import os
import shutil
from pathlib import Path

import jsonschema
import pytest

from emblink_studio_core.errors import CoreError
from emblink_studio_core.workspace import Workspace

RTOS = os.environ.get("EMBLINK_STUDIO_TEST_RTOS")
pytestmark = pytest.mark.skipif(not RTOS, reason="EMBLINK_STUDIO_TEST_RTOS is not set")


def validate(board_dir):
    """The validation, or a failure that shows what the generator printed."""
    try:
        return Workspace.open(board_dir).validate()
    except CoreError as exc:
        pytest.fail(f"{exc.message}\n{(exc.data or {}).get('output', '')}")

BOARDS = {
    # board: (part, cpu_hz, a pin in use and its signal, the first memory region)
    "st/nucleo_f446re": ("STM32F446RE", 180_000_000, ("PA2", "usart2_tx"), "flash"),
    "rpi/pico2_w": ("RP2350A", 150_000_000, ("GPIO0", "uart0_tx"), None),
}


@pytest.mark.parametrize("board_id", sorted(BOARDS))
def test_opening_a_board_shows_its_model(board_id, schemas):
    part, cpu_hz, (pin, signal), first_region = BOARDS[board_id]
    board_dir = Path(RTOS) / "boards" / board_id
    hw = json.loads((board_dir / "generated/hw.json").read_text(encoding="utf-8"))
    ws = Workspace.open(board_dir)

    status = ws.status(trusted=True)
    jsonschema.validate(status, schemas["project.status.result.schema.json"])
    assert status["board"]["soc"] == part
    assert status["board"]["cpu_hz"] == cpu_hz

    model = ws.model()
    views = {
        "describe": model.describe(),
        "pins": model.pins(),
        "clocks": model.clocks(),
        "memory": model.memory(),
    }
    for name, view in views.items():
        jsonschema.validate(view, schemas[f"board.{name}.result.schema.json"])
    assert views["describe"]["soc"]["part"] == part
    assert views["describe"]["devices"] == hw["derived"]["devices"]
    assert views["clocks"]["resolved"] == hw["derived"]["clock"]
    assert views["pins"]["assigned"] == hw["derived"]["pins"]
    assert {p["pin"]: p.get("signal") for p in views["pins"]["assigned"]}[pin] == signal
    assert views["memory"]["regions"] == hw["soc"]["memory"]["regions"]
    if first_region:
        assert views["memory"]["regions"][0]["name"] == first_region


@pytest.mark.parametrize("board_id", sorted(BOARDS))
def test_the_committed_boards_are_valid(board_id, schemas):
    result = validate(Path(RTOS) / "boards" / board_id)
    jsonschema.validate(result, schemas["project.validate.result.schema.json"])
    assert result["valid"] is True, result
    assert result["errors"] == 0
    for d in result["diagnostics"]:  # warnings, each located
        assert Path(d["file"]).is_file()
        assert d["line"] >= 1


def test_errors_are_reported_where_they_are(tmp_path, schemas):
    src = Path(RTOS)
    board_rel = Path("boards/st/nucleo_f446re")
    hw = json.loads((src / board_rel / "generated/hw.json").read_text(encoding="utf-8"))
    # A checkout of just what the generator reads: its own code and the board's inputs.
    root = tmp_path / "rtos"
    shutil.copytree(src / "hw", root / "hw", ignore=shutil.ignore_patterns("__pycache__"))
    for item in hw["inputs"]:
        (root / item["path"]).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src / item["path"], root / item["path"])

    board_yaml = root / board_rel / "board.yaml"
    text = board_yaml.read_text(encoding="utf-8")
    broken = {
        "cpu_hz: 180MHz": ("cpu_hz: 900MHz", "clocks.pll"),
        "usart2: { tx: PA2.af7": ("usart2: { tx: PA2.af9", "pins.usart2.tx"),
        "console: { device: usart2": ("console: { device: usart9", "console.device"),
    }
    expected = {}
    for good, (bad, _path) in broken.items():
        assert good in text, f"the NUCLEO-F446RE's description changed: no '{good}'"
        text = text.replace(good, bad)
    lines = text.splitlines()
    for bad, path in broken.values():
        # clocks.pll is reported at the pll line, the factors that cannot give 900 MHz.
        needle = "pll:" if path == "clocks.pll" else bad
        expected[path] = next(i for i, line in enumerate(lines, 1) if needle in line)
    board_yaml.write_text(text, encoding="utf-8")

    result = validate(root / board_rel)
    jsonschema.validate(result, schemas["project.validate.result.schema.json"])
    assert result["valid"] is False
    errors = {d["path"]: d for d in result["diagnostics"] if d["severity"] == "error"}
    for path, line in expected.items():
        assert path in errors, (path, sorted(errors))
        assert errors[path]["line"] == line, (path, errors[path])
        assert Path(errors[path]["file"]) == board_yaml.resolve()
    assert "PA2 offers af7 usart2_tx" in errors["pins.usart2.tx"]["message"]
