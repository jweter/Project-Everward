from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
UNREAL = ROOT / "unreal"
SOURCE = UNREAL / "Source" / "Everward"


class OneProbeProductionBootstrapTests(unittest.TestCase):
    def test_project_selects_the_production_game_mode(self) -> None:
        config = (UNREAL / "Config" / "DefaultEngine.ini").read_text(encoding="utf-8")
        self.assertIn(
            "GlobalDefaultGameMode=/Script/Everward.EverwardGameMode",
            config,
        )

    def test_game_mode_spawns_the_one_probe_pawn(self) -> None:
        game_mode = (SOURCE / "EverwardGameMode.cpp").read_text(encoding="utf-8")
        self.assertIn("DefaultPawnClass = AEverwardProbePawn::StaticClass();", game_mode)

    def test_probe_presentation_owns_exactly_one_adapter_and_one_root_boundary(self) -> None:
        pawn = (SOURCE / "EverwardProbePawn.cpp").read_text(encoding="utf-8")
        adapter_constructions = re.findall(
            r"CreateDefaultSubobject<UProbeSimulationAdapter>", pawn
        )
        self.assertEqual(adapter_constructions, ["CreateDefaultSubobject<UProbeSimulationAdapter>"])

        # The Prime A body now has a hierarchical manipulator presentation, but
        # the pawn still owns exactly one top-level presentation root and one
        # authoritative adapter. Child meshes may attach to ProbeRoot or to
        # manipulator pivots beneath that root.
        self.assertEqual(
            pawn.count('CreateDefaultSubobject<USceneComponent>(TEXT("ProbeRoot"))'),
            1,
        )
        self.assertEqual(pawn.count("SetRootComponent(ProbeRoot);"), 1)
        self.assertIn("Component->SetupAttachment(Parent);", pawn)
        self.assertIn("ConfigureMesh(ProbeMesh, ProbeRoot);", pawn)
        self.assertIn("ShoulderPivot->SetupAttachment(ProbeRoot);", pawn)
        self.assertNotIn("SetRootComponent(ProbeMesh);", pawn)

        self.assertGreaterEqual(
            pawn.count("CreateDefaultSubobject<UStaticMeshComponent>"),
            10,
        )
        self.assertNotIn("SetSimulatePhysics(true)", pawn)

    def test_presentation_does_not_bypass_the_adapter_boundary(self) -> None:
        for filename in (
            "EverwardGameMode.h",
            "EverwardGameMode.cpp",
            "EverwardProbePawn.h",
            "EverwardProbePawn.cpp",
        ):
            with self.subTest(filename=filename):
                source = (SOURCE / filename).read_text(encoding="utf-8")
                self.assertNotIn('#include "everward/simulation/', source)
                self.assertNotIn("everward::simulation", source)

    def test_adapter_uses_the_canonical_policy_aware_probe_runtime(self) -> None:
        adapter = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        self.assertIn("ProbeRuntime::make_canonical_ev0001()", adapter)
        self.assertIn('everward/simulation/software_policy.hpp', adapter)


if __name__ == "__main__":
    unittest.main()
