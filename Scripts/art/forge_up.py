"""Make sure a forge is serving, and open the official page. Idempotent: if something already
answers on the port it just opens the browser; otherwise it starts forge detached (its own
console window, survives this process) and opens once it answers.

Wired as a Claude Code SessionStart hook in .claude/settings.json so the selecter is up and on
screen every session. Also fine by hand:

    py Scripts/art/forge_up.py
"""

import subprocess
import sys
import time
import urllib.request
import webbrowser
from pathlib import Path

PORT = 8770
URL = "http://127.0.0.1:%d/official" % PORT
FORGE = Path(__file__).resolve().parent / "forge.py"


def up():
    try:
        with urllib.request.urlopen(URL, timeout=1) as r:
            return r.status == 200
    except Exception:
        return False


def main():
    if not up():
        flags = getattr(subprocess, "CREATE_NEW_CONSOLE", 0)
        subprocess.Popen([sys.executable, str(FORGE), "--port", str(PORT), "--no-open"],
                         cwd=str(FORGE.parents[2]), creationflags=flags,
                         stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
        for _ in range(40):
            if up():
                break
            time.sleep(0.5)
        else:
            print("forge did not answer on %s" % URL)
            return 1
    webbrowser.open(URL)
    print("forge  %s" % URL)
    return 0


if __name__ == "__main__":
    sys.exit(main())
