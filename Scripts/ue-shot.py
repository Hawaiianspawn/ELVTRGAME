#!/usr/bin/env python
"""ue-shot.py — capture the active editor/PIE window via SlateInspector to a PNG.

Usage: python ue-shot.py <out.png>
"""
import importlib.util
import base64
import json
import sys

spec = importlib.util.spec_from_file_location("uemcp", "ue-mcp-call.py")
uemcp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(uemcp)

import http.client

conn = http.client.HTTPConnection(uemcp.HOST, uemcp.PORT, timeout=60)
sid, _ = uemcp.rpc(conn, None, "initialize", {
    "protocolVersion": "2024-11-05", "capabilities": {},
    "clientInfo": {"name": "ue-shot", "version": "0"}}, rid=1)
uemcp.rpc(conn, sid, "notifications/initialized")
_, resp = uemcp.rpc(conn, sid, "tools/call", {"name": "call_tool", "arguments": {
    "toolset_name": "SlateInspectorToolset.SlateInspectorToolset",
    "tool_name": "Screenshot",
    "arguments": {"ref": ""}}}, rid=2)

out = sys.argv[1] if len(sys.argv) > 1 else "ue-shot.png"
result = resp.get("result", resp)
payload = None
for part in result.get("content", []):
    if part.get("type") == "text":
        payload = json.loads(part["text"])
if not payload or "returnValue" not in payload:
    sys.exit("no image in response: " + json.dumps(result)[:400])
rv = payload["returnValue"]
data = rv["data"] if isinstance(rv, dict) else rv
with open(out, "wb") as f:
    f.write(base64.b64decode(data))
print(out)
