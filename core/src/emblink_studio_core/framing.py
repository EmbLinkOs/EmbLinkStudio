# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    framing.py
# Author:  Junior Deogracias
# Created: 2026-11-11
"""Message framing: JSON bodies behind Content-Length headers, as in LSP (spec section 4).

`encode()` builds one frame and `read_message()` reads one from a binary stream.
FramingError means the stream is out of step and cannot be trusted; its subclass
BodyError means one body was not a JSON object while the stream is still in step.
The desktop's side of the same format is app/src/framing.cpp.
"""

from __future__ import annotations

import json
from typing import Any, BinaryIO


class FramingError(Exception):
    """The byte stream does not hold a well-formed frame; the stream cannot be trusted."""


class BodyError(FramingError):
    """A frame was read whole but its body is not a JSON object; the next frame is readable."""


def encode(message: dict[str, Any]) -> bytes:
    body = json.dumps(message, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    return b"Content-Length: " + str(len(body)).encode("ascii") + b"\r\n\r\n" + body


def read_message(stream: BinaryIO) -> dict[str, Any] | None:
    """Read one message, or return None at a clean end of stream."""
    length: int | None = None
    first = True
    while True:
        line = stream.readline()
        if not line:
            if first:
                return None
            raise FramingError("stream ended inside a header")
        first = False
        if line in (b"\r\n", b"\n"):
            break
        name, sep, value = line.decode("ascii", errors="replace").partition(":")
        if not sep:
            raise FramingError(f"malformed header line: {line!r}")
        if name.strip().lower() == "content-length":
            try:
                length = int(value.strip())
            except ValueError as exc:
                raise FramingError(f"bad Content-Length: {value.strip()!r}") from exc
    if length is None or length < 0:
        raise FramingError("missing Content-Length header")
    body = stream.read(length)
    if len(body) != length:
        raise FramingError("stream ended inside a body")
    try:
        message = json.loads(body.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise BodyError(f"body is not JSON: {exc}") from exc
    if not isinstance(message, dict):
        raise BodyError("body is not a JSON object")
    return message
