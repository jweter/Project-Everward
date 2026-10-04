from pathlib import Path


LAUNCHER = Path(__file__).with_name("run_unattended_low_spec_startup.ps1")


def test_cleanup_uses_survivor_pid_values_directly() -> None:
    source = LAUNCHER.read_text(encoding="utf-8")
    assert "$Survivor.Id" not in source


def test_process_tree_enumeration_does_not_silence_failures() -> None:
    source = LAUNCHER.read_text(encoding="utf-8")
    assert "Get-CimInstance Win32_Process -ErrorAction SilentlyContinue" not in source
