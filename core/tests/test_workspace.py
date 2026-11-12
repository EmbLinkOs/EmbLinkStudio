# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    test_workspace.py
# Author:  Junior Deogracias
# Created: 2026-11-12
"""A board of a checkout, opened, read, validated and generated (stage S1).

Against the fake checkout of conftest.py: what opens and what is refused, and why;
the formats a core supports; the model's views; validation with located diagnostics;
generating, after which the model is re-read; what diff reports for each kind of
change; a broken or incomplete generator; the boards of a checkout; and the trust
store.
"""

import json

import pytest

from conftest import write_board
from emblink_studio_core import protocol
from emblink_studio_core.errors import CoreError
from emblink_studio_core.rtos import Rtos
from emblink_studio_core.trust import TrustStore
from emblink_studio_core.workspace import Workspace, list_boards


def error_code(call):
    with pytest.raises(CoreError) as caught:
        call()
    return caught.value.code


# -- opening ----------------------------------------------------------------------------


def test_a_generated_board_opens_with_its_model(fake_rtos):
    ws = Workspace.open(fake_rtos / "boards/acme/demo")
    status = ws.status(trusted=False)
    assert status["kind"] == "board"
    assert status["id"] == "acme/demo"
    assert status["rtos"]["root"] == str(fake_rtos.resolve())
    assert status["rtos"]["generator"] == {
        "name": "emb-hwgen",
        "version": "0.1.0",
        "schema_version": 0,
    }
    assert status["generated"] is True
    assert status["board"]["soc"] == "ACME1"
    assert status["board"]["cpu_hz"] == 48000000


def test_a_board_not_generated_yet_opens_without_a_model(fake_rtos):
    ws = Workspace.open(fake_rtos / "boards/acme/blank")
    assert ws.status(trusted=False)["generated"] is False
    assert error_code(ws.model) == protocol.INVALID_REQUEST


def test_what_is_refused(fake_rtos, tmp_path):
    assert error_code(lambda: Workspace.open(tmp_path / "nothing")) == protocol.INVALID_PARAMS
    assert error_code(lambda: Workspace.open(fake_rtos / "boards")) == protocol.INVALID_PARAMS
    project = tmp_path / "project"
    project.mkdir()
    (project / "emb-project.yaml").write_text("schema_version: 0\nkind: project\n")
    with pytest.raises(CoreError) as caught:
        Workspace.open(project)
    assert "phase E1" in caught.value.message
    outside = write_board(tmp_path / "elsewhere", "acme/demo", "name: demo\ncpu_hz: 1\n")
    assert error_code(lambda: Workspace.open(outside)) == protocol.TOOL_MISSING


def test_another_generator_major_is_refused(fake_rtos):
    package = fake_rtos / "hw/gen/emb_hwgen/__init__.py"
    package.write_text('VERSION = "1.0.0"\nSCHEMA_VERSION = 0\n')
    assert (
        error_code(lambda: Workspace.open(fake_rtos / "boards/acme/demo"))
        == protocol.UNSUPPORTED_RTOS
    )


def test_another_hw_json_schema_is_refused(fake_rtos):
    hw_json = fake_rtos / "boards/acme/demo/generated/hw.json"
    data = json.loads(hw_json.read_text())
    data["schema_version"] = 7
    hw_json.write_text(json.dumps(data))
    with pytest.raises(CoreError) as caught:
        Workspace.open(fake_rtos / "boards/acme/demo")
    assert caught.value.code == protocol.UNSUPPORTED_RTOS
    assert "schema 7" in caught.value.message


# -- the model --------------------------------------------------------------------------


def test_the_views_are_the_generators_values(fake_rtos):
    model = Workspace.open(fake_rtos / "boards/acme/demo").model()
    hw = json.loads((fake_rtos / "boards/acme/demo/generated/hw.json").read_text())
    describe = model.describe()
    assert describe["board"]["name"] == "demo"
    assert describe["soc"]["part"] == "ACME1"
    assert describe["devices"] == hw["derived"]["devices"]
    pins = model.pins()
    assert pins["assigned"] == hw["derived"]["pins"]
    assert pins["alternate_functions"]["PA0"] == {"af1": "uart0_tx"}
    assert model.clocks()["resolved"]["cpu_hz"] == 48000000
    assert [r["name"] for r in model.memory()["regions"]] == ["flash", "sram"]


# -- validating and generating ----------------------------------------------------------


def test_a_valid_board(fake_rtos):
    result = Workspace.open(fake_rtos / "boards/acme/demo").validate()
    assert result == {"valid": True, "diagnostics": [], "errors": 0, "warnings": 0}


