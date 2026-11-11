# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    __init__.py
# Author:  Junior Deogracias
# Created: 2026-11-11
"""EmbLink Studio Core: the engine behind the EmbLink Studio desktop application.

The desktop runs the core as a child process (`emblink-studio-core serve`) and talks
JSON-RPC 2.0 to it over standard input and output. `__version__` is the core's own
version; the protocol version it speaks is `protocol.PROTOCOL_VERSION`.
"""

__version__ = "0.1.0.dev0"
