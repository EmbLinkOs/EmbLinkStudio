# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    test_diagnostics.py
# Author:  Junior Deogracias
# Created: 2026-11-12
"""The generator's lines become located diagnostics (the RTOS's rule HW-002).

Lines are taken from emb-hwgen's real output: an error with a YAML path, a warning, a
file outside the checkout, the document's root, a Windows drive letter; the summary
is dropped and a line in no known form is kept with its message.
"""

from pathlib import Path

from emblink_studio_core import diagnostics

ROOT = Path("/rtos")


def test_an_error_with_its_location():
    [d] = diagnostics.parse(
        "boards/st/nucleo_f446re/board.yaml:25: pins.usart2.tx: PA2 af9 is not usart2_tx "
        "(PA2 offers af7 usart2_tx)\n",
        ROOT,
    )
    assert d == {
        "severity": "error",
        "source": "emb-hwgen",
        "file": str(ROOT / "boards/st/nucleo_f446re/board.yaml"),
        "line": 25,
        "path": "pins.usart2.tx",
        "message": "PA2 af9 is not usart2_tx (PA2 offers af7 usart2_tx)",
    }


def test_a_warning_and_the_summary():
    found = diagnostics.parse(
        "warning: soc/st/stm32f4/stm32f4.yaml:17: cpu.mpu: the cortex_m port does not use the "
        "MPU yet\n"
        "emb-hwgen: /rtos/boards/st/nucleo_f446re is valid\n",
        ROOT,
    )
    assert len(found) == 1
    assert found[0]["severity"] == "warning"
    assert found[0]["path"] == "cpu.mpu"
    assert diagnostics.counts(found) == {"errors": 0, "warnings": 1}


def test_files_outside_the_checkout_stay_absolute():
    [d] = diagnostics.parse(
        "/home/me/boards/st/x/board.yaml:7: name: 'nucleo_f446re' differs from its directory 'x'",
        ROOT,
    )
    assert d["file"] == str(Path("/home/me/boards/st/x/board.yaml"))
    assert d["path"] == "name"


def test_the_document_root():
    [d] = diagnostics.parse(
        "warning: boards/rpi/pico2_w/board.yaml:1: (root): uncurated description", ROOT
    )
    assert d["path"] == "(root)"
    assert d["line"] == 1


def test_a_windows_drive_letter_is_part_of_the_file():
    [d] = diagnostics.parse(r"C:\rtos\boards\x\board.yaml:12: clocks.pll: out of range", ROOT)
    assert d["file"].endswith(r"board.yaml")
    assert d["line"] == 12
    assert d["path"] == "clocks.pll"


def test_a_message_without_a_path():
    [d] = diagnostics.parse("boards/x/board.yaml:3: something is off here", ROOT)
    assert d["message"] == "something is off here"
    assert "path" not in d


def test_an_unknown_line_is_kept():
    found = diagnostics.parse("\nthe generator said something new\nemb-hwgen: 1 error(s)\n", ROOT)
    assert found == [
        {"severity": "error", "source": "emb-hwgen", "message": "the generator said something new"}
    ]
