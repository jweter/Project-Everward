from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal/Source/Everward"


class PrimeASimpleEmbodimentSurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.pawn_h = (SOURCE / "EverwardProbePawn.h").read_text(encoding="utf-8")
        self.pawn_cpp = (SOURCE / "EverwardProbePawn.cpp").read_text(encoding="utf-8")

    def test_prime_a_reads_as_one_tube_with_wings_engine_and_sensor(self) -> None:
        for token in (
            "PrimeCentralTube",
            "PortThermalWing",
            "StarboardThermalWing",
            "MainPropulsionAssembly",
            "ForwardScienceSensor",
        ):
            self.assertIn(token, self.pawn_cpp)
        self.assertIn("CylinderMeshAsset", self.pawn_cpp)
        self.assertIn("ProbeMesh->SetRelativeRotation(FRotator(90.0, 0.0, 0.0))", self.pawn_cpp)
        self.assertNotIn("ProbeMesh->SetRelativeRotation(FRotator(0.0, 90.0, 0.0))", self.pawn_cpp)

    def test_manipulator_geometry_is_present_and_driven_by_authoritative_state(self) -> None:
        for token in (
            "PortUpperArm",
            "PortForearm",
            "PortToolHead",
            "StarboardUpperArm",
            "StarboardForearm",
            "StarboardToolHead",
            "UpdateManipulatorVisuals",
            "GetManipulatorArmStates",
            "State.DeploymentFraction",
            "State.ShoulderDegrees",
            "State.ElbowDegrees",
            "State.WristDegrees",
        ):
            self.assertIn(token, self.pawn_h + self.pawn_cpp)

    def test_visual_meshes_do_not_take_over_mechanical_truth(self) -> None:
        self.assertIn("SetCollisionEnabled(ECollisionEnabled::NoCollision)", self.pawn_cpp)
        self.assertIn("ProbeCollisionEnvelope", self.pawn_h)
        self.assertIn("SetSphereRadius(800.0f)", self.pawn_cpp)
        self.assertNotIn("SetSimulatePhysics(true)", self.pawn_cpp)
        self.assertNotIn("AddImpulse", self.pawn_cpp)

    def test_existing_functional_material_families_survive_on_new_body(self) -> None:
        for token in (
            "StructuralAlloy",
            "ProtectedCore",
            "EngineRefractory",
            "OpticalSurface",
            "RadiatorSurface",
            "ArmStructure",
            "ToolMaterial",
        ):
            self.assertIn(token, self.pawn_cpp)


if __name__ == "__main__":
    unittest.main()
