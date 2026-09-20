from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SMOKE = ROOT / "tools" / "run_unreal_headless_smoke.ps1"
WORKER = ROOT / "tools" / "everward_unattended_worker.py"


def test_headless_smoke_enforces_bounded_cold_start_floor() -> None:
    source = SMOKE.read_text(encoding="utf-8")
    assert '[int]$TimeoutSeconds = 300' in source
    assert '$EffectiveTimeoutSeconds = [Math]::Max($TimeoutSeconds, 300)' in source
    assert '$Process.WaitForExit($EffectiveTimeoutSeconds * 1000)' in source


def test_legacy_worker_timeout_cannot_shorten_cold_start_floor() -> None:
    worker = WORKER.read_text(encoding="utf-8")
    smoke = SMOKE.read_text(encoding="utf-8")
    assert '"-TimeoutSeconds",\n            "120"' in worker
    assert '[Math]::Max($TimeoutSeconds, 300)' in smoke
