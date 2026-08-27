from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation/include/everward/simulation"
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2ImpactDamageSurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.damage = (SIM / "impact_damage.hpp").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.adapter_cpp = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        self.hud = (SOURCE / "EverwardHUD.cpp").read_text(encoding="utf-8")

    def test_damage_truth_is_energy_based_and_engine_independent(self) -> None:
        self.assertIn("0.5 * snapshot.mass_kg", self.damage)
        self.assertIn("snapshot.last_contact_normal_speed_mps", self.damage)
        self.assertIn("enum class ImpactSeverity", self.damage)
        self.assertIn("enum class IntegrityBand", self.damage)
        self.assertIn("struct ImpactDamageRecord", self.damage)
        self.assertIn("class ImpactDamageModel", self.damage)
        self.assertNotIn("Unreal", self.damage.split("class DamageAwareProbeRuntime")[0])

    def test_damage_runtime_composes_existing_contact_runtime(self) -> None:
        self.assertIn("class DamageAwareProbeRuntime", self.damage)
        self.assertIn("ProbeRuntime runtime_{}", self.damage)
        self.assertIn("runtime_.advance_wall_ticks(wall_ticks)", self.damage)
        self.assertIn("damage_.assess_latest_contact(runtime_)", self.damage)
        self.assertNotIn("sweep_probe_against_body", self.damage)
        self.assertNotIn("resolve_static_contacts", self.damage)

    def test_component_integrity_supports_staged_recovery(self) -> None:
        self.assertIn("double sensors{1.0}", self.damage)
        self.assertIn("double propulsion{1.0}", self.damage)
        self.assertIn("double computation{1.0}", self.damage)
        self.assertIn("double thermal{1.0}", self.damage)
        self.assertIn("if (fraction <= 0.0) return IntegrityBand::Offline", self.damage)
        self.assertIn("if (fraction < 0.25) return IntegrityBand::Critical", self.damage)
        self.assertIn("runtime_.set_subsystem_operational(subsystem, fraction > 0.0)", self.damage)

    def test_partial_integrity_changes_existing_mechanical_performance(self) -> None:
        self.assertIn("delta.yaw *= effectiveness", self.damage)
        self.assertIn("local_delta_velocity.x *= effectiveness", self.damage)
        self.assertIn("runtime_.start_scan(target_id, duration_s / effectiveness)", self.damage)

    def test_unreal_adapter_exposes_damage_without_owning_it(self) -> None:
        self.assertIn("DamageAwareProbeRuntime", self.adapter_h)
        self.assertIn("DamageAwareProbeRuntime::make_canonical_ev0001()", self.adapter_cpp)
        for field in (
            "bHasImpactHistory",
            "LastImpactEnergyJoules",
            "LastImpactSeverity",
            "LastImpactSubsystem",
            "LastImpactIntegrityBefore",
            "LastImpactIntegrityAfter",
            "SensorsIntegrity",
            "PropulsionIntegrity",
            "ComputationIntegrity",
            "ThermalIntegrity",
            "IntegrityFraction",
        ):
            self.assertIn(field, self.adapter_h)
        self.assertIn("Core->component_integrity()", self.adapter_cpp)
        self.assertIn("Core->last_impact()", self.adapter_cpp)
        self.assertNotIn("0.5 *", self.adapter_cpp)

    def test_hud_renders_authoritative_impact_and_integrity(self) -> None:
        self.assertIn("IMPACT %s // %s // %.1f kJ", self.hud)
        self.assertIn("LastImpactEnergyJoules", self.hud)
        self.assertIn("LastImpactIntegrityBefore", self.hud)
        self.assertIn("LastImpactIntegrityAfter", self.hud)
        self.assertIn("Capability.IntegrityFraction", self.hud)
        self.assertNotIn('everward/simulation', self.hud)
        self.assertNotIn("AddImpulse", self.hud)
        self.assertNotIn("OnComponentHit", self.hud)


if __name__ == "__main__":
    unittest.main()
