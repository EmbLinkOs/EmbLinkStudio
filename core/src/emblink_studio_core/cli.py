# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    cli.py
# Author:  Junior Deogracias
# Created: 2026-11-11
"""The `emblink-studio-core` command line.

`serve` runs the JSON-RPC server on standard input and output; `--version` prints the
core's version and the protocol it speaks. Every RPC method has a subcommand too
(spec section 4), printing the same result as JSON:

    emblink-studio-core project status BOARD
    emblink-studio-core project validate BOARD      exit 0 if valid, 2 if not
    emblink-studio-core board list RTOS
    emblink-studio-core board describe|pins|clocks|memory BOARD
    emblink-studio-core generate run BOARD
    emblink-studio-core generate diff BOARD         exit 0 if up to date, 1 if not

BOARD is a board directory of an EmbLinkRTOS checkout, such as
~/EmbLinkRTOS/boards/st/nucleo_f446re; RTOS is the checkout. The person running a
command chose to run the checkout's generator, so the command line does not ask for
trust; the IDE does (spec section 9).
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

from . import __version__, protocol


def _print(result: Any) -> None:
    json.dump(result, sys.stdout, indent=2, ensure_ascii=False)
    sys.stdout.write("\n")


def _run(args: argparse.Namespace) -> int:
    from .rtos import Rtos
    from .workspace import Workspace, list_boards

    if args.command == "board" and args.action == "list":
        rtos = Rtos.find(Path(args.target).expanduser().resolve())
        if rtos is None:
            print(f"{args.target} is not an EmbLinkRTOS checkout.", file=sys.stderr)
            return 1
        _print(
            {
                "rtos": {"root": str(rtos.root), "generator": rtos.check_supported()},
                "boards": list_boards(rtos),
            }
        )
        return 0

    workspace = Workspace.open(args.target)
    if args.command == "project" and args.action == "status":
        _print(workspace.status(trusted=True))
        return 0
    if args.command == "project" and args.action == "validate":
        result = workspace.validate()
        _print(result)
        return 0 if result["valid"] else 2
    if args.command == "board":
        _print(getattr(workspace.model(), args.action)())
        return 0
    if args.command == "generate" and args.action == "run":
        result = workspace.generate()
        _print(result)
        return 0 if result["written"] else 2
    result = workspace.diff()
    _print(result)
    return 0 if result["up_to_date"] else 1


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="emblink-studio-core", description="EmbLink Studio Core")
    parser.add_argument(
        "--version",
        action="version",
        version=f"emblink-studio-core {__version__} (protocol {protocol.PROTOCOL_VERSION})",
    )
    commands = parser.add_subparsers(dest="command", metavar="command")
    commands.add_parser("serve", help="serve JSON-RPC 2.0 on standard input and output")

    project = commands.add_parser("project", help="a board's status and validation")
    project.add_argument("action", choices=["status", "validate"])
    project.add_argument("target", metavar="BOARD", help="a board directory of an RTOS checkout")
    board = commands.add_parser("board", help="the boards of a checkout, and a board's model")
    board.add_argument("action", choices=["list", "describe", "pins", "clocks", "memory"])
    board.add_argument(
        "target", metavar="BOARD_OR_RTOS", help="the checkout for list, a board directory otherwise"
    )
    generate = commands.add_parser(
        "generate", help="run the generator, or see what it would change"
    )
    generate.add_argument("action", choices=["run", "diff"])
    generate.add_argument("target", metavar="BOARD", help="a board directory of an RTOS checkout")
    args = parser.parse_args(argv)

    if args.command == "serve":
        from .server import serve

        return serve()
    if args.command is None:
        parser.print_help()
        return 2

    from .errors import CoreError

    try:
        return _run(args)
    except CoreError as exc:
        print(exc.message, file=sys.stderr)
        return 1
