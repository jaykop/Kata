#include "Targeting/KataAITargetingComponent.h"

#include "Data/KataAIData.h"
#include "Engine/World.h"
#include "FunctionLibraries/KataFL_Faction.h"
#include "GameFramework/Actor.h"
#include "KataAILog.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"

void UKataAITargetingComponent::InitializeKataAI(UKataAIData* InData, UAIPerceptionComponent* InPerception)
{
    ResetKataAI();
    AIData = InData;
    Perception = InPerception;
    HomeLocation = GetOwner()->GetActorLocation();
    if (AIData != nullptr && AIData->TargetingPreset == nullptr)
    {
        UE_LOG(LogKataAI, Warning, TEXT("AI Data '%s' has no Targeting Preset; AI targets will remain empty."), *AIData->GetName());
    }
    if (AIData != nullptr && InPerception != nullptr && GetWorld() != nullptr)
    {
        RefreshPerception();
        GetWorld()->GetTimerManager().SetTimer(RefreshTimer, this, &UKataAITargetingComponent::RefreshPerception,
            FMath::Max(AIData->TargetRefreshInterval, 0.01f), true);
    }
}

void UKataAITargetingComponent::ResetKataAI()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RefreshTimer);
    }
    AIData = nullptr;
    Perception.Reset();
    VisibleActors.Reset();
    CurrentTarget.Reset();
    bHadSelectedTarget = false;
    LastSeenTarget.Reset();
    LastKnownLocation = FVector::ZeroVector;
    HomeLocation = FVector::ZeroVector;
    LastSeenTime = 0.0f;
    MoveRetryCount = 0;
}

void UKataAITargetingComponent::UpdateSight(AActor* Actor, bool bVisible, const FVector& ObservedLocation)
{
    if (!IsValid(Actor) || Actor == GetOwner() || AIData == nullptr)
    {
        return;
    }
    FKataAIVisibleActor* Record = VisibleActors.FindByPredicate([Actor](const FKataAIVisibleActor& Entry)
    {
        return Entry.Actor.Get() == Actor;
    });
    if (bVisible)
    {
        if (Record == nullptr)
        {
            Record = &VisibleActors.AddDefaulted_GetRef();
            Record->Actor = Actor;
        }
        Record->Location = ObservedLocation;
        Record->ObservedTime = GetWorld()->GetTimeSeconds();
        if (CurrentTarget.Get() == Actor)
        {
            RememberTarget(Actor);
        }
    }
    else
    {
        // 실패 이벤트는 실제 위치를 노출할 수 있어 마지막 성공 위치만 보존한다.
        VisibleActors.RemoveAll([Actor](const FKataAIVisibleActor& Entry) { return Entry.Actor.Get() == Actor; });
    }
    SelectCurrentTarget();
}

void UKataAITargetingComponent::RefreshPerception()
{
    const UAIPerceptionComponent* Listener = Perception.Get();
    if (Listener == nullptr)
    {
        ResetKataAI();
        return;
    }
    TArray<AActor*> PerceivedActors;
    Listener->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
    VisibleActors.RemoveAll([&PerceivedActors](const FKataAIVisibleActor& Entry)
    {
        return !Entry.Actor.IsValid() || !PerceivedActors.Contains(Entry.Actor.Get());
    });
    for (AActor* Actor : PerceivedActors)
    {
        if (!IsValid(Actor) || Actor == GetOwner())
        {
            continue;
        }
        FKataAIVisibleActor* Record = VisibleActors.FindByPredicate([Actor](const FKataAIVisibleActor& Entry)
        {
            return Entry.Actor.Get() == Actor;
        });
        if (Record == nullptr)
        {
            Record = &VisibleActors.AddDefaulted_GetRef();
            Record->Actor = Actor;
        }
        // 현재 보이는 대상만 실제 좌표를 읽고 시야 상실 후의 기억 위치는 갱신하지 않는다.
        Record->Location = Actor->GetActorLocation();
        Record->ObservedTime = GetWorld()->GetTimeSeconds();
    }
    if (!LastSeenTarget.IsValid())
    {
        LastKnownLocation = FVector::ZeroVector;
        LastSeenTime = 0.0f;
    }
    SelectCurrentTarget();
}

