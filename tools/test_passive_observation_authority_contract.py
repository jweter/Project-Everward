from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "src/simulation/include/everward/simulation/core.hpp"
RUNTIME = ROOT / "src/simulation/include/everward/simulation/software_policy.hpp"
DAMAGE = ROOT / "src/simulation/include/everward/simulation/impact_damage.hpp"


def test_passive_observation_has_one_authoritative_mutation_boundary() -> None:
    core = CORE.read_text(encoding="utf-8")
    runtime = RUNTIME.read_text(encoding="utf-8")
    damage = DAMAGE.read_text(encoding="utf-8")

    assert "void record_passive_observation(" in core
    assert "ObservationMode::Passive" in core
    assert "apply_observation(" in core

    # Until an authorized sensor/proximity trigger exists, wrappers must not
    # invent a second passive-observation implementation or confidence model.
    assert "ObservationMode::Passive" not in runtime
    assert "ObservationMode::Passive" not in damage
