"""Validate canonical Prime Probe reference assets against their manifest."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
ASSET_ROOT = ROOT / "assets/reference/probe/gen1-prime"
MANIFEST_PATH = ASSET_ROOT / "docs/asset_manifest.json"

SOF_MARKERS = {
    0xC0, 0xC1, 0xC2, 0xC3, 0xC5, 0xC6, 0xC7,
    0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF,
}


def jpeg_dimensions(data: bytes) -> tuple[int, int]:
    """Return JPEG width/height without a third-party imaging dependency."""

    if not data.startswith(b"\xff\xd8"):
        raise ValueError("not a JPEG file")

    offset = 2
    while offset < len(data):
        if data[offset] != 0xFF:
            offset += 1
            continue
        while offset < len(data) and data[offset] == 0xFF:
            offset += 1
        if offset >= len(data):
            break

        marker = data[offset]
        offset += 1
        if marker in {0x01, *range(0xD0, 0xD9)}:
            continue
        if marker in {0xD9, 0xDA} or offset + 2 > len(data):
            break

        segment_length = int.from_bytes(data[offset : offset + 2], "big")
        if segment_length < 2 or offset + segment_length > len(data):
            raise ValueError("invalid JPEG segment")
        if marker in SOF_MARKERS:
            if segment_length < 7:
                raise ValueError("invalid JPEG size segment")
            height = int.from_bytes(data[offset + 3 : offset + 5], "big")
            width = int.from_bytes(data[offset + 5 : offset + 7], "big")
            if width < 1 or height < 1:
                raise ValueError("invalid JPEG dimensions")
            return width, height
        offset += segment_length

    raise ValueError("JPEG dimensions were not found")


def validate_reference_assets(
    asset_root: Path = ASSET_ROOT,
    manifest_path: Path = MANIFEST_PATH,
) -> list[str]:
    errors: list[str] = []
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return [f"cannot read asset manifest: {error}"]

    if manifest.get("schema_version") != 1:
        errors.append("asset manifest schema_version must be 1")
    records = manifest.get("records")
    if not isinstance(records, list) or not records:
        return errors + ["asset manifest records must be a non-empty list"]

    seen: set[str] = set()
    resolved_asset_root = asset_root.resolve()
    for index, record in enumerate(records):
        label = f"record {index}"
        if not isinstance(record, dict):
            errors.append(f"{label}: must be an object")
            continue
        relative = record.get("path")
        if not isinstance(relative, str) or not relative:
            errors.append(f"{label}: path must be a non-empty string")
            continue
        label = relative
        path = Path(relative)
        if path.is_absolute() or ".." in path.parts:
            errors.append(f"{label}: path must stay inside the reference package")
            continue
        expected_classification = path.parts[0] if path.parts else ""
        if expected_classification not in {"canonical", "exploratory"}:
            errors.append(f"{label}: path must be in canonical/ or exploratory/")
        if record.get("classification") != expected_classification:
            errors.append(
                f"{label}: classification must match the source directory "
                f"({expected_classification})"
            )
        if relative in seen:
            errors.append(f"{label}: duplicate manifest path")
            continue
        seen.add(relative)

        source = asset_root / path
        resolved_source = source.resolve()
        if not resolved_source.is_relative_to(resolved_asset_root):
            errors.append(f"{label}: resolved path must stay inside the reference package")
            continue
        try:
            data = resolved_source.read_bytes()
        except OSError as error:
            errors.append(f"{label}: cannot read asset: {error}")
            continue

        digest = hashlib.sha256(data).hexdigest()
        if digest != record.get("sha256"):
            errors.append(f"{label}: sha256 mismatch")

        try:
            width, height = jpeg_dimensions(data)
        except ValueError as error:
            errors.append(f"{label}: {error}")
            continue
        if width != record.get("width") or height != record.get("height"):
            errors.append(
                f"{label}: dimensions mismatch "
                f"(manifest {record.get('width')}x{record.get('height')}, "
                f"file {width}x{height})"
            )

    discovered = {
        str(path.relative_to(asset_root)).replace("\\", "/")
        for directory in ("canonical", "exploratory")
        for path in (asset_root / directory).rglob("*")
        if path.is_file() and path.suffix.lower() in {".jpg", ".jpeg"}
    }
    for relative in sorted(discovered - seen):
        errors.append(f"{relative}: reference image is not listed in the manifest")

    return errors


def main() -> int:
    errors = validate_reference_assets()
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        print(f"Reference asset validation failed with {len(errors)} error(s).")
        return 1
    print("Reference asset validation passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
