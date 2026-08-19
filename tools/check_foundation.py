"""Validate Everward repository foundation invariants.

This script intentionally uses only the Python standard library so the same
check can run locally and in GitHub Actions before the production toolchain is
selected.
"""

from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = (
    ".editorconfig",
    ".gitattributes",
    ".gitignore",
    ".github/CODEOWNERS",
    ".github/PULL_REQUEST_TEMPLATE.md",
    "README.md",
    "LICENSE",
    "CONTRIBUTING.md",
    "SECURITY.md",
    "THIRD_PARTY_NOTICES.md",
    "docs/VISION.md",
    "docs/DESIGN_PILLARS.md",
    "docs/GAMEPLAY_LOOP.md",
    "docs/SIMULATION_PHILOSOPHY.md",
    "docs/VISUAL_DIRECTION.md",
    "docs/AUDIO_DIRECTION.md",
    "docs/TECHNOLOGY_DECISIONS.md",
    "docs/ENGINE_DIRECTION.md",
    "docs/IP_AND_LICENSES.md",
    "docs/GLOSSARY.md",
    "docs/ROADMAP.md",
    "docs/PROJECT_STATUS.md",
    "docs/AGENT_DEVELOPMENT_POLICY.md",
    "docs/ARCHITECTURE.md",
    "docs/TESTING_STRATEGY.md",
    "docs/SAVE_FORMAT.md",
    "docs/PERFORMANCE_BUDGETS.md",
    "docs/DECISION_LOG.md",
    "docs/REPOSITORY_SETTINGS.md",
    "docs/ERROR_RESOLUTION_LEDGER.md",
    "docs/PHASE1_EXIT_GATE.md",
    "prototypes/simulation-clock/README.md",
    "prototypes/procedural-system/README.md",
    "prototypes/coordinate-scale/README.md",
    "prototypes/headless-simulation/README.md",
    "prototypes/rendering-benchmark/README.md",
    "src/README.md",
    "tests/README.md",
    "tools/README.md",
    "assets/README.md",
)

TEXT_SUFFIXES = {
    ".md",
    ".txt",
    ".py",
    ".yml",
    ".yaml",
    ".json",
    ".toml",
    ".ini",
    ".cfg",
    ".gd",
    ".cs",
    ".cpp",
    ".c",
    ".h",
    ".hpp",
}

IGNORED_DIRS = {".git", ".godot", "Binaries", "DerivedDataCache", "Intermediate", "Saved", "build", "dist", "artifacts"}
CONFLICT_MARKERS = ("<<<<<<<", "=======", ">>>>>>>")


def error(message: str, errors: list[str]) -> None:
    print(f"ERROR: {message}")
    errors.append(message)


def validate_required_files(errors: list[str]) -> None:
    for relative in REQUIRED_FILES:
        path = ROOT / relative
        if not path.is_file():
            error(f"missing required foundation file: {relative}", errors)
        elif path.stat().st_size == 0:
            error(f"required foundation file is empty: {relative}", errors)


def should_scan(path: Path) -> bool:
    if any(part in IGNORED_DIRS for part in path.parts):
        return False
    if path.name in {"LICENSE", ".editorconfig", ".gitattributes", ".gitignore", "CODEOWNERS"}:
        return True
    return path.suffix.lower() in TEXT_SUFFIXES


def validate_conflict_markers(errors: list[str]) -> None:
    for path in ROOT.rglob("*"):
        if not path.is_file() or not should_scan(path):
            continue
        try:
            lines = path.read_text(encoding="utf-8").splitlines()
        except UnicodeDecodeError:
            continue
        for line_number, line in enumerate(lines, start=1):
            if line.startswith(CONFLICT_MARKERS):
                error(
                    f"unresolved merge conflict marker: {path.relative_to(ROOT)}:{line_number}",
                    errors,
                )


def validate_docs(errors: list[str]) -> None:
    docs = ROOT / "docs"
    if not docs.is_dir():
        error("docs directory is missing", errors)
        return
    for path in docs.glob("*.md"):
        if path.stat().st_size == 0:
            error(f"documentation file is empty: {path.relative_to(ROOT)}", errors)


def main() -> int:
    errors: list[str] = []
    validate_required_files(errors)
    validate_conflict_markers(errors)
    validate_docs(errors)

    if errors:
        print(f"Foundation validation failed with {len(errors)} error(s).")
        return 1

    print(f"Foundation validation passed: {len(REQUIRED_FILES)} required files present.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
