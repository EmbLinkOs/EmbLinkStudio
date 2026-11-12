# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    trust.py
# Author:  Junior Deogracias
# Created: 2026-11-12
"""Which checkouts the person trusts to run code (spec section 9, Security).

Opening a board reads its files and runs nothing. Validating and generating run the
checkout's own generator, so they wait until the person trusts that checkout once
with project.trust. Trust is kept per path in the person's own settings, never in a
project or checkout, so a checkout cannot ship its own trust:

    macOS    ~/Library/Application Support/EmbLink Studio/trust.json
    Windows  %APPDATA%/EmbLink Studio/trust.json
    others   $XDG_CONFIG_HOME/emblink-studio/trust.json (~/.config by default)

EMBLINK_STUDIO_CONFIG_DIR replaces the directory, for tests.
"""

from __future__ import annotations

import json
import os
import sys
import tempfile
from pathlib import Path


def config_dir() -> Path:
    override = os.environ.get("EMBLINK_STUDIO_CONFIG_DIR")
    if override:
        return Path(override)
    if sys.platform == "darwin":
        return Path.home() / "Library" / "Application Support" / "EmbLink Studio"
    if sys.platform == "win32":
        return Path(os.environ.get("APPDATA", Path.home())) / "EmbLink Studio"
    return Path(os.environ.get("XDG_CONFIG_HOME", Path.home() / ".config")) / "emblink-studio"


class TrustStore:
    VERSION = 1

    def __init__(self, directory: Path | None = None) -> None:
        self.file = (directory or config_dir()) / "trust.json"

    def _load(self) -> set[str]:
        try:
            data = json.loads(self.file.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            return set()
        if not isinstance(data, dict) or data.get("version") != self.VERSION:
            return set()
        return {p for p in data.get("trusted", []) if isinstance(p, str)}

    @staticmethod
    def _key(path: Path) -> str:
        return str(path.expanduser().resolve())

    def is_trusted(self, path: Path) -> bool:
        return self._key(path) in self._load()

    def trust(self, path: Path) -> None:
        trusted = self._load() | {self._key(path)}
        self.file.parent.mkdir(parents=True, exist_ok=True)
        body = json.dumps({"version": self.VERSION, "trusted": sorted(trusted)}, indent=2)
        # Written whole and renamed into place, so a crash never leaves half a file.
        fd, tmp = tempfile.mkstemp(dir=self.file.parent, prefix=".trust-", suffix=".json")
        with os.fdopen(fd, "w", encoding="utf-8") as out:
            out.write(body + "\n")
        os.replace(tmp, self.file)
