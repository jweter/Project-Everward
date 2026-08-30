#include "EverwardProbePawn.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProbeSimulationAdapter.h"

namespace
{
const FLinearColor JointHardwareColor(0.12f, 0.13f, 0.14f, 1.0f);
const FLinearColor ArmStructureColor(0.27f, 0.29f, 0.31f, 1.0f);
const FLinearColor ToolMaterialColor(0.16f, 0.12f, 0.09f, 1.0f);
const FLinearColor SelectionHighlightColor(0.08f, 0.82f, 1.0f, 1.0f);

void SetComponentColor(UStaticMeshComponent* Component, const FLinearColor& Color)
{
    if (Component == nullptr)
    {
        return;
    }

    UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0));
    if (Dynamic == nullptr)
    {
        return;
    }

    // The current Prime functional-material scaffold writes both names because
    // the engine basic-shape material varies by engine version. Mirror that
    // convention here so selection feedback is robust against the same source.
    Dynamic->SetVectorParameterValue(TEXT("Color"), Color);
    Dynamic->SetVectorParameterValue(TEXT("BaseColor"), Color);
}
}

void AEverwardProbePawn::SetManipulatorSelectionHighlight(
    bool bEnabled,
    EEverwardManipulatorArmId ArmId,
    EEverwardManipulatorJoint Joint)
{
    // Restore the canonical Prime material family every call. The controller
    // refreshes this presentation state from the HUD selection, so closing the
    // manipulator panel immediately removes all selection color.
    SetComponentColor(PortShoulder, JointHardwareColor);
    SetComponentColor(StarboardShoulder, JointHardwareColor);
    SetComponentColor(PortUpperArm, ArmStructureColor);
    SetComponentColor(StarboardUpperArm, ArmStructureColor);
    SetComponentColor(PortForearm, ArmStructureColor);
    SetComponentColor(StarboardForearm, ArmStructureColor);
    SetComponentColor(PortToolHead, ToolMaterialColor);
    SetComponentColor(StarboardToolHead, ToolMaterialColor);

    if (!bEnabled)
    {
        return;
    }

    const bool bPort = ArmId == EEverwardManipulatorArmId::Port;
    UStaticMeshComponent* Shoulder = bPort ? PortShoulder.Get() : StarboardShoulder.Get();
    UStaticMeshComponent* UpperArm = bPort ? PortUpperArm.Get() : StarboardUpperArm.Get();
    UStaticMeshComponent* Forearm = bPort ? PortForearm.Get() : StarboardForearm.Get();
    UStaticMeshComponent* ToolHead = bPort ? PortToolHead.Get() : StarboardToolHead.Get();

    switch (Joint)
    {
        case EEverwardManipulatorJoint::Shoulder:
            // Shoulder motion carries the upper arm, so light both the joint
            // housing and the first driven segment.
            SetComponentColor(Shoulder, SelectionHighlightColor);
            SetComponentColor(UpperArm, SelectionHighlightColor);
            break;
        case EEverwardManipulatorJoint::Elbow:
            // The elbow command rotates the forearm and everything distal.
            // Highlight the directly-driven segment rather than the whole arm.
            SetComponentColor(Forearm, SelectionHighlightColor);
            break;
        case EEverwardManipulatorJoint::Wrist:
            // The current Prime blockout has no separate wrist housing mesh;
            // the tool head is the visible wrist-driven element.
            SetComponentColor(ToolHead, SelectionHighlightColor);
            break;
    }
}
