<!-- SPDX-License-Identifier: Apache-2.0 -->
# Contributing to EmbLink Studio

EmbLink Studio is licensed under the Apache License, Version 2.0 (see `LICENSE`).

## Two agreements

- **Contributor licence agreement (CLA).** Signed once, before your first contribution is
  merged. It grants the project the rights it needs to offer EmbLink Studio under other terms later.
  The agreement is being prepared and checked by a lawyer; see [CLA.md](CLA.md). Until it is
  published, no outside contribution can be merged.
- **Developer Certificate of Origin (DCO).** Every commit carries a `Signed-off-by` line
  matching its author, certifying the [DCO](DCO). Add it with `git commit -s`. CI checks it.

## Rules

- Every source file starts with an SPDX header (`SPDX-License-Identifier: Apache-2.0`) and a
  copyright line.
- C++ is formatted with clang-format 18 (`.clang-format`), Python with ruff (`ruff.toml`).
  `pip install clang-format==18.1.8` gives the version CI runs.
- A new third-party component is added to `third_party/components.json` in the same pull
  request. Only the licences in `tools/notices.py` are allowed; no GPL libraries in the
  application, and LGPL ones only linked dynamically.
- A change to the protocol updates its schema in `protocol/` and the tests on both sides.
- Knowledge of hardware stays in EmbLinkRTOS's tools: the application only shows and edits.
