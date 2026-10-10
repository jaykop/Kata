#include "KataRuntimeSpawnLayout.h"

namespace KataRuntimeSpawnLayout
{
    namespace
    {
        FTransform MakeFacingTransform(const FVector& Location, const FVector& PlayerLocation, float YawOffset)
        {
            const FVector ToPlayer = (PlayerLocation - Location).GetSafeNormal2D();
            const float FacingYaw = ToPlayer.IsNearlyZero() ? 0.f : ToPlayer.Rotation().Yaw;
            return FTransform(FRotator(0.f, FacingYaw + YawOffset, 0.f), Location);
        }
    }

    void Build(const FVector& PlayerLocation, float PlayerYaw, const FKataRuntimeSpawnLayout& Layout, TArray<FTransform>& OutTransforms)
    {
        OutTransforms.Reset();
        const int32 Count = FMath::Max(Layout.Count, 1);
        const float Distance = FMath::Max(Layout.Distance, 0.f);
        const float Spacing = FMath::Max(Layout.Spacing, 0.f);
        OutTransforms.Reserve(Count);

        if (Layout.Pattern == EKataRuntimeSpawnPattern::Circle && Distance > UE_KINDA_SMALL_NUMBER)
        {
            const float Circumference = 2.f * UE_PI * Distance;
            const bool bFullCircle = Spacing <= 0.f || Count * Spacing >= Circumference;
            // 균등 배치는 정면을 첫 개체로 두고, 호 배치는 정면을 가운데로 좌우 대칭이 되게 한다.
            const float StepDegrees = bFullCircle ? 360.f / Count : FMath::RadiansToDegrees(Spacing / Distance);
            const float StartDegrees = bFullCircle ? 0.f : -0.5f * StepDegrees * (Count - 1);
            for (int32 Index = 0; Index < Count; ++Index)
            {
                const float Yaw = PlayerYaw + StartDegrees + StepDegrees * Index;
                const FVector Location = PlayerLocation + FRotator(0.f, Yaw, 0.f).Vector() * Distance;
                OutTransforms.Add(MakeFacingTransform(Location, PlayerLocation, Layout.YawOffset));
            }
            return;
        }

        const FRotator PlayerRotation(0.f, PlayerYaw, 0.f);
        const FVector Forward = PlayerRotation.Vector();
        const FVector Right = FRotationMatrix(PlayerRotation).GetUnitAxis(EAxis::Y);
        const FVector Center = PlayerLocation + Forward * Distance;
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const float Offset = (Index - 0.5f * (Count - 1)) * Spacing;
            OutTransforms.Add(MakeFacingTransform(Center + Right * Offset, PlayerLocation, Layout.YawOffset));
        }
    }
}
