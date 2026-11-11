# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    test_server.py
# Author:  Junior Deogracias
# Created: 2026-11-11
"""The S0 lifecycle, run as a real child process the way the desktop runs it.

Covers initialize/shutdown/exit, the exit code when shutdown is skipped or standard
input closes, requests before initialize, unknown methods and bad params, refusals
after shutdown, recovery from a bad body, and that standard output holds only frames.
"""

from conftest import INIT, frames, notification, request, run_core
from emblink_studio_core import protocol

SHUTDOWN = request(2, "shutdown")
EXIT = notification("exit")


def test_initialize_shutdown_exit():
    code, out, err = run_core(frames(INIT, notification("initialized"), SHUTDOWN, EXIT))
    assert code == 0, err
    assert [m["id"] for m in out] == [1, 2]
    result = out[0]["result"]
    assert result["protocol_version"] == protocol.PROTOCOL_VERSION
    assert result["server"]["name"] == "emblink-studio-core"
    assert out[1]["result"] is None


def test_exit_without_shutdown_is_code_1():
    code, _, _ = run_core(frames(INIT, EXIT))
    assert code == 1


def test_closed_stdin_is_code_1():
    code, out, _ = run_core(frames(INIT))
    assert code == 1
    assert "result" in out[0]


def test_request_before_initialize():
    code, out, _ = run_core(frames(request(7, "project.status"), EXIT))
    assert out == [
        {
            "jsonrpc": "2.0",
            "id": 7,
            "error": {"code": protocol.NOT_INITIALIZED, "message": "initialize first"},
        }
    ]
    assert code == 1


def test_unknown_method_and_bad_params():
    code, out, _ = run_core(
        frames(
            request(1, "initialize", {"protocol_version": 3}),
            INIT,
            request(2, "no.such"),
            SHUTDOWN,
            EXIT,
        )
    )
    assert code == 0
    assert out[0]["error"]["code"] == protocol.INVALID_PARAMS
    assert "result" in out[1]
    assert out[2]["error"]["code"] == protocol.METHOD_NOT_FOUND


def test_second_initialize_and_requests_after_shutdown_are_refused():
    code, out, _ = run_core(
        frames(
            INIT,
            request(5, "initialize", INIT["params"]),
            SHUTDOWN,
            request(6, "initialize", INIT["params"]),
            EXIT,
        )
    )
    assert code == 0
    assert out[1]["error"]["code"] == protocol.INVALID_REQUEST
    assert out[3]["error"]["code"] == protocol.INVALID_REQUEST


def test_bad_body_is_a_parse_error_and_the_core_carries_on():
    bad = b"Content-Length: 5\r\n\r\n{nope"
    code, out, _ = run_core(frames(INIT) + bad + frames(SHUTDOWN, EXIT))
    assert code == 0
    assert out[1] == {
        "jsonrpc": "2.0",
        "id": None,
        "error": {"code": protocol.PARSE_ERROR, "message": out[1]["error"]["message"]},
    }
    assert out[2]["id"] == 2


def test_stdout_holds_only_frames():
    # Anything the core logs must go to stderr, or the desktop's parser breaks.
    code, out, err = run_core(frames(INIT, SHUTDOWN, EXIT))
    assert code == 0 and len(out) == 2
    assert "initialize from" in err
