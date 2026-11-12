# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    hwmodel.py
# Author:  Junior Deogracias
# Created: 2026-11-12
"""A board's resolved model, read from the hw.json emb-hwgen writes.

hw.json (schema version 0, hw/schemas/hw.schema.json in the RTOS) holds the board and
its SoC as the generator resolved them, and what it derived: the clock tree, the pins
in use, the devices, the linker regions and partitions. The board.* methods answer
from it (spec section 5: "From the generator's resolved model; nothing computed in
the GUI"), and the core computes nothing either: each view is a selection of hw.json,
its values exactly the generator's.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from . import protocol
from .errors import CoreError
from .rtos import SUPPORTED_HW_SCHEMAS


class HwModel:
    def __init__(self, data: dict[str, Any], path: Path) -> None:
        self.data = data
        self.path = path

    @classmethod
    def load(cls, path: Path) -> HwModel:
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except OSError as exc:
            raise CoreError(protocol.INVALID_REQUEST, f"Cannot read {path}: {exc}") from exc
        except json.JSONDecodeError as exc:
            raise CoreError(
                protocol.TOOL_FAILED, f"{path} is not valid JSON: {exc}", {"file": str(path)}
            ) from exc
        version = data.get("schema_version")
        if version not in SUPPORTED_HW_SCHEMAS:
            raise CoreError(
                protocol.UNSUPPORTED_RTOS,
                f"{path} is hw.json schema {version}; EmbLink Studio reads schema "
                f"{', '.join(map(str, sorted(SUPPORTED_HW_SCHEMAS)))}.",
                {"file": str(path)},
            )
        return cls(data, path)

    def _section(self, name: str) -> dict[str, Any]:
        value = self.data.get(name)
        return value if isinstance(value, dict) else {}

    @property
    def board(self) -> dict[str, Any]:
        return self._section("board")

    @property
    def soc(self) -> dict[str, Any]:
        return self._section("soc")

    @property
    def derived(self) -> dict[str, Any]:
        return self._section("derived")

    def summary(self) -> dict[str, Any]:
        """A line about the board, for lists: its name, SoC, CPU and clock."""
        clock = self.derived.get("clock", {})
        cpu = self.soc.get("cpu", {})
        return {
            "name": self.board.get("name"),
            "vendor": self.board.get("vendor"),
            "revision": self.board.get("revision"),
            "curated": bool(self.board.get("curated")),
            "soc": self.soc.get("part") or self.soc.get("name"),
            "family": self.soc.get("family"),
            "package": self.soc.get("package"),
            "core": cpu.get("core") if isinstance(cpu, dict) else None,
            "cpu_hz": clock.get("cpu_hz") if isinstance(clock, dict) else None,
        }

    def describe(self) -> dict[str, Any]:
        """The board and its chip: what the Board view shows (spec section 7)."""
        board_keys = (
            "name",
            "vendor",
            "revision",
            "doc",
            "curated",
            "console",
            "debug_probe",
            "leds",
            "buttons",
            "simulation",
            "defaults",
        )
        soc_keys = (
            "name",
            "part",
            "vendor",
            "family",
            "package",
            "curated",
            "cpu",
            "arch",
            "debug",
            "interrupts",
            "peripherals",
        )
        return {
            "board": {k: self.board[k] for k in board_keys if k in self.board},
            "soc": {k: self.soc[k] for k in soc_keys if k in self.soc},
            "devices": self.derived.get("devices", []),
            "profile": self.derived.get("profile"),
            "kconfig": self.derived.get("kconfig", {}),
            "generator": self.data.get("generator", {}),
            "inputs": self.data.get("inputs", []),
            "warnings": self.data.get("warnings", []),
        }

    def pins(self) -> dict[str, Any]:
        """The pins in use, with their owner, and every pin's alternate functions."""
        gpio = self.soc.get("gpio", {})
        return {
            "package": self.soc.get("package"),
            "assigned": self.derived.get("pins", []),
            "alternate_functions": self.soc.get("alternate_functions", {}),
            "ports": gpio.get("ports", {}) if isinstance(gpio, dict) else {},
        }

    def clocks(self) -> dict[str, Any]:
        """The clock tree the generator resolved, what the board asked for, the chip's limits."""
        return {
            "resolved": self.derived.get("clock", {}),
            "requested": self.board.get("clocks", {}),
            "oscillators": self.board.get("oscillators", {}),
            "limits": self.soc.get("clocks", {}),
            "timer": self.derived.get("timer", {}),
        }

    def memory(self) -> dict[str, Any]:
        """The chip's memory regions, the flash's geometry, the linker's regions, the partitions."""
        memory = self.soc.get("memory", {})
        return {
            "regions": memory.get("regions", []) if isinstance(memory, dict) else [],
            "flash": self.soc.get("flash", {}),
            "linker": self.derived.get("linker", {}),
            "partitions": self.derived.get("partitions", []),
        }
