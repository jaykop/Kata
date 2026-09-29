#include "KataCameraPlacement.h"

#include "KataCameraTypes.h"

void UKataCameraPlacement_BoomArm::Evaluate(FKataCameraPipelineContext& Context) const
{
    Context.CameraRotation = Context.ViewRotation;
    Context.CameraLocation = Context.PivotLocation - Context.ViewRotation.Vector() * Distance;
}
