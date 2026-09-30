#include "KataCameraRailComponent.h"

UKataCameraRailComponent::UKataCameraRailComponent()
{
    SetClosedLoop(false, false);
}

FVector UKataCameraRailComponent::GetOrbitOffsetAtDistance(float Distance) const
{
    const FVector LocalPosition = GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
    return GetRelativeRotation().RotateVector(LocalPosition * GetRelativeScale3D());
}
