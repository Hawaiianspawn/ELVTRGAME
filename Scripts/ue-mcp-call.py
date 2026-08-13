#!/usr/bin/env python
"""ue-mcp-call.py — one-shot driver for the Unreal MCP meta-server at 127.0.0.1:9000.

Usage:
  python ue-mcp-call.py list-toolsets
  python ue-mcp-call.py describe <toolset_name>
  python ue-mcp-call.py call <tool_name> '<json-args>' [toolset_name]

Prints the tool result text to stdout.

The server answers tools/call with a held-open SSE stream (no content-length),
so responses are read line-by-line via http.client until the data: payload
arrives — a plain read() blocks forever.
"""
import http.client
import json
import sys

HOST, PORT = "127.0.0.1", 9000


def rpc(conn, session, method, params=None, rid=None):
    msg = {"jsonrpc": "2.0", "method": method}
    if params is not None:
        msg["params"] = params
    if rid is not None:
        msg["id"] = rid
    headers = {"Content-Type": "application/json",
               "Accept": "application/json, text/event-stream"}
    if session:
        headers["mcp-session-id"] = session
    conn.request("POST", "/mcp", json.dumps(msg), headers)
    r = conn.getresponse()
    sid = r.getheader("mcp-session-id") or session
    ctype = r.getheader("content-type") or ""
    if "text/event-stream" in ctype:
        while True:
            line = r.fp.readline()
            if not line:
                return sid, {}
            line = line.decode(errors="replace").strip()
            if line.startswith("data:") and line[5:].strip():
                return sid, json.loads(line[5:].strip())
    return sid, json.loads(r.read().decode() or "{}")


def main():
    conn = http.client.HTTPConnection(HOST, PORT, timeout=60)
    sid, _ = rpc(conn, None, "initialize", {
        "protocolVersion": "2024-11-05", "capabilities": {},
        "clientInfo": {"name": "ue-mcp-call", "version": "0"}}, rid=1)
    rpc(conn, sid, "notifications/initialized")

    cmd = sys.argv[1]
    if cmd == "list-toolsets":
        _, resp = rpc(conn, sid, "tools/call", {"name": "list_toolsets", "arguments": {}}, rid=2)
    elif cmd == "describe":
        _, resp = rpc(conn, sid, "tools/call", {"name": "describe_toolset",
                      "arguments": {"toolset_name": sys.argv[2]}}, rid=2)
    elif cmd == "call":
        args = json.loads(sys.argv[3]) if len(sys.argv) > 3 else {}
        call = {"name": "call_tool", "arguments": {"tool_name": sys.argv[2], "arguments": args}}
        if len(sys.argv) > 4:
            call["arguments"]["toolset_name"] = sys.argv[4]
        _, resp = rpc(conn, sid, "tools/call", call, rid=2)
    else:
        sys.exit(__doc__)

    result = resp.get("result", resp)
    content = result.get("content")
    if not content:
        print(json.dumps(result, indent=1)[:4000])
        return
    for part in content:
        if part.get("type") == "text":
            print(part["text"])
    if result.get("isError"):
        sys.exit(1)


if __name__ == "__main__":
    main()
