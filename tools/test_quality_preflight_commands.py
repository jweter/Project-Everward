"""Regression coverage for cross-platform CMake/CTest preflight commands."""

from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path

_MODULE_PATH = Path(__file__).with_name("quality_preflight.py")
_SPEC = importlib.util.spec_from_file_location("everward_quality_preflight", _MODULE_PATH)
assert _SPEC is not None and _SPEC.loader is not None
quality_preflight = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(quality_preflight)


class QualityPreflightCommandTests(unittest.TestCase):
    def test_build_and_ctest_bind_release_for_multi_config_generators(self) -> None:
        _, build, test = quality_preflight.SIMULATION_BUILD
        self.assertEqual(build[build.index("--config") + 1], "Release")
        self.assertEqual(test[test.index("-C") + 1], "Release")

    def test_ctest_keeps_failure_output(self) -> None:
        self.assertIn("--output-on-failure", quality_preflight.SIMULATION_BUILD[-1])


if __name__ == "__main__":
    unittest.main()
