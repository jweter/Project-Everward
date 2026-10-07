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
    if not license_name:
        reasons.append("license name is missing")
    if attribution is True:
        reasons.append("attribution is required and must be reviewed before promotion")

    auto = policy.get("auto_approve", {})
    providers = {str(x).lower() for x in auto.get("providers", [])}
    licenses = {str(x).lower() for x in auto.get("licenses", [])}
    if (
        provider in providers
        and license_name.lower() in licenses
        and commercial is True
        and is_free is True
        and downloadable is True
        and attribution is False
    ):
        return Verdict("approved", (f"{provider} + {license_name} is in the auto-approval allowlist",))

    if reasons:
        return Verdict("review", tuple(reasons))
    return Verdict("review", ("provider/license combination is not in the auto-approval allowlist",))


def search_assets(
    query: str,
    *,
    asset_type: str | None = None,
    limit: int = 20,
    server: str = DEFAULT_SERVER,
    policy_path: Path = DEFAULT_POLICY,
    api_key: str | None = None,
) -> dict[str, Any]:
    policy = load_policy(policy_path)
    params: dict[str, Any] = {
        "q": query,
        "free": "true",
        "downloadable": "true",
        "limit": limit,
    }
    if asset_type:
        params["type"] = asset_type
    payload = _http_json(_server_url(server, "/v1/search", params), api_key)
    decorated: list[dict[str, Any]] = []
    for raw in payload.get("results", []):
        if not isinstance(raw, Mapping):
            continue
        candidate = dict(raw)
        candidate["everward_verdict"] = evaluate_asset(raw, policy).as_dict()
        decorated.append(candidate)
    return {
        "schema_version": 1,
        "query": query,
        "server": server,
        "results": decorated,
        "providers": payload.get("providers", []),
    }


def get_asset(asset_id: str, *, server: str = DEFAULT_SERVER, api_key: str | None = None) -> dict[str, Any]:
    quoted = urllib.parse.quote(asset_id, safe=":")
    return _http_json(_server_url(server, f"/v1/assets/{quoted}"), api_key)


def _safe_slug(value: str) -> str:
    slug = re.sub(r"[^A-Za-z0-9._-]+", "-", value).strip("-._").lower()
    if not slug:
        raise AssetPipelineError("Asset id cannot be converted to a safe path")
    return slug[:120]


def _filename_from_headers(headers: Any, fallback_url: str) -> str:
    disposition = headers.get("Content-Disposition")
    if disposition:
        match = re.search(r'filename\*?=(?:UTF-8\'\')?"?([^";]+)"?', disposition, re.IGNORECASE)
        if match:
            return Path(urllib.parse.unquote(match.group(1))).name
    path_name = Path(urllib.parse.urlparse(fallback_url).path).name
    return path_name or "asset-download.bin"


def _safe_extract_zip(archive: Path, destination: Path, *, max_extracted_bytes: int) -> list[Path]:
    extracted: list[Path] = []
    root = destination.resolve()
    total = 0
    with zipfile.ZipFile(archive) as bundle:
        for member in bundle.infolist():
            if member.is_dir():
                continue
            unix_mode = (member.external_attr >> 16) & 0o170000
            if unix_mode == 0o120000:
                raise AssetPipelineError(f"Refusing symlink in zip: {member.filename}")
            total += member.file_size
            if total > max_extracted_bytes:
                raise AssetPipelineError(
                    f"Refusing archive expanding beyond {max_extracted_bytes} bytes"
                )
            member_path = Path(member.filename)
            if member_path.is_absolute() or ".." in member_path.parts:
                raise AssetPipelineError(f"Refusing unsafe zip member: {member.filename}")
            target = (destination / member_path).resolve()
            if root not in target.parents and target != root:
                raise AssetPipelineError(f"Refusing zip path escape: {member.filename}")
            target.parent.mkdir(parents=True, exist_ok=True)
            with bundle.open(member, "r") as source, target.open("wb") as output:
                shutil.copyfileobj(source, output)
            extracted.append(target)
    return extracted


def stage_asset(
    asset_id: str,
    *,
    file_format: str | None = None,
    resolution: str | None = None,
    server: str = DEFAULT_SERVER,
    policy_path: Path = DEFAULT_POLICY,
    staging_root: Path = DEFAULT_STAGING_ROOT,
    api_key: str | None = None,
) -> Path:
    """Download an auto-approved asset into ignored staging with provenance."""
    policy = load_policy(policy_path)
    details = get_asset(asset_id, server=server, api_key=api_key)
    verdict = evaluate_asset(details, policy)
    if verdict.status != "approved":
        raise AssetPipelineError(
            f"{asset_id} is not eligible for automatic staging: {verdict.status}: {'; '.join(verdict.reasons)}"
        )

    params: dict[str, str] = {}
    if file_format:
        params["format"] = file_format
    if resolution:
        params["resolution"] = resolution
    quoted = urllib.parse.quote(asset_id, safe=":")
    url = _server_url(server, f"/v1/assets/{quoted}/download", params)

    asset_dir = staging_root / _safe_slug(asset_id)
    if asset_dir.exists():
        shutil.rmtree(asset_dir)
    asset_dir.mkdir(parents=True, exist_ok=True)

    headers = {"Accept": "*/*", "User-Agent": "Everward-Asset-Scout/1"}
    if api_key:
        headers["Authorization"] = f"Bearer {api_key}"
    request = urllib.request.Request(url, headers=headers)
    try:
        with urllib.request.urlopen(request, timeout=120) as response:  # noqa: S310 - explicit configured server
            final_url = response.geturl()
            filename = _filename_from_headers(response.headers, final_url)
            download_path = asset_dir / filename
            max_bytes = int(policy.get("technical_qa", {}).get("max_download_bytes", 1_073_741_824))
            written = 0
            with download_path.open("wb") as handle:
                while True:
                    chunk = response.read(1024 * 1024)
                    if not chunk:
                        break
                    written += len(chunk)
                    if written > max_bytes:
                        raise AssetPipelineError(f"Download exceeds policy limit of {max_bytes} bytes")
                    handle.write(chunk)
    except Exception as exc:
        shutil.rmtree(asset_dir, ignore_errors=True)
        if isinstance(exc, AssetPipelineError):
            raise
        raise AssetPipelineError(f"Asset download failed for {asset_id}: {exc}") from exc

    if zipfile.is_zipfile(download_path):
        extracted_dir = asset_dir / "payload"
        extracted_dir.mkdir()
        _safe_extract_zip(
            download_path,
            extracted_dir,
            max_extracted_bytes=int(policy.get("technical_qa", {}).get("max_extracted_bytes", 2_147_483_648)),
        )
        download_path.unlink()
    else:
        payload_dir = asset_dir / "payload"
        payload_dir.mkdir()
        download_path.replace(payload_dir / download_path.name)
