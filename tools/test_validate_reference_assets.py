"""Tests for the Prime Probe reference-asset validator."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import tempfile
import unittest

import validate_reference_assets


def _jpeg(width: int = 3, height: int = 2) -> bytes:
    return (
        b"\xff\xd8"
        b"\xff\xc0\x00\x0b\x08"
        + height.to_bytes(2, "big")
        + width.to_bytes(2, "big")
        + b"\x01\x01\x11\x00"
        b"\xff\xd9"
    )


class JpegDimensionsTests(unittest.TestCase):
    def test_reads_baseline_dimensions(self) -> None:
        self.assertEqual(validate_reference_assets.jpeg_dimensions(_jpeg()), (3, 2))

    def test_rejects_non_jpeg_data(self) -> None:
        with self.assertRaisesRegex(ValueError, "not a JPEG"):
            validate_reference_assets.jpeg_dimensions(b"not-an-image")


class ManifestValidationTests(unittest.TestCase):
    def setUp(self) -> None:
        self._temp = tempfile.TemporaryDirectory()
        self.root = Path(self._temp.name)
        self.assets = self.root / "assets"
        self.assets.mkdir()
        self.image = self.assets / "canonical.jpeg"
        self.image.write_bytes(_jpeg())
        self.manifest = self.root / "manifest.json"

    def tearDown(self) -> None:
        self._temp.cleanup()

    def write_manifest(self, **overrides: object) -> None:
        record: dict[str, object] = {
            "path": "canonical.jpeg",
            "sha256": hashlib.sha256(self.image.read_bytes()).hexdigest(),
            "width": 3,
            "height": 2,
            "classification": "canonical",
        }
        record.update(overrides)
        self.manifest.write_text(
            json.dumps({"schema_version": 1, "records": [record]}),
            encoding="utf-8",
        )

    def validate(self) -> list[str]:
        return validate_reference_assets.validate_reference_assets(
            self.assets, self.manifest
        )

    def test_accepts_a_matching_asset(self) -> None:
        self.write_manifest()
        self.assertEqual(self.validate(), [])

    def test_reports_hash_drift(self) -> None:
        self.write_manifest(sha256="0" * 64)
        self.assertIn("sha256 mismatch", "\n".join(self.validate()))

    def test_reports_dimension_drift(self) -> None:
        self.write_manifest(width=99)
        self.assertIn("dimensions mismatch", "\n".join(self.validate()))

    def test_rejects_path_escape(self) -> None:
        self.write_manifest(path="../outside.jpeg")
        self.assertIn("must stay inside", "\n".join(self.validate()))


if __name__ == "__main__":
    unittest.main()
