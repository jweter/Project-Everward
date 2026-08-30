#include "EverwardPlayerController.h"

#include "EverwardHUD.h"
#include "EverwardProbePawn.h"
#include "InputCoreTypes.h"
#include "ProbeSimulationAdapter.h"

void AEverwardPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    AEverwardProbePawn* Probe = Cast<AEverwardProbePawn>(GetPawn());
    AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    if (Probe != nullptr)
    {
        const bool bHighlightEnabled = EverwardHUD != nullptr && EverwardHUD->IsManipulatorPanelExpanded();
        const EEverwardManipulatorArmId ArmId = EverwardHUD != nullptr &&
            EverwardHUD->GetSelectedManipulatorArmIndex() == 1
                ? EEverwardManipulatorArmId::Starboard
                : EEverwardManipulatorArmId::Port;

        EEverwardManipulatorJoint Joint = EEverwardManipulatorJoint::Shoulder;
        if (EverwardHUD != nullptr)
        {
            switch (EverwardHUD->GetSelectedManipulatorJointIndex())
            {
                case 1: Joint = EEverwardManipulatorJoint::Elbow; break;
                case 2: Joint = EEverwardManipulatorJoint::Wrist; break;
                default: Joint = EEverwardManipulatorJoint::Shoulder; break;
            }
        }
        Probe->SetManipulatorSelectionHighlight(bHighlightEnabled, ArmId, Joint);
    }

    // First mining interaction. G is intentionally global during this early
    // physical-work slice: the player should be able to position the probe,
    // articulate the arm, and attempt extraction without navigating away from
    // the manipulator view. The command itself enforces scan/tool/reach/storage
    // requirements authoritatively and reports why an attempt fails.
    if (WasInputKeyJustPressed(EKeys::G))
    {
        if (UProbeSimulationAdapter* Adapter = GetProbeAdapter())
        {
            (void)Adapter->CommandMineBootstrapTarget();
        }
    }
}
