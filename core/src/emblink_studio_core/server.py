# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
"""The JSON-RPC server behind `emblink-studio-core serve` (spec sections 4 and 5).

Stage S0 implements the lifecycle only: initialize, initialized, shutdown, exit.
Standard output carries protocol traffic and nothing else; the log goes to standard error.
"""

from __future__ import annotations

import logging
import sys
from collections.abc import Callable
from typing import Any, BinaryIO

from . import __version__, protocol
from .framing import BodyError, FramingError, encode, read_message

log = logging.getLogger("emblink_studio_core")

Handler = Callable[[Any], Any]


class RpcError(Exception):
    def __init__(self, code: int, message: str, data: Any = None) -> None:
        super().__init__(message)
        self.code = code
        self.message = message
        self.data = data


class Server:
    def __init__(self, stdin: BinaryIO, stdout: BinaryIO) -> None:
        self._in = stdin
        self._out = stdout
        self._initialized = False
        self._shutdown = False
        self._exit_code: int | None = None
        self._requests: dict[str, Handler] = {
            "initialize": self._initialize,
            "shutdown": self._shutdown_request,
        }
        self._notifications: dict[str, Handler] = {
            "initialized": lambda _params: None,
            "exit": self._exit,
            "$/cancelRequest": lambda _params: None,  # no long operations yet
        }

    # -- the loop -----------------------------------------------------------------------

    def run(self) -> int:
        """Serve until `exit`; return the process exit code (0 only after `shutdown`)."""
        while self._exit_code is None:
            try:
                message = read_message(self._in)
            except BodyError as exc:
                self._send_error(None, protocol.PARSE_ERROR, str(exc))
                continue
            except FramingError as exc:
                log.error("unrecoverable framing error: %s", exc)
                return 1
            if message is None:
                log.warning("standard input closed without exit")
                return 1
            self._dispatch(message)
        return self._exit_code

    def _dispatch(self, message: dict[str, Any]) -> None:
        has_id = "id" in message
        msg_id = message.get("id")
        method = message.get("method")
        if message.get("jsonrpc") != "2.0" or not isinstance(method, str):
            if has_id:
                self._send_error(msg_id, protocol.INVALID_REQUEST, "not a JSON-RPC 2.0 request")
            return
        params = message.get("params")
        if not has_id:
            self._notify(method, params)
            return
        try:
            result = self._request(method, params)
        except RpcError as exc:
            self._send_error(msg_id, exc.code, exc.message, exc.data)
        except Exception as exc:  # a bug in a handler must not take the core down
            log.exception("handler for %s failed", method)
            self._send_error(msg_id, protocol.INTERNAL_ERROR, f"internal error: {exc}")
        else:
            self._send({"jsonrpc": "2.0", "id": msg_id, "result": result})

    def _request(self, method: str, params: Any) -> Any:
        handler = self._requests.get(method)
        if self._shutdown:
            raise RpcError(protocol.INVALID_REQUEST, "the core is shutting down")
        if not self._initialized and method != "initialize":
            raise RpcError(protocol.NOT_INITIALIZED, "initialize first")
        if handler is None:
            raise RpcError(protocol.METHOD_NOT_FOUND, f"unknown method: {method}")
        return handler(params)

    def _notify(self, method: str, params: Any) -> None:
        if not self._initialized and method != "exit":
            return
        handler = self._notifications.get(method)
        if handler is None:
            log.debug("ignored notification %s", method)
            return
        try:
            handler(params)
        except Exception:
            log.exception("notification %s failed", method)

    # -- output -------------------------------------------------------------------------

    def _send(self, message: dict[str, Any]) -> None:
        self._out.write(encode(message))
        self._out.flush()

    def _send_error(self, msg_id: Any, code: int, message: str, data: Any = None) -> None:
        error: dict[str, Any] = {"code": code, "message": message}
        if data is not None:
            error["data"] = data
        self._send({"jsonrpc": "2.0", "id": msg_id, "error": error})

    # -- lifecycle ----------------------------------------------------------------------

    def _initialize(self, params: Any) -> dict[str, Any]:
        if self._initialized:
            raise RpcError(protocol.INVALID_REQUEST, "already initialized")
        if not isinstance(params, dict) or not isinstance(params.get("protocol_version"), str):
            raise RpcError(protocol.INVALID_PARAMS, "params.protocol_version must be a string")
        try:
            protocol.major(params["protocol_version"])
        except ValueError as exc:
            raise RpcError(protocol.INVALID_PARAMS, str(exc)) from exc
        client = params.get("client", {})
        log.info("initialize from %s, protocol %s", client, params["protocol_version"])
        self._initialized = True
        return {
            "protocol_version": protocol.PROTOCOL_VERSION,
            "server": {"name": "emblink-studio-core", "version": __version__},
            "capabilities": {},
        }

    def _shutdown_request(self, _params: Any) -> None:
        self._shutdown = True
        return None

    def _exit(self, _params: Any) -> None:
        self._exit_code = 0 if self._shutdown else 1


def serve() -> int:
    logging.basicConfig(
        stream=sys.stderr, level=logging.INFO, format="%(levelname)s %(name)s: %(message)s"
    )
    return Server(sys.stdin.buffer, sys.stdout.buffer).run()
