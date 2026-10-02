#include "Targeting/KataPlayerTargetingComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Targeting/KataTargetPointComponent.h"
#include "TargetingSystem/TargetingPreset.h"

#if ENABLE_DRAW_DEBUG
namespace
{
    TAutoConsoleVariable<int32> CVarKataTargetingDebug(
        TEXT("Kata.Targeting.Debug"),
        0,
        TEXT("Draw Kata targeting debug. 0: off, 1: lock point and soft target."),
        ECVF_Cheat);

    void DrawTargetDebug(const UWorld* World, const AActor* Owner, const FVector& TargetLocation, const FColor& Color, float Duration)
    {
        if (World == nullptr || Owner == nullptr || CVarKataTargetingDebug.GetValueOnGameThread() <= 0)
        {
            return;
        }
        DrawDebugSphere(World, TargetLocation, 60.0f, 12, Color, false, Duration);
        DrawDebugLine(World, Owner->GetActorLocation(), TargetLocation, Color, false, Duration);
    }

    /** 소프트 타겟은 액션 시작 때만 갱신되므로 확인할 수 있을 만큼 남겨 둔다. */
    constexpr float SoftTargetDebugDuration = 1.0f;
}
#endif

UKataPlayerTargetingComponent::UKataPlayerTargetingComponent()
{
    // 락온 중에만 켠다. 간격이 곧 락온 유효성 검사 주기다.
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickInterval = 0.1f;
}

AActor* UKataPlayerTargetingComponent::GetLockTarget() const
{
    const UKataTargetPointComponent* Point = LockPoint.Get();
    return Point != nullptr ? Point->GetOwner() : nullptr;
}

AActor* UKataPlayerTargetingComponent::UpdateSoftTarget()
{
    AActor* NewSoftTarget = FindBestTarget(SoftTargetPreset);
    SoftTarget = NewSoftTarget;
#if ENABLE_DRAW_DEBUG
    if (NewSoftTarget != nullptr)
    {
        DrawTargetDebug(GetWorld(), GetOwner(), NewSoftTarget->GetActorLocation(), FColor::Yellow, SoftTargetDebugDuration);
    }
#endif
    return NewSoftTarget;
}

bool UKataPlayerTargetingComponent::AcquireLock()
{
    TArray<UKataTargetPointComponent*> Candidates;
    FindTargetPoints(LockOnPreset, Candidates);
    for (UKataTargetPointComponent* Candidate : Candidates)
    {
        if (IsLockPointValid(Candidate))
        {
            SetLockPoint(Candidate);
            return true;
        }
    }
    return false;
}

bool UKataPlayerTargetingComponent::SwitchLockLeft()
{
    return SwitchLock(SwitchLeftPreset);
}

bool UKataPlayerTargetingComponent::SwitchLockRight()
{
    return SwitchLock(SwitchRightPreset);
}

void UKataPlayerTargetingComponent::ReleaseLock()
{
    SetLockPoint(nullptr);
}

void UKataPlayerTargetingComponent::GetLockOnCandidates(TArray<UKataTargetPointComponent*>& OutPoints) const
{
    FindTargetPoints(LockOnPreset, OutPoints);
    OutPoints.RemoveAll([this](const UKataTargetPointComponent* Point)
    {
        return !IsLockPointValid(Point);
    });
}

AActor* UKataPlayerTargetingComponent::GetCurrentTarget_Implementation() const
{
    AActor* Locked = GetLockTarget();
    return Locked != nullptr ? Locked : SoftTarget.Get();
}

AActor* UKataPlayerTargetingComponent::ResolveActionTarget_Implementation()
{
    if (AActor* Locked = GetLockTarget())
    {
        return Locked;
    }

    // 소프트 타겟은 방향 기준일 뿐이다. 입력 방향이 우선하는 동안에는 정하지 않아 대상과 공격 방향이 어긋나지 않게 한다.
    FVector InputDirection;
    if (GetMoveInputDirection(InputDirection))
    {
        SoftTarget.Reset();
        return nullptr;
    }
    return UpdateSoftTarget();
}

bool UKataPlayerTargetingComponent::CanKeepActionTarget_Implementation(AActor* CurrentTarget) const
{
    if (const AActor* Locked = GetLockTarget())
    {
        return CurrentTarget == Locked;
    }

    FVector InputDirection;
    return !GetMoveInputDirection(InputDirection);
}

bool UKataPlayerTargetingComponent::ResolveFacingDirection_Implementation(AActor* ActionTarget, FVector& OutDirection) const
{
    // 큰 대상은 부위마다 수평 위치가 다르므로 액터 원점이 아니라 락온 지점을 향한다.
    if (const UKataTargetPointComponent* Point = LockPoint.Get())
    {
        return GetDirectionToLocation(Point->GetComponentLocation(), OutDirection);
    }
    if (GetMoveInputDirection(OutDirection))
    {
        return true;
    }
    return Super::ResolveFacingDirection_Implementation(ActionTarget, OutDirection);
}

void UKataPlayerTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const UKataTargetPointComponent* Point = LockPoint.Get();
    if (!IsLockPointValid(Point))
    {
        HandleLockLost(Point != nullptr && !Point->IsTargetPointEnabled() ? LockPointDisabledBehavior : LockLostBehavior);
        return;
    }

#if ENABLE_DRAW_DEBUG
    // Tick 간격 동안 선이 남아 있어야 깜빡이지 않는다.
    DrawTargetDebug(GetWorld(), GetOwner(), Point->GetComponentLocation(), FColor::Red,
        FMath::Max(PrimaryComponentTick.TickInterval, DeltaTime) + 0.02f);
#endif
}

void UKataPlayerTargetingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 종료 중에는 구독과 태그만 정리하고 변경 알림을 보내지 않는다. 구독자도 함께 정리되는 중일 수 있다.
    if (AActor* PointOwner = LockPointOwner.Get())
    {
        PointOwner->OnEndPlay.RemoveDynamic(this, &UKataPlayerTargetingComponent::HandleLockTargetEndPlay);
    }
    if (UKataTargetPointComponent* Point = LockPoint.Get())
    {
        Point->OnEnabledChanged.RemoveDynamic(this, &UKataPlayerTargetingComponent::HandleLockPointEnabledChanged);
    }
    ClearStatusTags();
    LockPoint.Reset();
    LockPointOwner.Reset();
    SoftTarget.Reset();
    Super::EndPlay(EndPlayReason);
}

void UKataPlayerTargetingComponent::SetLockPoint(UKataTargetPointComponent* NewPoint)
{
    UKataTargetPointComponent* OldPoint = LockPoint.Get();
    // 지점이 파괴돼 약한 참조만 비었을 때는 OldPoint도 nullptr이다. 이 경우에도 Tick을 끄고 해제를 알려야 한다.
    if (OldPoint == NewPoint && (NewPoint != nullptr || LockPoint.IsExplicitlyNull()))
    {
        return;
    }

    if (AActor* OldOwner = LockPointOwner.Get())
    {
        OldOwner->OnEndPlay.RemoveDynamic(this, &UKataPlayerTargetingComponent::HandleLockTargetEndPlay);
    }
    if (OldPoint != nullptr)
    {
        OldPoint->OnEnabledChanged.RemoveDynamic(this, &UKataPlayerTargetingComponent::HandleLockPointEnabledChanged);
    }

    AActor* NewOwner = NewPoint != nullptr ? NewPoint->GetOwner() : nullptr;
    LockPoint = NewPoint;
    LockPointOwner = NewOwner;
    if (NewOwner != nullptr)
    {
        NewOwner->OnEndPlay.AddUniqueDynamic(this, &UKataPlayerTargetingComponent::HandleLockTargetEndPlay);
    }
    if (NewPoint != nullptr)
    {
        NewPoint->OnEnabledChanged.AddUniqueDynamic(this, &UKataPlayerTargetingComponent::HandleLockPointEnabledChanged);
    }
    SetComponentTickEnabled(NewPoint != nullptr);
    UpdateStatusTags(NewOwner);

    OnLockTargetChanged.Broadcast(OldPoint, NewPoint);
}

bool UKataPlayerTargetingComponent::SwitchLock(const UTargetingPreset* Preset)
{
    const UKataTargetPointComponent* Current = LockPoint.Get();
    if (Current == nullptr)
    {
        return false;
    }

    TArray<UKataTargetPointComponent*> Candidates;
    FindTargetPoints(Preset, Candidates);
    for (UKataTargetPointComponent* Candidate : Candidates)
    {
        // 같은 액터의 다른 부위도 후보다. 현재 지점만 뺀다.
        if (Candidate != Current && IsLockPointValid(Candidate))
        {
            SetLockPoint(Candidate);
            return true;
        }
    }
    return false;
}

bool UKataPlayerTargetingComponent::IsLockPointValid(const UKataTargetPointComponent* Point) const
{
    if (!IsValid(Point) || !Point->IsTargetPointEnabled())
    {
        return false;
    }

    const AActor* Target = Point->GetOwner();
    if (!IsValid(Target) || Target->IsActorBeingDestroyed())
    {
        return false;
    }

    const AActor* Owner = GetOwner();
    if (MaxLockDistance > 0.0f && Owner != nullptr
        && FVector::DistSquared(Owner->GetActorLocation(), Point->GetComponentLocation()) > FMath::Square(MaxLockDistance))
    {
        return false;
    }

    if (!LockBreakTags.IsEmpty())
    {
        const UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
        if (AbilitySystem != nullptr && AbilitySystem->HasAnyMatchingGameplayTags(LockBreakTags))
        {
            return false;
        }
    }
    return true;
}

