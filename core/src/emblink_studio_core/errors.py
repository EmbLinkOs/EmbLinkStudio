# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    errors.py
# Author:  Junior Deogracias
# Created: 2026-11-12
"""The core's error, shared by the server, the command line and the project code.

A CoreError carries one of the protocol's error codes (spec section 5, Errors), a
message written for the person, and optional data: the output of the tool that
failed and, where it applies, the file and line. The server answers it as a JSON-RPC
error; the command line prints its message and exits with 1.
"""

from __future__ import annotations

from typing import Any


class CoreError(Exception):
    def __init__(self, code: int, message: str, data: Any = None) -> None:
        super().__init__(message)
        self.code = code
        self.message = message
        self.data = data
