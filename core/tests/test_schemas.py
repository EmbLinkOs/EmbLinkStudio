# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# File:    test_schemas.py
# Author:  Junior Deogracias
# Created: 2026-11-11
"""Every message the core sends matches the schemas in protocol/ (spec section 9, Tests).

Checks that the schemas themselves are valid, then validates the initialize params,
the lifecycle results and the error responses against them.
"""

import jsonschema

from conftest import INIT, frames, notification, request, run_core


def test_schemas_are_valid(schemas):
    assert schemas, "no schemas found in protocol/"
    for schema in schemas.values():
        jsonschema.Draft202012Validator.check_schema(schema)


def test_initialize_params_we_send_are_valid(schemas):
    jsonschema.validate(INIT["params"], schemas["initialize.params.schema.json"])


def test_lifecycle_results_match(schemas):
    _, out, _ = run_core(frames(INIT, request(2, "shutdown"), notification("exit")))
    jsonschema.validate(out[0]["result"], schemas["initialize.result.schema.json"])
    jsonschema.validate(out[1]["result"], schemas["shutdown.result.schema.json"])


def test_errors_match(schemas):
    _, out, _ = run_core(
        frames(
            request(1, "early"),
            INIT,
            request(2, "no.such"),
            request(3, "initialize", {}),
            notification("exit"),
        )
    )
    errors = [m for m in out if "error" in m]
    assert len(errors) == 3
    for message in errors:
        jsonschema.validate(message, schemas["error.schema.json"])
