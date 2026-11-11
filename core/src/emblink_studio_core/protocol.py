# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
"""Protocol constants shared with the desktop (spec section 5)."""

from __future__ import annotations

PROTOCOL_VERSION = "0.1.0"

# JSON-RPC 2.0's own codes.
PARSE_ERROR = -32700
INVALID_REQUEST = -32600
METHOD_NOT_FOUND = -32601
INVALID_PARAMS = -32602
INTERNAL_ERROR = -32603

# EmbLink Studio's codes (spec section 5, Errors).
NOT_INITIALIZED = 1001
NO_PROJECT = 1002
UNTRUSTED = 1003
CANCELLED = 1004
TOOL_FAILED = 1010
TOOL_MISSING = 1011
CONFLICT = 1020
UNSUPPORTED_RTOS = 1030


def major(version: str) -> int:
    """The major part of a semantic version string."""
    head = version.split(".", 1)[0]
    if not head.isdigit():
        raise ValueError(f"not a semantic version: {version!r}")
    return int(head)
