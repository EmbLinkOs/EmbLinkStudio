# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
import io

import pytest

from emblink_studio_core.framing import BodyError, FramingError, encode, read_message


def test_round_trip_keeps_unicode():
    message = {"jsonrpc": "2.0", "method": "log", "params": {"text": "µs, °C, 温度"}}
    assert read_message(io.BytesIO(encode(message))) == message


def test_content_length_counts_bytes_not_characters():
    frame = encode({"t": "é"})
    header, body = frame.split(b"\r\n\r\n", 1)
    assert int(header.split(b":")[1]) == len(body)


def test_clean_end_of_stream_is_none():
    assert read_message(io.BytesIO(b"")) is None


def test_extra_headers_and_case_are_accepted():
    body = b'{"a":1}'
    frame = b"content-type: application/json\r\nCONTENT-LENGTH: 7\r\n\r\n" + body
    assert read_message(io.BytesIO(frame)) == {"a": 1}


def test_two_frames_in_one_stream():
    stream = io.BytesIO(encode({"n": 1}) + encode({"n": 2}))
    assert read_message(stream) == {"n": 1}
    assert read_message(stream) == {"n": 2}
    assert read_message(stream) is None


@pytest.mark.parametrize(
    "data",
    [
        b"Content-Length: 10\r\n",
        b"Content-Length: 10\r\n\r\n{}",
        b"X: 1\r\n\r\n",
        b"garbage\r\n\r\n",
    ],
)
def test_broken_frames_raise(data):
    with pytest.raises(FramingError):
        read_message(io.BytesIO(data))


def test_bad_body_leaves_the_stream_readable():
    stream = io.BytesIO(b"Content-Length: 3\r\n\r\nnot" + encode({"ok": True}))
    with pytest.raises(BodyError):
        read_message(stream)
    assert read_message(stream) == {"ok": True}