void UKataAITargetingComponent::GetVisibleActors(TArray<AActor*>& OutActors) const
{
    OutActors.Reset();
    for (const FKataAIVisibleActor& Record : VisibleActors)
    {
        if (AActor* Actor = Record.Actor.Get())
        {
            OutActors.Add(Actor);
        }
    }
}

bool UKataAITargetingComponent::IsVisibleHostile(AActor* Actor) const
{
    return IsValid(Actor) && UKataFL_Faction::IsHostile(GetOwner(), Actor)
        && VisibleActors.ContainsByPredicate([Actor](const FKataAIVisibleActor& Entry) { return Entry.Actor.Get() == Actor; });
}

void UKataAITargetingComponent::SelectCurrentTarget()
{
    TArray<AActor*> Candidates;
    if (AIData != nullptr)
    {
        FindTargets(AIData->TargetingPreset, Candidates);
    }
    Candidates.RemoveAll([this](AActor* Actor) { return !IsVisibleHostile(Actor); });
    AActor* Selected = CurrentTarget.Get();
    // 거리 순위만 바뀌면 대상을 유지하지만 Preset에서 제외된 대상은 교체한다.
    if (Selected == nullptr || !Candidates.Contains(Selected))
    {
        Selected = Candidates.IsEmpty() ? nullptr : Candidates[0];
    }
    const bool bPreviouslySelected = bHadSelectedTarget;
    const bool bChanged = CurrentTarget.Get() != Selected || bHadSelectedTarget != (Selected != nullptr);
    CurrentTarget = Selected;
    bHadSelectedTarget = Selected != nullptr;
    if (Selected != nullptr)
    {
        RememberTarget(Selected);
    }
    if (!bPreviouslySelected && Selected != nullptr)
    {
        // 새 교전은 이전 이동 실패 이력과 무관하므로 재시도 횟수를 다시 센다.
        MoveRetryCount = 0;
    }
    if (bChanged)
    {
        OnTargetChanged.Broadcast(bPreviouslySelected, Selected);
    }
}

void UKataAITargetingComponent::RememberTarget(AActor* Target)
{
    if (const FKataAIVisibleActor* Record = VisibleActors.FindByPredicate([Target](const FKataAIVisibleActor& Entry)
        { return Entry.Actor.Get() == Target; }))
    {
        LastSeenTarget = Target;
        LastKnownLocation = Record->Location;
        LastSeenTime = Record->ObservedTime;
    }
}

AActor* UKataAITargetingComponent::GetCurrentTarget_Implementation() const
{
    AActor* Target = CurrentTarget.Get();
    return IsVisibleHostile(Target) ? Target : nullptr;
}

AActor* UKataAITargetingComponent::ResolveActionTarget_Implementation()
{
    SelectCurrentTarget();
    return GetCurrentTarget();
}

bool UKataAITargetingComponent::CanKeepActionTarget_Implementation(AActor* ActionTarget) const
{
    return ActionTarget != nullptr && ActionTarget == GetCurrentTarget();
}

void UKataAITargetingComponent::ClearTargetMemory()
{
    if (GetCurrentTarget() == nullptr)
    {
        LastSeenTarget.Reset();
        LastKnownLocation = FVector::ZeroVector;
        LastSeenTime = 0.0f;
        MoveRetryCount = 0;
    }
}

void UKataAITargetingComponent::SetHomeLocation(const FVector& NewHomeLocation)
{
    HomeLocation = NewHomeLocation;
}

int32 UKataAITargetingComponent::IncrementMoveRetryCount()
{
    MoveRetryCount = MoveRetryCount < MAX_int32 ? MoveRetryCount + 1 : MoveRetryCount;
    return MoveRetryCount;
}

void UKataAITargetingComponent::ResetMoveRetryCount()
{
    MoveRetryCount = 0;
}

bool UKataAITargetingComponent::HasLastKnownLocation() const
{
    return LastSeenTarget.IsValid();
}

float UKataAITargetingComponent::GetTimeSinceLastSeen() const
{
    return HasLastKnownLocation() && GetWorld() != nullptr
        ? FMath::Max(0.0f, GetWorld()->GetTimeSeconds() - LastSeenTime) : 0.0f;
}

void UKataAITargetingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ResetKataAI();
    Super::EndPlay(EndPlayReason);
}
