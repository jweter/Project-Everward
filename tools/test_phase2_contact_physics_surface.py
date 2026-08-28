from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation/include/everward/simulation"
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2ContactPhysicsSurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.types = (SIM / "types.hpp").read_text(encoding="utf-8")
        self.core = (SIM / "core.hpp").read_text(encoding="utf-8")
        self.runtime = (SIM / "software_policy.hpp").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.adapter_cpp = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        self.pawn_h = (SOURCE / "EverwardProbePawn.h").read_text(encoding="utf-8")
        self.pawn_cpp = (SOURCE / "EverwardProbePawn.cpp").read_text(encoding="utf-8")
        self.environment_h = (SOURCE / "EverwardPhase2TestEnvironment.h").read_text(encoding="utf-8")
        self.environment_cpp = (SOURCE / "EverwardPhase2TestEnvironment.cpp").read_text(encoding="utf-8")

    def test_contact_truth_is_engine_independent(self) -> None:
        self.assertIn("struct StaticSphereBody", self.types)
        self.assertIn("collision_envelope_radius_m", self.types)
        self.assertIn("last_contact_point_m", self.types)
        self.assertIn("last_contact_surface_normal", self.types)
        self.assertIn("last_contact_relative_velocity_mps", self.types)
        self.assertIn("DomainEventType::Contact", self.core)
        self.assertIn("void resolve_contact(", self.core)
        self.assertIn("resolve_static_contacts", self.runtime)
        self.assertIn("sweep_probe_against_body", self.runtime)
        self.assertIn("core_.resolve_contact(", self.runtime)

    def test_swept_solver_blocks_and_preserves_tangent_motion(self) -> None:
        self.assertIn("combined_radius", self.runtime)
        self.assertIn("discriminant", self.runtime)
        self.assertIn("candidate.fraction < earliest.fraction", self.runtime)
        self.assertIn("resolved_velocity = subtract(incoming_velocity", self.runtime)
        self.assertIn("normal_speed", self.runtime)

    def test_unreal_body_matches_authoritative_registered_body(self) -> None:
        self.assertIn("BootstrapBodyCenterXMeters = 50.0", self.environment_h)
        self.assertIn("BootstrapBodyRadiusMeters = 2.0", self.environment_h)
        self.assertIn("Core->add_static_sphere_body", self.adapter_cpp)
        self.assertIn("AEverwardPhase2TestEnvironment::BootstrapBodyCenterXMeters", self.adapter_cpp)
        self.assertIn("AEverwardPhase2TestEnvironment::BootstrapBodyRadiusMeters", self.adapter_cpp)
        self.assertIn("SetCollisionEnabled(ECollisionEnabled::QueryOnly)", self.environment_cpp)

    def test_probe_collision_envelope_is_not_decorative_mesh_collision(self) -> None:
        self.assertIn("ProbeCollisionEnvelope", self.pawn_h)
        self.assertIn("USphereComponent", self.pawn_h)
        self.assertIn("collision_envelope_radius_m{8.0}", self.types)
        self.assertIn("SetSphereRadius(800.0f)", self.pawn_cpp)
        self.assertIn("Component->SetCollisionEnabled(ECollisionEnabled::NoCollision)", self.pawn_cpp)
        self.assertNotIn("SetSimulatePhysics(true)", self.pawn_cpp)

    def test_contact_telemetry_crosses_adapter_boundary(self) -> None:
        for field in (
            "CollisionEnvelopeRadiusMeters",
            "bHasContactHistory",
            "LastContactBodyId",
            "LastContactPointMeters",
            "LastContactSurfaceNormal",
            "LastContactRelativeVelocityMetersPerSecond",
            "LastContactNormalSpeedMetersPerSecond",
            "LastContactTick",
        ):
            self.assertIn(field, self.adapter_h)
            self.assertIn(field, self.adapter_cpp)

    def test_unreal_presentation_does_not_author_contact_resolution(self) -> None:
        self.assertNotIn("AddImpulse", self.pawn_cpp)
        self.assertNotIn("SetPhysicsLinearVelocity", self.pawn_cpp)
        self.assertNotIn("OnComponentHit", self.pawn_cpp)
        self.assertNotIn("SimulatePhysics", self.environment_cpp)


if __name__ == "__main__":
    unittest.main()
