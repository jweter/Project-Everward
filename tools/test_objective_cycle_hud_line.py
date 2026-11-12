from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
HUD = ROOT / "unreal/Source/Everward/EverwardHUD.cpp"
PANEL_WIDTH = 54  # TruncatedPanelLine's default; the HUD panel line is cut to this many characters


def objective_line(state: str, cycles: int, pending: list[str]) -> str:
    """Mirror of RestorationObjectiveCycleLine() in EverwardHUD.cpp, including TruncatedPanelLine's cut."""

    def truncated(text: str, maximum: int = PANEL_WIDTH) -> str:
        return text if len(text) <= maximum else text[: max(0, maximum - 3)] + "..."

    pending_text = ", ".join(pending)
    if state == "ACTIVE":
        return truncated(f"OBJECTIVE  ACTIVE // PENDING {pending_text or 'NONE'} // CYCLES {cycles}")
    hint = "  [F7] NEXT CYCLE"
    body = f"OBJECTIVE  {state} // CYCLES {cycles}"
    if pending_text:
        body += f" // PENDING {pending_text}"
    return truncated(body, PANEL_WIDTH - len(hint)) + hint


class ObjectiveCycleHudLineTests(unittest.TestCase):
    """The objective row's [F7] hint was appended to a 72-78 character line that the 54 character HUD panel then cut,
    so the hint was clipped or lost in exactly the Idle and Completed states it exists for. The hint is now appended
    after the body is shortened. The C++ cannot be compiled here (no Unreal), so this pins the source shape and the
    arithmetic on a faithful mirror; the Unreal compile and an in-editor look stay PENDING."""

    def setUp(self) -> None:
        self.hud = HUD.read_text(encoding="utf-8")

    def test_the_source_appends_the_hint_after_truncating_the_body(self) -> None:
        self.assertIn('const FString TriggerHint = TEXT("  [F7] NEXT CYCLE");', self.hud)
        self.assertIn("TruncatedPanelLine(Body, 54 - TriggerHint.Len()) + TriggerHint", self.hud)
        self.assertNotIn("Line += TEXT(\"  //  [F7] TRIGGER NEXT CYCLE\")", self.hud)

    def test_the_hint_is_never_cut_in_the_idle_and_completed_states(self) -> None:
        for state in ("IDLE", "COMPLETED"):
            for cycles in (0, 1, 12, 999):
                for pending in ([], ["THERMAL"], ["THERMAL", "SENSORS", "PROPULSION"]):
                    line = objective_line(state, cycles, pending)
                    self.assertTrue(line.endswith("[F7] NEXT CYCLE"), line)
                    self.assertLessEqual(len(line), PANEL_WIDTH, line)

    def test_an_active_cycle_leads_with_what_the_player_must_restore(self) -> None:
        line = objective_line("ACTIVE", 1, ["THERMAL", "SENSORS"])
        self.assertTrue(line.startswith("OBJECTIVE  ACTIVE // PENDING THERMAL, SENS"), line)
        self.assertLessEqual(len(line), PANEL_WIDTH)
        self.assertNotIn("[F7]", line)  # no trigger hint while a cycle is Active, matching the accept/reject rule

    def test_the_old_form_really_lost_the_hint(self) -> None:
        old = "OBJECTIVE  COMPLETED // CYCLES 12 // PENDING NONE  //  [F7] TRIGGER NEXT CYCLE"
        cut = old if len(old) <= PANEL_WIDTH else old[: PANEL_WIDTH - 3] + "..."
        self.assertNotIn("TRIGGER NEXT CYCLE", cut)
        self.assertIsNone(re.search(r"\[F7\]", cut))


if __name__ == "__main__":
    unittest.main()
