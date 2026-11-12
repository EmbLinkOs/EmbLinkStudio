# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    rtos.py
# Author:  Junior Deogracias
# Created: 2026-11-12
"""An EmbLinkRTOS checkout, and its hardware generator, emb-hwgen (spec section 4).

The core never carries a copy of the RTOS's tools: it runs the emb-hwgen of the
checkout a board belongs to, as a subprocess, and reads what it writes. It also never
imports the RTOS's code into its own process; the generator's version is read from
its source.

The Python that runs the generator is the core's own, or EMBLINK_STUDIO_RTOS_PYTHON
when set. The generator's only third-party dependency is PyYAML, which the core
therefore depends on too.

Supported formats (spec section 4: the core states the range it supports): hw.json
schema version 0, and the diagnostics of emb-hwgen 0.x, one per line as
'file:line: node.path: message' (the RTOS's rule HW-002). A checkout outside that
range is refused with unsupported_rtos.
"""

from __future__ import annotations

import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

from . import protocol
from .errors import CoreError

HWGEN = Path("hw/gen/emb-hwgen.py")
HWGEN_PACKAGE = Path("hw/gen/emb_hwgen/__init__.py")
SUPPORTED_HW_SCHEMAS = frozenset({0})
SUPPORTED_GENERATOR_MAJORS = frozenset({0})
GENERATOR_TIMEOUT_S = 120


def interpreter() -> str:
    """The Python that runs the RTOS's tools."""
    return os.environ.get("EMBLINK_STUDIO_RTOS_PYTHON") or sys.executable


@dataclass(frozen=True)
class Rtos:
    root: Path

    @classmethod
    def find(cls, start: Path) -> Rtos | None:
        """The checkout `start` is inside of: the nearest directory holding hw/gen/emb-hwgen.py."""
        for directory in (start, *start.parents):
            if (directory / HWGEN).is_file():
                return cls(directory)
        return None

    def generator(self) -> dict[str, object]:
        """emb-hwgen's name, version and the hw.json schema version it writes."""
        try:
            source = (self.root / HWGEN_PACKAGE).read_text(encoding="utf-8")
        except OSError as exc:
            raise CoreError(
                protocol.TOOL_MISSING, f"The RTOS checkout at {self.root} has no emb-hwgen package."
            ) from exc
        version = re.search(r'^VERSION\s*=\s*"([^"]+)"', source, re.M)
        schema = re.search(r"^SCHEMA_VERSION\s*=\s*(\d+)", source, re.M)
        return {
            "name": "emb-hwgen",
            "version": version.group(1) if version else "unknown",
            "schema_version": int(schema.group(1)) if schema else -1,
        }

    def check_supported(self) -> dict[str, object]:
        """The generator's facts, or unsupported_rtos when this core cannot read its formats."""
        gen = self.generator()
        version = str(gen["version"])
        major = int(version.split(".", 1)[0]) if version.split(".", 1)[0].isdigit() else -1
        if (
            major not in SUPPORTED_GENERATOR_MAJORS
            or gen["schema_version"] not in SUPPORTED_HW_SCHEMAS
        ):
            raise CoreError(
                protocol.UNSUPPORTED_RTOS,
                f"This RTOS checkout's emb-hwgen {version} writes hw.json schema "
                f"{gen['schema_version']}; EmbLink Studio reads emb-hwgen 0.x and hw.json schema "
                f"{', '.join(map(str, sorted(SUPPORTED_HW_SCHEMAS)))}.",
                {"generator": gen},
            )
        return gen

    def boards(self) -> list[Path]:
        """Every board directory of the checkout: boards/<vendor>/<board>/ holding board.yaml."""
        return sorted(p.parent for p in (self.root / "boards").glob("*/*/board.yaml"))

    def board_id(self, board_dir: Path) -> str:
        """'st/nucleo_f446re' for boards/st/nucleo_f446re."""
        try:
            return board_dir.relative_to(self.root / "boards").as_posix()
        except ValueError:
            return board_dir.name

    def run_hwgen(self, *args: str) -> subprocess.CompletedProcess[str]:
        """Runs the checkout's emb-hwgen with `args`, from the checkout's root."""
        script = self.root / HWGEN
        if not script.is_file():
            raise CoreError(protocol.TOOL_MISSING, f"emb-hwgen is missing from {self.root}.")
        try:
            proc = subprocess.run(
                [interpreter(), str(script), *args],
                cwd=self.root,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
                timeout=GENERATOR_TIMEOUT_S,
            )
        except FileNotFoundError as exc:
            raise CoreError(
                protocol.TOOL_MISSING, f"The Python that runs the RTOS's tools is missing: {exc}"
            ) from exc
        except subprocess.TimeoutExpired as exc:
            raise CoreError(
                protocol.TOOL_FAILED,
                f"emb-hwgen did not finish within {GENERATOR_TIMEOUT_S} s.",
                {"tool": "emb-hwgen", "args": list(args)},
            ) from exc
        if "No module named 'yaml'" in proc.stderr:
            raise CoreError(
                protocol.TOOL_MISSING,
                "The RTOS's generator needs PyYAML in the Python that runs it "
                f"({interpreter()}). Install it with: pip install pyyaml",
                {"tool": "emb-hwgen", "output": proc.stderr},
            )
        return proc
