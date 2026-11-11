# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
"""The `embstudio-core` command; every RPC method gains a subcommand as it lands (spec §4)."""

from __future__ import annotations

import argparse

from . import __version__, protocol


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="embstudio-core", description="EmbStudio Core")
    parser.add_argument(
        "--version",
        action="version",
        version=f"embstudio-core {__version__} (protocol {protocol.PROTOCOL_VERSION})",
    )
    commands = parser.add_subparsers(dest="command", metavar="command")
    commands.add_parser("serve", help="serve JSON-RPC 2.0 on standard input and output")
    args = parser.parse_args(argv)

    if args.command == "serve":
        from .server import serve

        return serve()
    parser.print_help()
    return 2
