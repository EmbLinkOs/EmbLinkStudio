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
core's version and the protocol it speaks. Every RPC method gains a subcommand as it
lands (spec section 4).
"""

from __future__ import annotations

import argparse

from . import __version__, protocol


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="emblink-studio-core", description="EmbLink Studio Core")
    parser.add_argument(
        "--version",
        action="version",
        version=f"emblink-studio-core {__version__} (protocol {protocol.PROTOCOL_VERSION})",
    )
    commands = parser.add_subparsers(dest="command", metavar="command")
    commands.add_parser("serve", help="serve JSON-RPC 2.0 on standard input and output")
    args = parser.parse_args(argv)

    if args.command == "serve":
        from .server import serve

        return serve()
    parser.print_help()
    return 2
