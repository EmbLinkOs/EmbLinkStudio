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

- Every source file starts with the file header below: the SPDX licence line and the copyright
  line come first, then the project line, then what the file is for, who wrote it and when.
  The date is the day the file was created; it does not change when the file does.

  C++ (`.h`, `.cpp`), a Doxygen block:

  ```cpp
  // SPDX-License-Identifier: Apache-2.0
  // Copyright 2026 Your Name
  //
  // This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
  // Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

  /**
   * @file    example.cpp
   * @brief   One line: what the file holds.
   *
   * @details
   * A few lines: what it does, its rules and its edge cases.
   *
   * @see     example.h
   * @author  Your Name
   * @date    YYYY-MM-DD (created)
   */
  ```

  Python: the same lines as comments, and the description in the module docstring.

  ```python
  # SPDX-License-Identifier: Apache-2.0
  # Copyright 2026 Your Name
  #
  # This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
  # Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
  #
  # File:    example.py
  # Author:  Your Name
  # Created: YYYY-MM-DD
  """One line: what the module holds.

  A few lines: what it does, its rules and its edge cases.
  """
  ```

  Build and configuration files (CMake, TOML, YAML) carry the first five lines and one or two
  lines on what the file is for. Markdown files carry the SPDX line only; JSON files, which
  cannot hold comments, carry none.
- C++ is formatted with clang-format 18 (`.clang-format`), Python with ruff (`ruff.toml`).
  `pip install clang-format==18.1.8` gives the version CI runs.
- A new third-party component is added to `third_party/components.json` in the same pull
  request. Only the licences in `tools/notices.py` are allowed; no GPL libraries in the
  application, and LGPL ones only linked dynamically.
- A change to the protocol updates its schema in `protocol/` and the tests on both sides.
- Knowledge of hardware stays in EmbLinkRTOS's tools: the application only shows and edits.
