# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    diagnostics.py
# Author:  Junior Deogracias
# Created: 2026-11-12
"""The generator's diagnostics, as structured records the views can link to a file.

emb-hwgen 0.x writes one diagnostic per line (the RTOS's rule HW-002):

    boards/st/nucleo_f446re/board.yaml:25: pins.usart2.tx: PA2 af9 is not usart2_tx
    warning: soc/st/stm32f4/stm32f4.yaml:17: cpu.mpu: the cortex_m port does not use the MPU yet

that is, an optional 'warning: ', then the file (relative to the checkout, or absolute
for a board outside it), the line, the node's path in the YAML ('(root)' for the
document) and the message. Each becomes

    {"severity", "message", "file", "line", "path", "source"}

with the file made absolute, so a view opens it at the line. Lines that are not
diagnostics (the generator's own summary) are dropped; a line in no known form is
kept as a diagnostic with only its message, so nothing the generator says is lost.
When the generator writes JSON diagnostics (asked of the RTOS for S1), they are read
instead and this parser stays for older checkouts.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

SOURCE = "emb-hwgen"
_LOCATED = re.compile(r"^(?P<file>.+?):(?P<line>\d+): (?P<rest>.*)$")
_PATH = re.compile(r"^(?P<path>[^\s:]+): (?P<message>.+)$")


def parse(text: str, root: Path) -> list[dict[str, Any]]:
    """The diagnostics in emb-hwgen's output `text`; relative files are under `root`."""
    out: list[dict[str, Any]] = []
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith("emb-hwgen"):
            continue  # blank, or the generator's summary ('emb-hwgen: 8 error(s)')
        severity = "error"
        if line.startswith("warning: "):
            severity = "warning"
            line = line[len("warning: ") :]
        diagnostic: dict[str, Any] = {"severity": severity, "source": SOURCE}
        located = _LOCATED.match(line)
        if located is None:
            diagnostic["message"] = line
            out.append(diagnostic)
            continue
        file = Path(located["file"])
        diagnostic["file"] = str(file if file.is_absolute() else root / file)
        diagnostic["line"] = int(located["line"])
        rest = located["rest"]
        with_path = _PATH.match(rest)
        if with_path is not None:
            diagnostic["path"] = with_path["path"]
            diagnostic["message"] = with_path["message"]
        else:
            diagnostic["message"] = rest
        out.append(diagnostic)
    return out


def counts(diagnostics: list[dict[str, Any]]) -> dict[str, int]:
    return {
        "errors": sum(1 for d in diagnostics if d["severity"] == "error"),
        "warnings": sum(1 for d in diagnostics if d["severity"] == "warning"),
    }
