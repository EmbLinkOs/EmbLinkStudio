# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    workspace.py
# Author:  Junior Deogracias
# Created: 2026-11-12
"""The workspace the core serves: in stage S1, a board of an EmbLinkRTOS checkout.

Until the project manifest and the module contract exist (the RTOS's phase E1),
stages S1 to S3 work on what exists: the boards in the RTOS tree and their generated
model (spec section 8). So project.open takes a board directory, boards/<vendor>/<name>/
holding board.yaml, inside a checkout. A directory holding emb-project.yaml is refused
for now with a message that says so; it opens once the project schema is agreed.

Opening reads files and runs nothing. validate, generate and diff run the checkout's
emb-hwgen and need the checkout trusted (trust.py). The model is re-read from hw.json
whenever the file changes, so a generate, or an edit made outside, shows at once.
"""

from __future__ import annotations

import difflib
import tempfile
from pathlib import Path
from typing import Any

from . import diagnostics, protocol
from .errors import CoreError
from .hwmodel import HwModel
from .rtos import Rtos

# A diff longer than this is cut, and says so: a view shows it, it is not a patch.
DIFF_LINE_LIMIT = 400


class Workspace:
    kind = "board"

    def __init__(self, board_dir: Path, rtos: Rtos, generator: dict[str, object]) -> None:
        self.board_dir = board_dir
        self.rtos = rtos
        self.generator = generator
        self._model: HwModel | None = None
        self._model_stamp: tuple[int, int] | None = None

    # -- opening ------------------------------------------------------------------------

    @classmethod
    def open(cls, path: str | Path) -> Workspace:
        directory = Path(path).expanduser()
        if not directory.is_dir():
            raise CoreError(protocol.INVALID_PARAMS, f"{directory} is not a directory.")
        directory = directory.resolve()
        if (directory / "emb-project.yaml").is_file() and not (directory / "board.yaml").is_file():
            raise CoreError(
                protocol.INVALID_PARAMS,
                "Projects (emb-project.yaml) open once the project manifest is agreed with the "
                "RTOS (phase E1). For now, open a board of an EmbLinkRTOS checkout, such as "
                "boards/st/nucleo_f446re.",
            )
        if not (directory / "board.yaml").is_file():
            raise CoreError(
                protocol.INVALID_PARAMS,
                f"{directory} holds no board.yaml. Open a board of an EmbLinkRTOS checkout, "
                "such as boards/st/nucleo_f446re.",
            )
        rtos = Rtos.find(directory)
        if rtos is None:
            raise CoreError(
                protocol.TOOL_MISSING,
                f"{directory} is not inside an EmbLinkRTOS checkout (no hw/gen/emb-hwgen.py "
                "above it).",
            )
        workspace = cls(directory, rtos, rtos.check_supported())
        if workspace.hw_json.is_file():
            workspace.model()  # refuse an unsupported hw.json now, not at the first view
        return workspace

    # -- what it is ---------------------------------------------------------------------

    @property
    def id(self) -> str:
        return self.rtos.board_id(self.board_dir)

    @property
    def generated_dir(self) -> Path:
        return self.board_dir / "generated"

    @property
    def hw_json(self) -> Path:
        return self.generated_dir / "hw.json"

    def model(self) -> HwModel:
        try:
            stat = self.hw_json.stat()
        except OSError as exc:
            raise CoreError(
                protocol.INVALID_REQUEST,
                f"The board {self.id} has no generated model yet: generate it first.",
            ) from exc
        stamp = (stat.st_mtime_ns, stat.st_size)
        if self._model is None or stamp != self._model_stamp:
            self._model = HwModel.load(self.hw_json)
            self._model_stamp = stamp
        return self._model

    def status(self, trusted: bool) -> dict[str, Any]:
        status: dict[str, Any] = {
            "kind": self.kind,
            "path": str(self.board_dir),
            "id": self.id,
            "name": self.board_dir.name,
            "rtos": {"root": str(self.rtos.root), "generator": self.generator},
            "trusted": trusted,
            "generated": self.hw_json.is_file(),
        }
        if status["generated"]:
            status["board"] = self.model().summary()
        return status

    # -- running the generator ----------------------------------------------------------

    def _board_arg(self) -> str:
        try:
            return self.board_dir.relative_to(self.rtos.root).as_posix()
        except ValueError:
            return str(self.board_dir)

    def _diagnostics(self, output: str) -> list[dict[str, Any]]:
        return diagnostics.parse(output, self.rtos.root)

    def _failed(self, what: str, proc: Any) -> CoreError:
        return CoreError(
            protocol.TOOL_FAILED,
            f"emb-hwgen {what} failed (exit code {proc.returncode}).",
            {
                "tool": "emb-hwgen",
                "exit_code": proc.returncode,
                "output": proc.stderr + proc.stdout,
            },
        )

    def validate(self) -> dict[str, Any]:
        """The generator's verdict on the board and its SoC, every diagnostic located."""
        proc = self.rtos.run_hwgen("validate", "--board", self._board_arg())
        if proc.returncode not in (0, 2):
            raise self._failed("validate", proc)
        found = self._diagnostics(proc.stderr)
        return {"valid": proc.returncode == 0, "diagnostics": found, **diagnostics.counts(found)}

    def generate(self) -> dict[str, Any]:
        """Writes the board's generated/ and re-reads its model."""
        proc = self.rtos.run_hwgen("generate", "--board", self._board_arg())
        if proc.returncode not in (0, 2):
            raise self._failed("generate", proc)
        found = self._diagnostics(proc.stderr)
        written = proc.returncode == 0
        files = (
            sorted(p.name for p in self.generated_dir.iterdir() if p.is_file()) if written else []
        )
        result: dict[str, Any] = {
            "written": written,
            "files": files,
            "diagnostics": found,
            **diagnostics.counts(found),
        }
        if written:
            result["board"] = self.model().summary()
        return result

    def diff(self) -> dict[str, Any]:
        """Which generated files a generate would change, and how; nothing is written."""
        with tempfile.TemporaryDirectory(prefix="emblink-studio-diff-") as tmp:
            proc = self.rtos.run_hwgen("generate", "--board", self._board_arg(), "--out", tmp)
            if proc.returncode not in (0, 2):
                raise self._failed("generate", proc)
            found = self._diagnostics(proc.stderr)
            if proc.returncode == 2:
                return {
                    "up_to_date": False,
                    "files": [],
                    "diagnostics": found,
                    **diagnostics.counts(found),
                }
            fresh = {p.name: p for p in Path(tmp).iterdir() if p.is_file()}
            current = (
                {p.name: p for p in self.generated_dir.iterdir() if p.is_file()}
                if self.generated_dir.is_dir()
                else {}
            )
            files = [
                self._file_diff(name, current.get(name), fresh.get(name))
                for name in sorted(set(fresh) | set(current))
            ]
        return {
            "up_to_date": all(f["status"] == "unchanged" for f in files),
            "files": files,
            "diagnostics": found,
            **diagnostics.counts(found),
        }

    @staticmethod
    def _file_diff(name: str, have: Path | None, want: Path | None) -> dict[str, Any]:
        if want is None:
            return {"name": name, "status": "removed"}
        if have is None:
            return {"name": name, "status": "added"}
        old = have.read_text(encoding="utf-8", errors="replace")
        new = want.read_text(encoding="utf-8", errors="replace")
        if old == new:
            return {"name": name, "status": "unchanged"}
        lines = list(
            difflib.unified_diff(
                old.splitlines(True), new.splitlines(True), f"{name} (now)", f"{name} (generated)"
            )
        )
        cut = len(lines) > DIFF_LINE_LIMIT
        return {
            "name": name,
            "status": "changed",
            "diff": "".join(lines[:DIFF_LINE_LIMIT]),
            "truncated": cut,
        }


def list_boards(rtos: Rtos) -> list[dict[str, Any]]:
    """Every board of a checkout, with its summary where its model is generated and readable."""
    out = []
    for board_dir in rtos.boards():
        entry: dict[str, Any] = {
            "id": rtos.board_id(board_dir),
            "name": board_dir.name,
            "path": str(board_dir),
            "generated": False,
        }
        hw_json = board_dir / "generated" / "hw.json"
        if hw_json.is_file():
            try:
                entry["board"] = HwModel.load(hw_json).summary()
                entry["generated"] = True
            except CoreError as exc:
                entry["problem"] = exc.message
        out.append(entry)
    return out
