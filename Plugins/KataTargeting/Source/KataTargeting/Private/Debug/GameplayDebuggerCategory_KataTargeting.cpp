#include "Debug/GameplayDebuggerCategory_KataTargeting.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Targeting/KataPlayerTargetingComponent.h"
#include "Targeting/KataTargetPointComponent.h"

namespace
{
    FString DescribePoint(const UKataTargetPointComponent* Point)
    {
        return FString::Printf(TEXT("%s.%s"), *GetNameSafe(Point->GetOwner()), *Point->GetName());
    }
}

FGameplayDebuggerCategory_KataTargeting::FGameplayDebuggerCategory_KataTargeting()
{
    // 락온은 플레이어 단위라 디버그 대상 액터를 고르지 않아도 표시한다.
    bShowOnlyWithDebugActor = false;
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_KataTargeting::MakeInstance()
{
    return MakeShareable(new FGameplayDebuggerCategory_KataTargeting());
}

void FGameplayDebuggerCategory_KataTargeting::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
    const APawn* Pawn = OwnerPC != nullptr ? OwnerPC->GetPawn() : nullptr;
    const UKataPlayerTargetingComponent* Targeting = Pawn != nullptr ? Pawn->FindComponentByClass<UKataPlayerTargetingComponent>() : nullptr;
    if (Targeting == nullptr)
    {
        AddTextLine(TEXT("{red}Player pawn has no UKataPlayerTargetingComponent"));
        return;
    }

    const UKataTargetPointComponent* LockPoint = Targeting->GetLockPoint();
    AddTextLine(LockPoint != nullptr
        ? FString::Printf(TEXT("{white}Lock Point: {red}%s"), *DescribePoint(LockPoint))
        : FString(TEXT("{white}Lock Point: {grey}none")));
    AddTextLine(FString::Printf(TEXT("{white}Soft Target: {yellow}%s"), *GetNameSafe(Targeting->GetSoftTarget())));

    TArray<UKataTargetPointComponent*> Candidates;
    Targeting->GetLockOnCandidates(Candidates);
    if (Candidates.IsEmpty())
    {
        AddTextLine(TEXT("{white}Lock-On Candidates: {grey}none (check Lock On Preset and Kata Expand Target Points)"));
    }
    for (int32 Index = 0; Index < Candidates.Num(); ++Index)
    {
        const UKataTargetPointComponent* Point = Candidates[Index];
        const bool bIsLockPoint = Point == LockPoint;
        AddTextLine(FString::Printf(TEXT("{white}Candidate #%d: %s%s"), Index + 1, bIsLockPoint ? TEXT("{red}") : TEXT("{green}"), *DescribePoint(Point)));
        AddShape(FGameplayDebuggerShape::MakePoint(Point->GetComponentLocation(), 10.0f,
            bIsLockPoint ? FColor::Red : FColor::Green, FString::Printf(TEXT("#%d"), Index + 1)));
    }

    // 후보에서 빠졌지만 아직 잡고 있는 지점(거리 초과 직전 등)도 위치를 보인다.
    if (LockPoint != nullptr && !Candidates.Contains(LockPoint))
    {
        AddShape(FGameplayDebuggerShape::MakePoint(LockPoint->GetComponentLocation(), 10.0f, FColor::Red, TEXT("Lock")));
    }
}

#endif // WITH_GAMEPLAY_DEBUGGER
