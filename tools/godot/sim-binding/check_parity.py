"""G1 parity check: the Everward simulation must give identical results natively and inside Godot.

Runs the native reference executable and the headless Godot script (tests/parity.gd) through the
GDExtension, then compares their output line by line. Exit 0 only if every line matches.

    python check_parity.py --godot PATH\\Godot_v4.7.2-stable_win64_console.exe --native PATH\\native_parity.exe

Known issue (documented in README): on Godot 4.7.2 with this toolchain, the very first headless
`--import` of a project that registers any extension class exits with a crash after the import has
completed. The checker reports it, confirms the import produced its cache, and re-imports once; it never
treats a parity mismatch or a crash during the parity run itself as success.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def run(cmd: list[str], timeout: int = 180) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)


def clean(text: str) -> list[str]:
    lines = []
    for raw in text.replace("\r", "").splitlines():
        if not raw.strip() or raw.startswith("Godot Engine"):
            continue
        lines.append(raw.rstrip())
    return lines


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--godot", required=True, help="Godot 4.7.x console executable")
    ap.add_argument("--native", required=True, help="native_parity executable built from src/native_parity_main.cpp")
    ap.add_argument("--project", default=str(HERE / "project"))
    args = ap.parse_args()
    project = Path(args.project)

    native = run([args.native])
    if native.returncode != 0:
        print(f"FAIL native reference exited {native.returncode}")
        return 1
    expected = clean(native.stdout)

    cache = project / ".godot" / "extension_list.cfg"
    imp = run([args.godot, "--headless", "--path", str(project), "--import"])
    if imp.returncode != 0:
        print(f"NOTE first --import exited {imp.returncode} (known first-import crash); cache present: {cache.exists()}")
        if not cache.exists():
            print("FAIL import did not produce the extension cache")
            return 1
        again = run([args.godot, "--headless", "--path", str(project), "--import"])
        print(f"NOTE re-import exited {again.returncode}")
        if again.returncode != 0:
            print("FAIL re-import also failed")
            return 1

    godot = run([args.godot, "--headless", "--path", str(project), "--script", "res://tests/parity.gd"])
    actual = clean(godot.stdout)
    if godot.returncode != 0:
        print(f"FAIL Godot parity run exited {godot.returncode}")
        print("\n".join(actual[-10:]))
        return 1

    if actual != expected:
        print("FAIL outputs differ")
        for i in range(max(len(actual), len(expected))):
            e = expected[i] if i < len(expected) else "<missing>"
            a = actual[i] if i < len(actual) else "<missing>"
            if e != a:
                print(f"  line {i + 1}\n    native: {e}\n    godot:  {a}")
        return 1
    print(f"PASS {len(expected)} lines identical (native == Godot)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