def test_errors_and_warnings_are_located(fake_rtos):
    board = fake_rtos / "boards/acme/demo"
    (board / "board.yaml").write_text(
        "name: demo\n"
        "clocks:\n"
        "  cpu_hz: 900000000  # error: clocks.cpu_hz: above the 200 MHz the chip allows\n"
        "pins: {}  # warning: pins: no pin assigned\n"
    )
    result = Workspace.open(board).validate()
    assert result["valid"] is False
    assert (result["errors"], result["warnings"]) == (1, 1)
    error = next(d for d in result["diagnostics"] if d["severity"] == "error")
    assert error["file"] == str((board / "board.yaml").resolve())
    assert error["line"] == 3
    assert error["path"] == "clocks.cpu_hz"
    assert error["message"] == "above the 200 MHz the chip allows"


def test_a_crashing_generator_is_a_tool_failure(fake_rtos):
    board = fake_rtos / "boards/acme/demo"
    (board / "board.yaml").write_text("name: demo\ncpu_hz: 1  # crash\n")
    with pytest.raises(CoreError) as caught:
        Workspace.open(board).validate()
    assert caught.value.code == protocol.TOOL_FAILED
    assert "KeyError" in caught.value.data["output"]


def test_a_generator_without_pyyaml_says_what_to_install(fake_rtos):
    board = fake_rtos / "boards/acme/demo"
    (board / "board.yaml").write_text("name: demo\ncpu_hz: 1  # no-yaml\n")
    with pytest.raises(CoreError) as caught:
        Workspace.open(board).validate()
    assert caught.value.code == protocol.TOOL_MISSING
    assert "pip install pyyaml" in caught.value.message


def test_generate_writes_and_the_model_follows(fake_rtos):
    board = fake_rtos / "boards/acme/demo"
    ws = Workspace.open(board)
    assert ws.model().clocks()["resolved"]["cpu_hz"] == 48000000
    (board / "board.yaml").write_text(
        (board / "board.yaml").read_text().replace("48000000", "96000000")
    )
    result = ws.generate()
    assert result["written"] is True
    assert result["files"] == ["hw.json", "hw_config.h"]
    assert result["board"]["cpu_hz"] == 96000000
    assert ws.model().clocks()["resolved"]["cpu_hz"] == 96000000


def test_generate_refused_writes_nothing(fake_rtos):
    board = fake_rtos / "boards/acme/demo"
    before = (board / "generated/hw.json").read_text()
    (board / "board.yaml").write_text("name: demo\ncpu_hz: 1  # error: cpu_hz: too slow\n")
    result = Workspace.open(board).generate()
    assert result["written"] is False
    assert result["errors"] == 1
    assert (board / "generated/hw.json").read_text() == before


def test_diff_reports_each_kind_of_change(fake_rtos):
    board = fake_rtos / "boards/acme/demo"
    ws = Workspace.open(board)
    assert ws.diff()["up_to_date"] is True
    (board / "generated/hw_config.h").write_text("/* edited by hand */\n")
    (board / "generated/notes.txt").write_text("not an output\n")
    result = ws.diff()
    assert result["up_to_date"] is False
    by_name = {f["name"]: f for f in result["files"]}
    assert by_name["hw.json"]["status"] == "unchanged"
    assert by_name["hw_config.h"]["status"] == "changed"
    assert "+#define EMB_CPU_HZ 48000000" in by_name["hw_config.h"]["diff"]
    assert by_name["notes.txt"]["status"] == "removed"
    (board / "generated/hw.json").unlink()
    assert {f["name"]: f["status"] for f in ws.diff()["files"]}["hw.json"] == "added"
    # diff never writes
    assert not (board / "generated/hw.json").exists()


def test_the_boards_of_a_checkout(fake_rtos):
    boards = list_boards(Rtos.find(fake_rtos))
    assert [b["id"] for b in boards] == ["acme/blank", "acme/demo"]
    assert boards[0]["generated"] is False
    assert boards[1]["board"]["name"] == "demo"


# -- trust ------------------------------------------------------------------------------


def test_trust_is_kept_per_path_and_survives(tmp_path):
    store = TrustStore(tmp_path / "config")
    checkout = tmp_path / "rtos"
    checkout.mkdir()
    assert not store.is_trusted(checkout)
    store.trust(checkout)
    assert TrustStore(tmp_path / "config").is_trusted(checkout / "." / "")
    assert not store.is_trusted(tmp_path)


def test_a_damaged_trust_file_trusts_nothing(tmp_path):
    (tmp_path / "trust.json").write_text("{ not json")
    store = TrustStore(tmp_path)
    assert not store.is_trusted(tmp_path)
    store.trust(tmp_path)
    assert store.is_trusted(tmp_path)
