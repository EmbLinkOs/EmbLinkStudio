<!-- SPDX-License-Identifier: Apache-2.0 -->
# EmbLink Studio

The IDE made for EmbLinkRTOS: a native Qt 6 desktop application (`app/`) in front of
EmbLink Studio Core, a Python engine (`core/`) that it talks to over JSON-RPC 2.0 on stdio.
Stage S0 (setup) of the [specification](docs/specification.md): the application starts the core, initializes it,
shows its log, and offers a restart if it stops.

## Build and test

Needs CMake 3.21+, Ninja, Qt 6.8 or later, Python 3.11 or later.

```sh
cmake --preset dev            # set CMAKE_PREFIX_PATH to your Qt if CMake can't find it
cmake --build --preset dev
ctest --preset dev            # Qt tests, against the real core and fake ones

python3 -m venv core/.venv && core/.venv/bin/pip install -e "core[dev]"
core/.venv/bin/pytest core    # the core's tests, including the protocol schemas
```

Run the app: `build/dev/app/EmbLinkStudio.app/Contents/MacOS/EmbLinkStudio` on macOS,
`build/dev/app/EmbLinkStudio` elsewhere. It runs the core from `core/src` with the Python CMake
found; set `EMBLINK_STUDIO_CORE` to an installed `emblink-studio-core` to use that instead.

## Layout

| Path | What |
| --- | --- |
| `app/` | The Qt application and its tests |
| `core/` | EmbLink Studio Core: `emblink-studio-core serve` |
| `protocol/` | JSON Schemas of every method's parameters and result |
| `docs/` | The [specification](docs/specification.md) and the [development guide](docs/development.md) |
| `third_party/components.json` | Every third-party component and its licence |
| `tools/notices.py` | Checks those licences and writes the notices list for the About box |

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Licensed under the Apache License 2.0 ([LICENSE](LICENSE)).
