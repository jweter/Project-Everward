from __future__ import annotations

import importlib.util
import json
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

MODULE_PATH = Path(__file__).with_name("everward_asset_scout.py")
SPEC = importlib.util.spec_from_file_location("everward_asset_scout", MODULE_PATH)
assert SPEC and SPEC.loader
scout = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = scout
SPEC.loader.exec_module(scout)


POLICY = {
    "schema_version": 1,
    "auto_approve": {
        "providers": ["polyhaven", "ambientcg"],
        "licenses": ["CC0"],
    },
    "technical_qa": {
        "max_download_bytes": 1024 * 1024,
        "max_extracted_bytes": 2 * 1024 * 1024,
        "max_file_bytes": 1024 * 1024,
        "importable_extensions": [".gltf", ".glb", ".png"],
    },
}


def write_json(path: Path, payload: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload), encoding="utf-8")


class VerdictTests(unittest.TestCase):
    def test_cc0_allowlisted_provider_auto_approves(self) -> None:
        asset = {
            "id": "polyhaven:rock_01",
            "provider": "polyhaven",
            "license": {"name": "CC0", "commercialUse": True, "attributionRequired": False},
            "price": {"free": True},
            "downloadable": True,
        }
        verdict = scout.evaluate_asset(asset, POLICY)
        self.assertEqual("approved", verdict.status)

    def test_attribution_or_unknown_provider_requires_review(self) -> None:
        asset = {
            "id": "blenderkit:rock_01",
            "provider": "blenderkit",
            "license": {"name": "CC0", "commercialUse": True, "attributionRequired": True},
            "price": {"free": True},
            "downloadable": True,
        }
        verdict = scout.evaluate_asset(asset, POLICY)
        self.assertEqual("review", verdict.status)
        self.assertTrue(any("attribution" in reason for reason in verdict.reasons))

    def test_noncommercial_asset_rejected(self) -> None:
        asset = {
            "id": "example:nope",
            "provider": "example",
            "license": {"name": "Example", "commercialUse": False, "attributionRequired": False},
            "price": {"free": True},
            "downloadable": True,
        }
        self.assertEqual("rejected", scout.evaluate_asset(asset, POLICY).status)


class ArchiveSafetyTests(unittest.TestCase):
    def test_zip_path_traversal_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            archive = root / "bad.zip"
            with zipfile.ZipFile(archive, "w") as bundle:
                bundle.writestr("../escape.txt", "no")
            with self.assertRaises(scout.AssetPipelineError):
                scout._safe_extract_zip(
                    archive,
                    root / "out",
                    max_extracted_bytes=1024,
                )


class QaTests(unittest.TestCase):
    def _fixture(self, root: Path, payload_name: str, payload_bytes: bytes) -> tuple[Path, Path]:
        asset_dir = root / "staging" / "polyhaven-rock"
        payload = asset_dir / "payload"
        payload.mkdir(parents=True)
        (payload / payload_name).write_bytes(payload_bytes)
        write_json(
            asset_dir / "provenance.json",
            {
                "schema_version": 1,
                "asset_id": "polyhaven:rock",
                "provider": "polyhaven",
                "license": {"name": "CC0", "commercialUse": True, "attributionRequired": False},
                "price": {"free": True},
                "downloadable": True,
                "verdict": {"status": "approved", "reasons": []},
            },
        )
        policy_path = root / "policy.json"
        write_json(policy_path, POLICY)
        return asset_dir, policy_path

    def test_gltf_dependency_check_and_hash(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            gltf = json.dumps(
                {
                    "asset": {"version": "2.0"},
                    "meshes": [{"primitives": []}],
                    "buffers": [{"uri": "mesh.bin"}],
                }
            ).encode()
            asset_dir, policy_path = self._fixture(root, "rock.gltf", gltf)
            (asset_dir / "payload" / "mesh.bin").write_bytes(b"mesh")
            result = scout.run_qa(asset_dir, policy_path=policy_path)
            self.assertEqual("pass", result["status"])
            self.assertEqual(2, len(result["files"]))
            self.assertTrue(all(record["sha256"] for record in result["files"]))

    def test_missing_gltf_companion_fails(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            gltf = json.dumps(
                {"asset": {"version": "2.0"}, "meshes": [{}], "buffers": [{"uri": "missing.bin"}]}
            ).encode()
            asset_dir, policy_path = self._fixture(root, "rock.gltf", gltf)
            result = scout.run_qa(asset_dir, policy_path=policy_path)
            self.assertEqual("fail", result["status"])
            self.assertTrue(any("missing" in failure for failure in result["failures"]))

    def test_script_payload_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            asset_dir, policy_path = self._fixture(root, "payload.py", b"print('no')")
            result = scout.run_qa(asset_dir, policy_path=policy_path)
            self.assertEqual("fail", result["status"])


class PromotionTests(unittest.TestCase):
    def test_promotion_updates_registry_and_import_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            asset_dir = root / "staging" / "polyhaven-rock"
            payload = asset_dir / "payload"
            payload.mkdir(parents=True)
            (payload / "rock.png").write_bytes(b"texture")
            provenance = {
                "schema_version": 1,
                "asset_id": "polyhaven:rock",
                "provider": "polyhaven",
                "source_url": "https://example.invalid/rock",
                "license": {"name": "CC0", "commercialUse": True, "attributionRequired": False},
                "price": {"free": True},
                "downloadable": True,
                "verdict": {"status": "approved", "reasons": []},
            }
            write_json(asset_dir / "provenance.json", provenance)
            policy_path = root / "policy.json"
            write_json(policy_path, POLICY)
            qa = scout.run_qa(asset_dir, policy_path=policy_path)
            self.assertEqual("pass", qa["status"])

            registry_path = root / "asset_registry.json"
            manifest_path = root / "unreal_import_manifest.json"
            third_party = root / "third_party"
            write_json(registry_path, {"schema_version": 1, "assets": []})

            entry = scout.promote_asset(
                asset_dir,
                registry_path=registry_path,
                third_party_root=third_party,
                import_manifest_path=manifest_path,
                policy_path=policy_path,
                repo_root=root,
            )
            self.assertEqual("polyhaven:rock", entry["asset_id"])
            registry = json.loads(registry_path.read_text())
            self.assertEqual(1, len(registry["assets"]))
            manifest = json.loads(manifest_path.read_text())
            self.assertEqual(1, len(manifest["imports"]))
            self.assertTrue(manifest["imports"][0]["destination"].startswith("/Game/ThirdParty/"))

    def test_stale_manifest_validation_fails(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            registry = root / "registry.json"
            manifest = root / "manifest.json"
            write_json(registry, {"schema_version": 1, "assets": []})
            write_json(manifest, {"schema_version": 1, "imports": [{"asset_id": "stale"}]})
            with self.assertRaises(scout.AssetPipelineError):
                scout.validate_registry(registry, manifest)


if __name__ == "__main__":
    unittest.main()
