"""Run Everward's repository-specific foundation and simulation preflight."""

from __future__ import annotations

import argparse
import subprocess
import sys
from collections.abc import Sequence

Command = tuple[str, ...]

FOUNDATION_CHECKS: tuple[Command, ...] = (
    (sys.executable, "tools/check_foundation.py"),
    ("git", "diff", "--check"),
)

PYTHON_TESTS: tuple[Command, ...] = (
    (sys.executable, "-m", "unittest", "discover", "-s", "tools", "-p", "test_*.py", "-v"),
    (
        sys.executable,
        "-m",
        "unittest",
        "discover",
        "-s",
        "prototypes/simulation-clock",
        "-p",
        "test_*.py",
        "-v",
    ),
    (
        sys.executable,
        "-m",
        "unittest",
        "discover",
        "-s",
        "prototypes/procedural-system",
        "-p",
        "test_*.py",
        "-v",
    ),
    (
        sys.executable,
        "-m",
        "unittest",
        "discover",
        "-s",
        "prototypes/coordinate-scale",
        "-p",
        "test_*.py",
        "-v",
    ),
    (
        sys.executable,
        "-m",
        "unittest",
        "discover",
        "-s",
        "prototypes/headless-simulation",
        "-p",
        "test_*.py",
        "-v",
    ),
    (
        sys.executable,
        "-m",
        "unittest",
        "discover",
        "-s",
        "prototypes/rendering-benchmark",
        "-p",
        "test_*.py",
        "-v",
    ),
    (
        sys.executable,
        "-m",
        "unittest",
        "discover",
        "-s",
        "prototypes",
        "-p",
        "test_*.py",
        "-t",
        "prototypes",
        "-v",
    ),
)

SIMULATION_BUILD: tuple[Command, ...] = (
    ("cmake", "-S", "src/simulation", "-B", "build/simulation", "-DCMAKE_BUILD_TYPE=Release"),
    ("cmake", "--build", "build/simulation", "--parallel", "2"),
    ("ctest", "--test-dir", "build/simulation", "--output-on-failure"),
)


def run(commands: Sequence[Command]) -> int:
    """Run commands in order and stop at the first failure."""
    for command in commands:
        print(f"+ {' '.join(command)}", flush=True)
        returncode = subprocess.run(command, check=False).returncode  # noqa: S603
        if returncode != 0:
            return returncode
    return 0


def main(argv: Sequence[str] | None = None) -> int:
    """Run the fast foundation gate, or full CI-parity simulation validation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--full",
        action="store_true",
        help="Also build/test the C++ simulation core and all Python prototype suites.",
    )
    args = parser.parse_args(argv)

    returncode = run(FOUNDATION_CHECKS)
    if returncode != 0 or not args.full:
        return returncode
    returncode = run(SIMULATION_BUILD)
    if returncode != 0:
        return returncode
    return run(PYTHON_TESTS)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
