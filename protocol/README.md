<!-- SPDX-License-Identifier: Apache-2.0 -->
# Protocol schemas

One JSON Schema (draft 2020-12) per method's parameters and result, named
`<method>.params.schema.json` and `<method>.result.schema.json`, plus `error.schema.json`
for every error response. The core's tests (`core/tests/test_schemas.py`) check every
message the core sends against them. The protocol is described in the [specification](../docs/specification.md), §5.

Protocol version: 0.1.0. A change that breaks an existing client bumps the major version.
