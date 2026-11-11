#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
"""Check every third-party component's licence and write the notices list (spec section 9).

Fails (exit 1) on a licence outside the allowed list, an LGPL component that is not linked
dynamically, or a runtime dependency of the core that components.json does not list.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import tomllib
from pathlib import Path

ALLOWED = {
    "MIT",
    "BSD-2-Clause",
    "BSD-3-Clause",
    "Apache-2.0",
    "ISC",
    "Zlib",
    "MPL-2.0",
    "PSF-2.0",
    "LGPL-2.1-only",
    "LGPL-2.1-or-later",
    "LGPL-3.0-only",
    "LGPL-3.0-or-later",
}
REQUIRED_FIELDS = ("name", "version", "licence", "url", "used_by")


def check(components: list[dict], runtime_deps: list[str]) -> list[str]:
    problems = []
    for c in components:
        missing = [f for f in REQUIRED_FIELDS if not c.get(f)]
        if missing:
            problems.append(f"{c.get('name', '?')}: missing {', '.join(missing)}")
            continue
        if c["licence"] not in ALLOWED:
            problems.append(f"{c['name']}: licence {c['licence']} is not on the allowed list")
        if c["licence"].startswith("LGPL") and c.get("linkage") != "dynamic":
            problems.append(f"{c['name']}: LGPL code must be linked dynamically")
    listed = {c["name"].lower() for c in components}
    for dep in runtime_deps:
        name = re.split(r"[ <>=!~;\[]", dep, maxsplit=1)[0].lower()
        if name not in listed:
            problems.append(f"core dependency {name} is not listed in components.json")
    return problems


def render(components: list[dict]) -> str:
    lines = [
        "# Third-party notices",
        "",
        "EmbLink Studio is licensed under the Apache License 2.0. It uses the components below,",
        "each under its own licence. LGPL components are linked dynamically and can be",
        "replaced; their sources are available from the links given.",
        "",
        "| Component | Version | Licence | Used by |",
        "| --- | --- | --- | --- |",
    ]
    for c in sorted(components, key=lambda c: c["name"].lower()):
        lines.append(
            f"| [{c['name']}]({c['url']}) | {c['version']} | {c['licence']} | {c['used_by']} |"
        )
    sources = [c for c in components if c.get("source")]
    if sources:
        lines += ["", "## Sources", ""]
        lines += [f"- {c['name']}: {c['source']}" for c in sources]
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--components", type=Path, required=True)
    parser.add_argument("--pyproject", type=Path, required=True)
    parser.add_argument("--output", type=Path, help="where to write the notices (default: none)")
    args = parser.parse_args()

    components = json.loads(args.components.read_text())["components"]
    runtime_deps = tomllib.loads(args.pyproject.read_text())["project"].get("dependencies", [])
    problems = check(components, runtime_deps)
    for problem in problems:
        print(f"notices: {problem}", file=sys.stderr)
    if problems:
        return 1
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(render(components))
    return 0


if __name__ == "__main__":
    sys.exit(main())
