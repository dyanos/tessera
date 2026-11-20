"""End-to-end smoke test for tessera-lsp over stdio.

    python scripts/smoke_lsp.py <path-to-tessera-lsp> <workspace-root>

Opens a scratch file with a parse error, waits for tess diagnostics, then
fixes it to a type error, and exercises symbols, hover, completion,
definition, signature help and semantic tokens. Exits non-zero on failure.
"""

import json
import os
import pathlib
import subprocess
import sys
import tempfile
import threading
import queue
import time

server_path, root = sys.argv[1], pathlib.Path(sys.argv[2]).resolve()
proc = subprocess.Popen([server_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
inbox: "queue.Queue[dict]" = queue.Queue()


def reader():
    out = proc.stdout
    while True:
        headers = {}
        while True:
            line = out.readline()
            if not line:
                return
            line = line.decode().strip()
            if not line:
                break
            k, v = line.split(":", 1)
            headers[k.lower()] = v.strip()
        body = out.read(int(headers["content-length"]))
        inbox.put(json.loads(body))


threading.Thread(target=reader, daemon=True).start()
next_id = 0


def send(msg):
    data = json.dumps(msg).encode()
    proc.stdin.write(b"Content-Length: %d\r\n\r\n" % len(data) + data)
    proc.stdin.flush()


def request(method, params):
    global next_id
    next_id += 1
    send({"jsonrpc": "2.0", "id": next_id, "method": method, "params": params})
    deadline = time.time() + 20
    while time.time() < deadline:
        msg = inbox.get(timeout=20)
        if msg.get("id") == next_id:
            if "error" in msg:
                raise RuntimeError(f"{method}: {msg['error']}")
            return msg["result"]
        pending.append(msg)
    raise TimeoutError(method)


def notify(method, params):
    send({"jsonrpc": "2.0", "method": method, "params": params})


pending: list = []


def wait_diagnostics(uri, pred, timeout=60):
    deadline = time.time() + timeout
    while time.time() < deadline:
        while pending:
            msg = pending.pop(0)
            if msg.get("method") == "textDocument/publishDiagnostics" and msg["params"]["uri"] == uri and pred(msg["params"]["diagnostics"]):
                return msg["params"]["diagnostics"]
        try:
            msg = inbox.get(timeout=1)
        except queue.Empty:
            continue
        if msg.get("method") == "window/logMessage":
            print("  log:", msg["params"]["message"])
        pending.append(msg)
    raise TimeoutError("diagnostics")


failures = []


def check(cond, what):
    print(("  ok   " if cond else "  FAIL ") + what)
    if not cond:
        failures.append(what)


tmpdir = pathlib.Path(tempfile.mkdtemp(prefix="tessera-smoke-"))
lib = tmpdir / "geometry.csec"
lib.write_text("// Squared length.\ndef norm2(x: Double, y: Double): Double {\n    return x * x + y * y;\n}\n", encoding="utf-8")
main = tmpdir / "main.csec"
broken = 'import "geometry";\n\ndef main(): Int {\n    val n: Double = norm2(3.0, 4.0)\n    val bad = (n + ;\n    return 0;\n}\n'
main.write_text(broken, encoding="utf-8")
uri = main.as_uri()

init = request("initialize", {
    "processId": os.getpid(),
    "rootUri": root.as_uri(),
    "capabilities": {"textDocument": {"completion": {"completionItem": {"snippetSupport": True}}}},
    "workspaceFolders": [{"uri": root.as_uri(), "name": "root"}],
})
check("semanticTokensProvider" in init["capabilities"], "initialize advertises semantic tokens")
notify("initialized", {})
notify("textDocument/didOpen", {"textDocument": {"uri": uri, "languageId": "tessera", "version": 1, "text": broken}})

diags = wait_diagnostics(uri, lambda d: any(x.get("source") == "tess" for x in d))
parse = [d for d in diags if d.get("source") == "tess"]
print("  diagnostics:", [(d["range"]["start"], d["message"]) for d in parse])
check(parse and parse[0]["range"]["start"]["line"] == 5, "tess parse error mapped through the import to line 6")

# Fix the syntax, introduce a type error, save.
typed = 'import "geometry";\n\ndef main(): Int {\n    val n: Double = norm2(3.0, 4.0);\n    val label: Int = "four";\n    return 0;\n}\n'
main.write_text(typed, encoding="utf-8")
notify("textDocument/didChange", {"textDocument": {"uri": uri, "version": 2}, "contentChanges": [{"text": typed}]})
notify("textDocument/didSave", {"textDocument": {"uri": uri}})
diags = wait_diagnostics(uri, lambda d: any(x.get("code") == "type" for x in d))
t = [d for d in diags if d.get("code") == "type"]
print("  diagnostics:", [(d["range"]["start"], d["message"]) for d in t])
check(t[0]["range"]["start"] == {"line": 4, "character": 8}, "type error anchored on `label`")

syms = request("textDocument/documentSymbol", {"textDocument": {"uri": uri}})
check([s["name"] for s in syms] == ["main"], "document symbols")

hover = request("textDocument/hover", {"textDocument": {"uri": uri}, "position": {"line": 3, "character": 22}})
check(hover and "def norm2(x: Double, y: Double): Double" in hover["contents"]["value"] and "Squared length" in hover["contents"]["value"], "hover on imported function")

defn = request("textDocument/definition", {"textDocument": {"uri": uri}, "position": {"line": 3, "character": 22}})
check(defn and defn[0]["uri"].lower().endswith("geometry.csec") and defn[0]["range"]["start"]["line"] == 1, "definition jumps into the imported file")

defn = request("textDocument/definition", {"textDocument": {"uri": uri}, "position": {"line": 0, "character": 10}})
check(defn and defn[0]["uri"].lower().endswith("geometry.csec"), "definition on the import string opens the file")

comp = request("textDocument/completion", {"textDocument": {"uri": uri}, "position": {"line": 5, "character": 4}})
labels = {c["label"] for c in comp}
check({"n", "label", "norm2", "while", "Tensor", "println"} <= labels, "completion: locals, imports, keywords, types, builtins")

sig = request("textDocument/signatureHelp", {"textDocument": {"uri": uri}, "position": {"line": 3, "character": 31}})
check(sig and sig["activeParameter"] == 1 and len(sig["signatures"][0]["parameters"]) == 2, "signature help on the 2nd argument")

toks = request("textDocument/semanticTokens/full", {"textDocument": {"uri": uri}})
check(len(toks["data"]) % 5 == 0 and len(toks["data"]) > 40, "semantic tokens")

folds = request("textDocument/foldingRange", {"textDocument": {"uri": uri}})
check(any(f["startLine"] == 2 for f in folds), "folding range for main's body")

ws = request("workspace/symbol", {"query": "relu4"})
check(any(s["name"] == "relu4" for s in ws), "workspace symbols find examples/feedforward_nn.csec")

request("shutdown", None)
notify("exit", None)
proc.wait(timeout=10)
print("FAILED:" if failures else "all checks passed", *failures, sep="\n  ")
sys.exit(1 if failures else 0)
