# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    __main__.py
# Author:  Junior Deogracias
# Created: 2026-11-11
"""Run the core as a module: `python -m emblink_studio_core <command>`.

This is how the desktop starts the core from a source tree (CoreClient::defaultLaunch);
it does the same as the installed `emblink-studio-core` command.
"""

import sys

from .cli import main

sys.exit(main())
