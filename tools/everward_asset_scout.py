#!/usr/bin/env python3
"""Everward third-party asset discovery, licensing, QA, and promotion gate.

The pipeline intentionally keeps network discovery separate from production
promotion. Search results are advisory. Only policy-approved, directly
downloadable assets can enter staging, and only staging assets that pass local
technical QA can enter the tracked third-party asset registry.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import sys
import urllib.parse
import urllib.request
import zipfile
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Mapping, Sequence

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_POLICY = REPO_ROOT / "assets" / "pipeline" / "policy.json"
DEFAULT_REGISTRY = REPO_ROOT / "assets" / "third_party" / "asset_registry.json"
DEFAULT_IMPORT_MANIFEST = REPO_ROOT / "assets" / "pipeline" / "unreal_import_manifest.json"
DEFAULT_STAGING_ROOT = REPO_ROOT / "assets" / "staging"
DEFAULT_THIRD_PARTY_ROOT = REPO_ROOT / "assets" / "third_party"
DEFAULT_SERVER = os.environ.get("EVERWARD_ASSET_SERVER_URL", "http://127.0.0.1:8787")

PROVENANCE_FILE = "provenance.json"
QA_FILE = "qa.json"
IMPORTABLE_EXTENSIONS = {
    ".fbx",
    ".glb",
    ".gltf",
    ".obj",
    ".png",
    ".jpg",
    ".jpeg",
    ".tga",
    ".exr",
    ".hdr",
}
DANGEROUS_EXTENSIONS = {
    ".exe",
    ".dll",
    ".com",
    ".msi",
    ".bat",
    ".cmd",
    ".ps1",
    ".vbs",
    ".js",
    ".mjs",
    ".cjs",
    ".py",
    ".pyw",
    ".sh",
    ".app",
    ".scr",
}


class AssetPipelineError(RuntimeError):
    """A fail-closed asset-pipeline error."""


@dataclass(frozen=True)
class Verdict:
    status: str
    reasons: tuple[str, ...]

    def as_dict(self) -> dict[str, Any]:
        return {"status": self.status, "reasons": list(self.reasons)}


def _read_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise AssetPipelineError(f"Unable to read valid JSON from {path}: {exc}") from exc


def _write_json(path: Path, payload: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    temporary.replace(path)


def load_policy(path: Path = DEFAULT_POLICY) -> dict[str, Any]:
    policy = _read_json(path)
    if policy.get("schema_version") != 1:
        raise AssetPipelineError("Unsupported asset policy schema_version; expected 1")
    return policy


def _server_url(base_url: str, path: str, params: Mapping[str, Any] | None = None) -> str:
    base = base_url.rstrip("/")
    query = ""
    if params:
        query = "?" + urllib.parse.urlencode(
            [(key, item) for key, value in params.items() for item in (value if isinstance(value, list) else [value])]
        )
    return f"{base}{path}{query}"


def _http_json(url: str, api_key: str | None = None) -> dict[str, Any]:
    headers = {"Accept": "application/json", "User-Agent": "Everward-Asset-Scout/1"}
    if api_key:
        headers["Authorization"] = f"Bearer {api_key}"
    request = urllib.request.Request(url, headers=headers)
    try:
        with urllib.request.urlopen(request, timeout=30) as response:  # noqa: S310 - URL is explicit operator configuration
            body = response.read()
    except Exception as exc:  # urllib surfaces several transport subclasses
        raise AssetPipelineError(f"Asset server request failed: {url}: {exc}") from exc
    try:
        payload = json.loads(body.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise AssetPipelineError(f"Asset server returned invalid JSON: {url}") from exc
    if not isinstance(payload, dict):
        raise AssetPipelineError(f"Asset server returned non-object JSON: {url}")
    return payload


def _provider_for(asset: Mapping[str, Any]) -> str:
    provider = str(asset.get("provider") or "").strip().lower()
    if provider:
        return provider
    asset_id = str(asset.get("id") or "")
    if ":" in asset_id:
        return asset_id.split(":", 1)[0].lower()
    return ""


def _license(asset: Mapping[str, Any]) -> Mapping[str, Any]:
    value = asset.get("license")
    return value if isinstance(value, Mapping) else {}


def _free(asset: Mapping[str, Any]) -> bool | None:
    price = asset.get("price")
    if isinstance(price, Mapping) and "free" in price:
        return bool(price.get("free"))
    if "free" in asset:
        return bool(asset.get("free"))
    return None


def evaluate_asset(asset: Mapping[str, Any], policy: Mapping[str, Any]) -> Verdict:
    """Return the conservative licensing/acquisition verdict for an asset."""
    reasons: list[str] = []
    asset_id = str(asset.get("id") or "").strip()
    provider = _provider_for(asset)
    license_info = _license(asset)
    license_name = str(license_info.get("name") or "").strip()
    commercial = license_info.get("commercialUse")
    attribution = license_info.get("attributionRequired")
    downloadable = asset.get("downloadable")
    is_free = _free(asset)

    if not asset_id or not provider:
        return Verdict("rejected", ("missing stable asset id/provider",))
    if commercial is False:
        return Verdict("rejected", ("license explicitly forbids commercial use",))
    if is_free is False:
        return Verdict("rejected", ("asset is not free under current policy",))

    if commercial is not True:
        reasons.append("commercial-use permission is not explicitly confirmed")
    if is_free is not True:
        reasons.append("free status is not explicitly confirmed")
    if downloadable is not True:
        reasons.append("asset is not directly downloadable through the approved server")