void UKataPlayerTargetingComponent::HandleLockLost(EKataLockLostBehavior Behavior)
{
    const UKataTargetPointComponent* LostPoint = LockPoint.Get();
    UKataTargetPointComponent* NextPoint = nullptr;
    if (Behavior == EKataLockLostBehavior::SwitchToNext)
    {
        TArray<UKataTargetPointComponent*> Candidates;
        FindTargetPoints(LockOnPreset, Candidates);
        for (UKataTargetPointComponent* Candidate : Candidates)
        {
            // 잃은 지점이 아직 월드에 남아 후보로 나올 수 있으므로 제외한다.
            if (Candidate != LostPoint && IsLockPointValid(Candidate))
            {
                NextPoint = Candidate;
                break;
            }
        }
    }
    SetLockPoint(NextPoint);
}

void UKataPlayerTargetingComponent::UpdateStatusTags(AActor* NewTargetActor)
{
    UAbilitySystemComponent* OwnAbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
    const bool bLocked = NewTargetActor != nullptr;

    // Loose 태그는 개수로 관리되므로 붙인 횟수만큼만 뗀다. 락온 중 지점만 바뀌면 소유자 태그를 다시 붙이지 않는다.
    if (bLocked && !AppliedLockingTag.IsValid() && LockingStatusTag.IsValid() && OwnAbilitySystem != nullptr)
    {
        OwnAbilitySystem->AddLooseGameplayTag(LockingStatusTag);
        AppliedLockingTag = LockingStatusTag;
    }
    else if (!bLocked && AppliedLockingTag.IsValid())
    {
        if (OwnAbilitySystem != nullptr)
        {
            OwnAbilitySystem->RemoveLooseGameplayTag(AppliedLockingTag);
        }
        AppliedLockingTag = FGameplayTag();
    }

    // 같은 액터의 다른 부위로 옮길 때는 대상 태그를 떼고 다시 붙이지 않아 태그 변화 이벤트가 생기지 않게 한다.
    if (TaggedTargetActor.Get() == NewTargetActor && (NewTargetActor != nullptr || TaggedTargetActor.IsExplicitlyNull()))
    {
        return;
    }
    if (AppliedTargetedTag.IsValid())
    {
        if (UAbilitySystemComponent* OldAbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TaggedTargetActor.Get()))
        {
            OldAbilitySystem->RemoveLooseGameplayTag(AppliedTargetedTag);
        }
        AppliedTargetedTag = FGameplayTag();
    }
    TaggedTargetActor.Reset();

    if (NewTargetActor != nullptr && TargetedStatusTag.IsValid())
    {
        if (UAbilitySystemComponent* NewAbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(NewTargetActor))
        {
            NewAbilitySystem->AddLooseGameplayTag(TargetedStatusTag);
            AppliedTargetedTag = TargetedStatusTag;
            TaggedTargetActor = NewTargetActor;
        }
    }
}

void UKataPlayerTargetingComponent::ClearStatusTags()
{
    if (AppliedLockingTag.IsValid())
    {
        if (UAbilitySystemComponent* OwnAbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
        {
            OwnAbilitySystem->RemoveLooseGameplayTag(AppliedLockingTag);
        }
        AppliedLockingTag = FGameplayTag();
    }
    if (AppliedTargetedTag.IsValid())
    {
        if (UAbilitySystemComponent* TargetAbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TaggedTargetActor.Get()))
        {
            TargetAbilitySystem->RemoveLooseGameplayTag(AppliedTargetedTag);
        }
        AppliedTargetedTag = FGameplayTag();
    }
    TaggedTargetActor.Reset();
}

bool UKataPlayerTargetingComponent::GetMoveInputDirection(FVector& OutDirection) const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (Pawn == nullptr)
    {
        return false;
    }

    // 공격 입력과 이동 입력의 처리 순서는 보장되지 않는다. 이번 프레임 입력이 아직 없으면 직전 프레임 입력을 쓴다.
    FVector Direction = Pawn->GetPendingMovementInputVector();
    Direction.Z = 0.0f;
    if (Direction.IsNearlyZero())
    {
        Direction = Pawn->GetLastMovementInputVector();
        Direction.Z = 0.0f;
    }
    if (!Direction.Normalize())
    {
        return false;
    }
    OutDirection = Direction;
    return true;
}

void UKataPlayerTargetingComponent::HandleLockTargetEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
    if (Actor == LockPointOwner.Get())
    {
        HandleLockLost(LockLostBehavior);
    }
}

void UKataPlayerTargetingComponent::HandleLockPointEnabledChanged(UKataTargetPointComponent* Point, bool bEnabled)
{
    if (!bEnabled && Point == LockPoint.Get())
    {
        HandleLockLost(LockPointDisabledBehavior);
    }
}
