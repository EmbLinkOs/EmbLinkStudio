# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
"""A fake core for the desktop's tests: `fake_core.py <behaviour>`.

wrong-major  answers initialize with protocol 9.0.0
crash        exits with code 3 right after answering initialize
silent-exit  exits with code 0 before answering anything
"""

import json
import sys


def read():
    length = None
    while True:
        line = sys.stdin.buffer.readline()
        if not line:
            return None
        if line in (b"\r\n", b"\n"):
            break
        name, _, value = line.decode().partition(":")
        if name.strip().lower() == "content-length":
            length = int(value)
    return json.loads(sys.stdin.buffer.read(length))


def send(message):
    body = json.dumps(message).encode()
    sys.stdout.buffer.write(b"Content-Length: %d\r\n\r\n" % len(body) + body)
    sys.stdout.buffer.flush()


behaviour = sys.argv[1]
if behaviour == "silent-exit":
    sys.exit(0)
while (message := read()) is not None:
    if message.get("method") == "initialize":
        version = "9.0.0" if behaviour == "wrong-major" else "0.1.0"
        send(
            {
                "jsonrpc": "2.0",
                "id": message["id"],
                "result": {
                    "protocol_version": version,
                    "server": {"name": "embstudio-core", "version": "fake"},
                    "capabilities": {},
                },
            }
        )
        if behaviour == "crash":
            print("fake core: crashing on purpose", file=sys.stderr, flush=True)
            sys.exit(3)
    elif message.get("method") == "shutdown":
        send({"jsonrpc": "2.0", "id": message["id"], "result": None})
    elif message.get("method") == "exit":
        sys.exit(0)
