#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    fake_hwgen.py
# Author:  Junior Deogracias
# Created: 2026-11-12
"""A stand-in for EmbLinkRTOS's emb-hwgen, for the core's tests.

The `fake_rtos` fixture installs it as hw/gen/emb-hwgen.py of a temporary checkout.
It has the real tool's interface (validate, and generate --board DIR [--out DIR]),
exit codes (0, and 2 for description errors) and output format (the RTOS's HW-002:
'file:line: node.path: message', warnings prefixed 'warning: '). What it reports is
written in the board.yaml it reads, one instruction per line:

    # error: <path>: <message>     an error at this line
    # warning: <path>: <message>   a warning at this line
    # crash                        a traceback and exit code 3, as a broken generator
    # no-yaml                      PyYAML missing, as the real tool reports it

generate writes hw.json and hw_config.h from the board's name and cpu_hz lines.
"""

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def model(text: str, rel: str) -> dict:
    name = re.search(r"^name: (\S+)", text, re.M).group(1)
    cpu_hz = int(re.search(r"cpu_hz: (\d+)", text).group(1))
    schema = (
        int(re.search(r"^hw_schema: (\d+)", text, re.M).group(1)) if "hw_schema:" in text else 0
    )
    return {
        "schema_version": schema,
        "generator": {"name": "emb-hwgen", "version": "0.1.0"},
        "inputs": [{"path": f"{rel}/board.yaml", "sha256": "0" * 64}],
        "warnings": [],
        "board": {
            "name": name,
            "vendor": "acme",
            "revision": "A",
            "curated": True,
            "doc": "A test board.",
            "console": {"device": "uart0", "baud": 115200},
            "leds": [{"name": "user", "gpio": "PA5", "active": "high"}],
            "clocks": {"cpu_hz": cpu_hz},
            "oscillators": {"hse": {"hz": 8000000}},
        },
        "soc": {
            "name": "acme1",
            "part": "ACME1",
            "vendor": "acme",
            "family": "acme",
            "package": "qfn32",
            "cpu": {"core": "cortex-m4f"},
            "gpio": {"ports": {"A": 255}},
            "alternate_functions": {"PA0": {"af1": "uart0_tx"}, "PA1": {"af1": "uart0_rx"}},
            "memory": {
                "regions": [
                    {"name": "flash", "base": 0, "size": 65536, "attrs": ["r", "x"]},
                    {"name": "sram", "base": 536870912, "size": 16384, "attrs": ["r", "w"]},
                ]
            },
            "flash": {"region": "flash"},
            "clocks": {"max_hz": 200000000},
            "peripherals": [{"name": "uart0", "base": 1073741824, "size": 1024}],
        },
        "derived": {
            "clock": {"cpu_hz": cpu_hz, "source": "hse", "source_hz": 8000000},
            "pins": [
                {
                    "pin": "PA0",
                    "port": "A",
                    "number": 0,
                    "mode": "af",
                    "af": 1,
                    "gpio": "gpioa",
                    "owner": "pins.uart0.tx",
                    "signal": "uart0_tx",
                }
            ],
            "devices": [{"name": "uart0", "class": "uart"}],
            "profile": "base",
            "kconfig": {"CONFIG_EMB_CM_CPU_HZ": cpu_hz},
            "timer": {"name": "systick"},
            "linker": {"flash": {"origin": 0, "length": 65536}},
            "partitions": [],
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="cmd", required=True)
    for cmd in ("validate", "generate"):
        p = sub.add_parser(cmd)
        p.add_argument("--board", required=True)
        p.add_argument("--out")
    args = parser.parse_args()

    board = Path(args.board)
    board = board if board.is_absolute() else ROOT / board
    rel = board.relative_to(ROOT).as_posix() if board.is_relative_to(ROOT) else str(board)
    text = (board / "board.yaml").read_text(encoding="utf-8")
    errors, warnings = [], []
    for number, line in enumerate(text.splitlines(), 1):
        if "# crash" in line:
            print("Traceback (most recent call last):\n  KeyError: 'pll'", file=sys.stderr)
            return 3
        if "# no-yaml" in line:
            print("ModuleNotFoundError: No module named 'yaml'", file=sys.stderr)
            return 1
        for tag, found in (("# error: ", errors), ("# warning: ", warnings)):
            if tag in line:
                found.append(f"{rel}/board.yaml:{number}: {line.split(tag, 1)[1]}")
    for warning in warnings:
        print(f"warning: {warning}", file=sys.stderr)
    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        print(f"emb-hwgen: {len(errors)} error(s)", file=sys.stderr)
        return 2
    if args.cmd == "validate":
        print(f"emb-hwgen: {args.board} is valid")
        return 0

    out = Path(args.out) if args.out else board / "generated"
    out.mkdir(parents=True, exist_ok=True)
    hw = model(text, rel)
    (out / "hw.json").write_text(json.dumps(hw, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    (out / "hw_config.h").write_text(
        f"/* generated by emb-hwgen */\n#define EMB_CPU_HZ {hw['derived']['clock']['cpu_hz']}\n",
        encoding="utf-8",
    )
    print(f"emb-hwgen: wrote 2 files to {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
