"""Repository-native source contracts. They do not claim a successful UBT compile."""
from pathlib import Path
import unittest

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
UNREAL = ROOT / "unreal/Source/Everward"
SIM = ROOT / "src/simulation/include/everward/simulation"


class UnrealMechanicalBridgeSurfaceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn_h = (UNREAL / "EverwardProbePawn.h").read_text(encoding="utf-8")
        cls.pawn_cpp = (UNREAL / "EverwardProbePawn.cpp").read_text(encoding="utf-8")
        cls.adapter = (UNREAL / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        cls.contact = (SIM / "manipulator_tool_contact.hpp").read_text(encoding="utf-8")

    def test_imported_visuals_are_optional_soft_mesh_assets(self):
        for name in (
            "ProceduralUpperArmMesh", "ProceduralForearmMesh",
            "ProceduralMiningToolMesh",
        ):
            self.assertIn(f"TSoftObjectPtr<UStaticMesh> {name};", self.pawn_h)
            self.assertIn(f"ApplyModule({name},", self.pawn_cpp)
        self.assertIn("if (Source.IsNull()) return;", self.pawn_cpp)
        self.assertIn("Source.LoadSynchronous()", self.pawn_cpp)
        self.assertIn("if (Mesh == nullptr)", self.pawn_cpp)

    def test_presentation_reuses_existing_telemetry_and_pivots(self):
        start = self.pawn_cpp.index("void AEverwardProbePawn::UpdateManipulatorVisuals()")
        end = self.pawn_cpp.index("void AEverwardProbePawn::Tick(", start)
        visuals = self.pawn_cpp[start:end]
        self.assertIn("SimulationAdapter->GetManipulatorArmStates()", visuals)
        self.assertIn("ShoulderPivot->SetRelativeRotation(", visuals)
        self.assertIn("ElbowPivot->SetRelativeRotation(", visuals)
        self.assertIn("WristPivot->SetRelativeRotation(", visuals)
        self.assertIn("bPort ? PortShoulderPivot", visuals)
        self.assertIn("bPort ? PortToolHead", visuals)

    def test_existing_collision_and_tool_tip_model_stays_authoritative(self):
        self.assertIn("kTipOffsetFromWristM = 0.79", self.contact)
        self.assertIn("manipulator_tool_tip_contact_sample(", self.contact)
        self.assertIn("Component->SetCollisionEnabled(ECollisionEnabled::NoCollision)", self.pawn_cpp)
        begin = self.pawn_cpp.split("void AEverwardProbePawn::BeginPlay()", 1)[1]
        self.assertLess(begin.index("ApplyProceduralArmMeshesIfAssigned()"),
                        begin.index("UpdateManipulatorVisuals()"))
        before = self.pawn_cpp.split("void AEverwardProbePawn::ApplyProceduralArmMeshesIfAssigned()", 1)[1]
        module = before.split("void AEverwardProbePawn::ApplyPrimeFunctionalMaterials()", 1)[0]
        self.assertNotIn("CommandAdjust", module)
        self.assertNotIn("CommandMine", module)
        self.assertNotIn("SetRelativeLocation(", module)
        self.assertNotIn("SetRelativeRotation(", module)
        self.assertNotIn("SetCollision", module)
        self.assertIn("FEverwardManipulatorArmState", self.adapter)


if __name__ == "__main__":
    unittest.main()
