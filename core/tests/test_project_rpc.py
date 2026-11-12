# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    test_project_rpc.py
# Author:  Junior Deogracias
# Created: 2026-11-12
"""The S1 methods over JSON-RPC, as the desktop calls them (spec sections 5 and 9).

Runs the core as a child process against the fake checkout: a call before any board
is open answers no_project; running the generator answers untrusted until the
checkout is trusted; then every method answers, and every result and error matches
its schema in protocol/. Trust is remembered from one core process to the next.
"""

import jsonschema

from conftest import INIT, frames, notification, request, run_core

METHODS = [
    "project.status",
    "project.validate",
    "board.list",
    "board.describe",
    "board.pins",
    "board.clocks",
    "board.memory",
    "generate.diff",
    "generate.run",
]


def by_id(out):
    return {m["id"]: m for m in out if "id" in m}


def check_result(schemas, method, message):
    assert "result" in message, message
    jsonschema.validate(message["result"], schemas[f"{method}.result.schema.json"])


def test_a_session_on_a_board(fake_rtos, config_dir, schemas):
    board = str(fake_rtos / "boards/acme/demo")
    env = {"EMBLINK_STUDIO_CONFIG_DIR": str(config_dir)}
    calls = [
        INIT,
        request(2, "board.describe"),  # nothing open yet
        request(3, "project.open", {"path": board}),
        request(4, "project.validate"),  # not trusted yet
        request(5, "generate.run"),
        request(6, "project.trust"),
    ]
    calls += [request(10 + i, m) for i, m in enumerate(METHODS)]
    calls += [
        request(30, "project.close"),
        request(31, "board.pins"),
        request(32, "project.open", {"path": str(fake_rtos / "boards")}),
        request(33, "shutdown"),
        notification("exit"),
    ]
    jsonschema.validate(calls[2]["params"], schemas["project.open.params.schema.json"])
    code, out, log = run_core(frames(*calls), env)
    assert code == 0, log
    answers = by_id(out)

    for msg_id, error in ((2, 1002), (4, 1003), (5, 1003), (31, 1002), (32, -32602)):
        assert answers[msg_id]["error"]["code"] == error, answers[msg_id]
        jsonschema.validate(answers[msg_id], schemas["error.schema.json"])

    check_result(schemas, "project.open", answers[3])
    assert answers[3]["result"]["trusted"] is False
    check_result(schemas, "project.trust", answers[6])
    assert answers[6]["result"]["trusted"] is True

    for i, method in enumerate(METHODS):
        check_result(schemas, method, answers[10 + i])
    assert answers[11]["result"]["valid"] is True
    assert [b["id"] for b in answers[12]["result"]["boards"]] == ["acme/blank", "acme/demo"]
    assert answers[15]["result"]["resolved"]["cpu_hz"] == 48000000
    assert answers[17]["result"]["up_to_date"] is True
    assert answers[18]["result"]["written"] is True
    assert answers[30]["result"] is None
    jsonschema.validate(answers[30]["result"], schemas["project.close.result.schema.json"])


def test_trust_is_remembered_by_the_next_core(fake_rtos, config_dir):
    board = str(fake_rtos / "boards/acme/demo")
    env = {"EMBLINK_STUDIO_CONFIG_DIR": str(config_dir)}
    run_core(
        frames(
            INIT,
            request(2, "project.open", {"path": board}),
            request(3, "project.trust"),
            request(4, "shutdown"),
            notification("exit"),
        ),
        env,
    )
    _, out, _ = run_core(
        frames(
            INIT,
            request(2, "project.open", {"path": board}),
            request(3, "project.validate"),
            request(4, "shutdown"),
            notification("exit"),
        ),
        env,
    )
    answers = by_id(out)
    assert answers[2]["result"]["trusted"] is True
    assert answers[3]["result"]["valid"] is True


def test_board_list_of_a_checkout_with_nothing_open(fake_rtos, config_dir, schemas):
    _, out, _ = run_core(
        frames(
            INIT,
            request(2, "board.list", {"rtos": str(fake_rtos)}),
            request(3, "board.list", {"rtos": str(config_dir)}),
            request(4, "shutdown"),
            notification("exit"),
        ),
        {"EMBLINK_STUDIO_CONFIG_DIR": str(config_dir)},
    )
    answers = by_id(out)
    check_result(schemas, "board.list", answers[2])
    assert len(answers[2]["result"]["boards"]) == 2
    assert answers[3]["error"]["code"] == 1011
