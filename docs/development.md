<!-- SPDX-License-Identifier: Apache-2.0 -->
# Development

How to set up a machine, build and test EmbLink Studio, and get a change into `main`.

## What you need

CMake 3.21 or later, Ninja, Qt 6.8 or later, Python 3.11 or later, and clang-format 18 for C++ formatting
(`pip install clang-format==18.1.8` gives the version CI runs). On macOS, Homebrew's `qt`, `cmake`, `ninja` and
`python` are enough. On Linux, also the `libxcb1-dev` package.

The first `cmake --preset dev` downloads Qt Advanced Docking System, pinned by version and SHA-256
in `CMakeLists.txt`, and builds it as a shared library. Later configures reuse it.

## Build and test

```sh
cmake --preset dev            # set CMAKE_PREFIX_PATH to your Qt if CMake can't find it
cmake --build --preset dev
ctest --preset dev            # Qt tests, against the real core and fake ones

python3 -m venv core/.venv && core/.venv/bin/pip install -e "core[dev]"
core/.venv/bin/pytest core    # the core's tests, including the protocol schemas
core/.venv/bin/ruff check . && core/.venv/bin/ruff format --check .
```

The core's acceptance tests run against a real EmbLinkRTOS checkout: set `EMBLINK_STUDIO_TEST_RTOS` to it, as
CI does with the RTOS at a pinned commit; without it they are skipped.

```sh
EMBLINK_STUDIO_TEST_RTOS=~/EmbLinkRTOS core/.venv/bin/pytest core
core/.venv/bin/emblink-studio-core board list ~/EmbLinkRTOS          # every method has a command
core/.venv/bin/emblink-studio-core project validate ~/EmbLinkRTOS/boards/st/nucleo_f446re
```

Make `core/.venv` before the first `cmake --preset dev`: the build then runs the core with that Python, which
has the core's dependencies (PyYAML, for the RTOS's generator). Configured before it existed, run
`cmake --preset dev -DPython3_EXECUTABLE=$PWD/core/.venv/bin/python` once.

Run the application: `build/dev/app/EmbLinkStudio.app/Contents/MacOS/EmbLinkStudio` on macOS,
`build/dev/app/EmbLinkStudio` elsewhere. It runs the core from `core/src` with the Python CMake found; set
`EMBLINK_STUDIO_CORE` to an installed `emblink-studio-core` to use that instead.

Open a board with File › Open Board (⌘O): a board directory of an EmbLinkRTOS checkout, such as
`~/EmbLinkRTOS/boards/st/nucleo_f446re`. Validating and generating run the checkout's generator, so the
application asks once to trust the checkout.

Options for a run: `--open <board>` opens that board, `--language en|fr|zh_CN` uses that language without saving it, `--stage
configure|code|debug|verify` opens on that stage, and `--screenshot <file>` saves a picture of the window and
quits.

## The look and the languages

Every colour, size, font and timing comes from [`docs/design/tokens.json`](design/tokens.json), which the
build compiles into the application; change the look there, not in the C++. The design brief explains the
tokens in section 4f.

The interface is in English, French and Simplified Chinese. Write every visible string in English inside
`tr()`, then refresh the translation files and translate the new strings in Qt Linguist:

```sh
cmake --build --preset dev --target update_translations   # updates app/i18n/*.ts from the sources
linguist app/i18n/emblinkstudio_fr.ts app/i18n/emblinkstudio_zh_CN.ts
```

The build compiles the `.ts` files into the application. A string with no translation shows in English.
Option keys, pin, register and file names, and code are never translated.

## The editor

The repository's [`.clangd`](../.clangd) points clangd at `build/dev/compile_commands.json`, so any
editor that uses clangd (VS Code with the clangd extension, Qt Creator, Neovim, Emacs) finds the Qt headers
and the build's flags. Run `cmake --preset dev` once first; until that file exists, the editor reports headers
as missing.
After the first configure, restart the language server (in VS Code: *clangd: Restart language server*).

`progress-screenshots/` is ignored by git: keep pictures of the application as it grows there, on your
machine only, for example with `--screenshot progress-screenshots/configure-fr.png --language fr`.

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
| `app/i18n/` | The French and Simplified Chinese translations |
| `core/` | EmbLink Studio Core: `emblink-studio-core serve` |
| `protocol/` | JSON Schemas of every method's parameters and result |
| `docs/` | The specification and this guide |
| `docs/design/` | The design brief and the design tokens the application reads |
| `third_party/components.json` | Every third-party component and its licence |
| `tools/notices.py` | Checks those licences and writes the notices list for the About box |
