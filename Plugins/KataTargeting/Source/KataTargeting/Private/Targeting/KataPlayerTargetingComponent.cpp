#include "Targeting/KataPlayerTargetingComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "TargetingSystem/TargetingPreset.h"

#if ENABLE_DRAW_DEBUG
namespace
{
    TAutoConsoleVariable<int32> CVarKataTargetingDebug(
        TEXT("Kata.Targeting.Debug"),
        0,
        TEXT("Draw Kata targeting debug. 0: off, 1: lock and soft targets."),
        ECVF_Cheat);

    void DrawTargetDebug(const UWorld* World, const AActor* Owner, const AActor* Target, const FColor& Color, float Duration)
    {
        if (World == nullptr || Owner == nullptr || Target == nullptr || CVarKataTargetingDebug.GetValueOnGameThread() <= 0)
        {
            return;
        }
        DrawDebugSphere(World, Target->GetActorLocation(), 60.0f, 12, Color, false, Duration);
        DrawDebugLine(World, Owner->GetActorLocation(), Target->GetActorLocation(), Color, false, Duration);
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

AActor* UKataPlayerTargetingComponent::UpdateSoftTarget()
{
    AActor* NewSoftTarget = FindBestTarget(SoftTargetPreset);
    SoftTarget = NewSoftTarget;
#if ENABLE_DRAW_DEBUG
    DrawTargetDebug(GetWorld(), GetOwner(), NewSoftTarget, FColor::Yellow, SoftTargetDebugDuration);
#endif
    return NewSoftTarget;
}

bool UKataPlayerTargetingComponent::AcquireLock()
{
    AActor* Target = FindBestTarget(LockOnPreset);
    if (Target == nullptr || !IsLockTargetValid(Target))
    {
        return false;
    }
    SetLockTarget(Target);
    return true;
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
    SetLockTarget(nullptr);
}

AActor* UKataPlayerTargetingComponent::GetCurrentTarget_Implementation() const
{
    return LockTarget.IsValid() ? LockTarget.Get() : SoftTarget.Get();
}

AActor* UKataPlayerTargetingComponent::ResolveActionTarget_Implementation()
{
    if (AActor* Locked = LockTarget.Get())
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
    if (const AActor* Locked = LockTarget.Get())
    {
        return CurrentTarget == Locked;
    }

    FVector InputDirection;
    return !GetMoveInputDirection(InputDirection);
}

bool UKataPlayerTargetingComponent::ResolveFacingDirection_Implementation(AActor* ActionTarget, FVector& OutDirection) const
{
    if (const AActor* Locked = LockTarget.Get())
    {
        return GetDirectionToActor(Locked, OutDirection);
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

    const AActor* Locked = LockTarget.Get();
    if (!IsLockTargetValid(Locked))
    {
        HandleLockLost();
        return;
    }

#if ENABLE_DRAW_DEBUG
    // Tick 간격 동안 선이 남아 있어야 깜빡이지 않는다.
    DrawTargetDebug(GetWorld(), GetOwner(), Locked, FColor::Red, FMath::Max(PrimaryComponentTick.TickInterval, DeltaTime) + 0.02f);
#endif
}

void UKataPlayerTargetingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 종료 중에는 구독만 정리하고 변경 알림을 보내지 않는다. 구독자도 함께 정리되는 중일 수 있다.
    if (AActor* Locked = LockTarget.Get())
    {
        Locked->OnEndPlay.RemoveDynamic(this, &UKataPlayerTargetingComponent::HandleLockTargetEndPlay);
    }
    LockTarget.Reset();
    SoftTarget.Reset();
    Super::EndPlay(EndPlayReason);
}

void UKataPlayerTargetingComponent::SetLockTarget(AActor* NewTarget)
{
    AActor* OldTarget = LockTarget.Get();
    // 대상이 파괴돼 약한 참조만 비었을 때는 OldTarget도 nullptr이다. 이 경우에도 Tick을 끄고 해제를 알려야 한다.
    if (OldTarget == NewTarget && (NewTarget != nullptr || LockTarget.IsExplicitlyNull()))
    {
        return;
    }

    if (OldTarget != nullptr)
    {
        OldTarget->OnEndPlay.RemoveDynamic(this, &UKataPlayerTargetingComponent::HandleLockTargetEndPlay);
    }
    LockTarget = NewTarget;
    if (NewTarget != nullptr)
    {
        NewTarget->OnEndPlay.AddUniqueDynamic(this, &UKataPlayerTargetingComponent::HandleLockTargetEndPlay);
    }
    SetComponentTickEnabled(NewTarget != nullptr);

    OnLockTargetChanged.Broadcast(OldTarget, NewTarget);
}

bool UKataPlayerTargetingComponent::SwitchLock(const UTargetingPreset* Preset)
{
    AActor* Current = LockTarget.Get();
    if (Current == nullptr)
    {
        return false;
    }

    AActor* Target = FindBestTarget(Preset);
    if (Target == nullptr || Target == Current || !IsLockTargetValid(Target))
    {
        return false;
    }
    SetLockTarget(Target);
    return true;
}

bool UKataPlayerTargetingComponent::IsLockTargetValid(const AActor* Target) const
{
    if (!IsValid(Target) || Target->IsActorBeingDestroyed())
    {
        return false;
    }

    const AActor* Owner = GetOwner();
    if (MaxLockDistance > 0.0f && Owner != nullptr
        && FVector::DistSquared(Owner->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(MaxLockDistance))
    {
        return false;
    }

    if (!LockBreakTags.IsEmpty())
    {
        const UAbilitySystemComponent* AbilitySystem =
            UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
        if (AbilitySystem != nullptr && AbilitySystem->HasAnyMatchingGameplayTags(LockBreakTags))
        {
            return false;
        }
    }
    return true;
}

void UKataPlayerTargetingComponent::HandleLockLost()
{
    AActor* LostTarget = LockTarget.Get();
    AActor* NextTarget = nullptr;
    if (LockLostBehavior == EKataLockLostBehavior::SwitchToNext)
    {
        TArray<AActor*> Candidates;
        FindTargets(LockOnPreset, Candidates);
        for (AActor* Candidate : Candidates)
        {
            // 잃은 대상이 아직 월드에 남아 후보로 나올 수 있으므로 제외한다.
            if (Candidate != LostTarget && IsLockTargetValid(Candidate))
            {
                NextTarget = Candidate;
                break;
            }
        }
    }
    SetLockTarget(NextTarget);
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
    if (Actor == LockTarget.Get())
    {
        HandleLockLost();
    }
}
