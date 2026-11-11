<!-- SPDX-License-Identifier: Apache-2.0 -->
# Development

How to set up a machine, build and test EmbLink Studio, and get a change into `main`.

## What you need

CMake 3.21 or later, Ninja, Qt 6.8 or later, Python 3.11 or later, and clang-format 18 for C++ formatting
(`pip install clang-format==18.1.8` gives the version CI runs). On macOS, Homebrew's `qt`, `cmake`, `ninja` and
`python` are enough.

## Build and test

```sh
cmake --preset dev            # set CMAKE_PREFIX_PATH to your Qt if CMake can't find it
cmake --build --preset dev
ctest --preset dev            # Qt tests, against the real core and fake ones

python3 -m venv core/.venv && core/.venv/bin/pip install -e "core[dev]"
core/.venv/bin/pytest core    # the core's tests, including the protocol schemas
core/.venv/bin/ruff check . && core/.venv/bin/ruff format --check .
```

Run the application: `build/dev/app/EmbLinkStudio.app/Contents/MacOS/EmbLinkStudio` on macOS,
`build/dev/app/EmbLinkStudio` elsewhere. It runs the core from `core/src` with the Python CMake found; set
`EMBLINK_STUDIO_CORE` to an installed `emblink-studio-core` to use that instead.

## The editor

The repository's [`.clangd`](../.clangd) points clangd at `build/dev/compile_commands.json`, so any
editor that uses clangd (VS Code with the clangd extension, Qt Creator, Neovim, Emacs) finds the Qt headers
and the build's flags. Run `cmake --preset dev` once first; until that file exists, the editor reports headers
as missing.
After the first configure, restart the language server (in VS Code: *clangd: Restart language server*).

`progress-screenshots/` is ignored by git: keep pictures of the application as it grows there, on your
machine only.

## How a change reaches `main`

`main` takes changes only through pull requests, and only when every CI check passes:

1. Create a branch from `main`.
2. Commit with `git commit -s`. The DCO check requires a `Signed-off-by` line that matches the commit's author.
3. Give every new source file the header shown in [CONTRIBUTING.md](../CONTRIBUTING.md).
4. Push the branch and open a pull request.
5. Wait for the 11 checks: the core on three systems and two Python versions, the application on three
   systems, clang-format, and the DCO sign-off.
6. Merge with *Squash and merge* or *Rebase and merge*. Merge commits are off, so the history stays linear.

Release tags (`v*`) can only be created by the maintainers, and can never be moved or deleted.

## Where things are

| Path | What |
| --- | --- |
| `app/` | The Qt application and its tests |
| `core/` | EmbLink Studio Core: `emblink-studio-core serve` |
| `protocol/` | JSON Schemas of every method's parameters and result |
| `docs/` | The specification and this guide |
| `third_party/components.json` | Every third-party component and its licence |
| `tools/notices.py` | Checks those licences and writes the notices list for the About box |
