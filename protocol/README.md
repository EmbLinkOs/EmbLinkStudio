<!-- SPDX-License-Identifier: Apache-2.0 -->
# Protocol schemas

One JSON Schema (draft 2020-12) per method's parameters and result, named
`<method>.params.schema.json` and `<method>.result.schema.json`, plus `error.schema.json`
for every error response. The core's tests (`core/tests/test_schemas.py`) check every
message the core sends against them. The protocol is described in the [specification](../docs/specification.md), §5.

Protocol version: 0.1.0. A change that breaks an existing client bumps the major version.

| Method | Since | What it does |
| --- | --- | --- |
| `initialize`, `shutdown` | S0 | The lifecycle |
| `project.open`, `close`, `status` | S1 | A board of an EmbLinkRTOS checkout is the workspace until the project manifest exists |
| `project.trust` | S1 | Lets the checkout run its generator (spec §9); remembered in the person's settings |
| `project.validate` | S1 | The generator's verdict, each diagnostic with its file, line and YAML path |
| `board.list`, `describe`, `pins`, `clocks`, `memory` | S1 | The generator's resolved model, from `hw.json` |
| `generate.run`, `diff` | S1 | Runs the checkout's `emb-hwgen`; `diff` writes nothing |
