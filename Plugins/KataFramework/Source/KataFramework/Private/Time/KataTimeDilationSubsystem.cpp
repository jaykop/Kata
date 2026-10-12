#include "Time/KataTimeDilationSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WorldSettings.h"
#include "KataFrameworkLog.h"

namespace
{
    bool IsValidTimeDilation(float TimeDilation)
    {
        return FMath::IsFinite(TimeDilation) && TimeDilation >= 0.0f;
    }
}

int32 UKataTimeDilationSubsystem::RequestActorTimeDilation(AActor* TargetActor, float TimeDilation, int32 Priority, float Duration)
{
    if (!IsValid(TargetActor) || TargetActor->GetWorld() != GetWorld())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("RequestActorTimeDilation ignored: target actor is missing or belongs to another world"));
        return INDEX_NONE;
    }
    if (!IsValidTimeDilation(TimeDilation))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("RequestActorTimeDilation ignored: invalid time dilation %f for %s"), TimeDilation, *GetNameSafe(TargetActor));
        return INDEX_NONE;
    }

    FRequest Request;
    Request.TargetActor = TargetActor;
    Request.TimeDilation = TimeDilation;
    Request.Priority = Priority;
    Request.ExpireTime = Duration > 0.0f ? ElapsedTime + Duration : -1.0;

    // 원래 값은 첫 요청이 엔진 값을 바꾸기 전에 기억한다.
    if (!OriginalActorTimeDilations.Contains(TargetActor))
    {
        OriginalActorTimeDilations.Add(TargetActor, TargetActor->CustomTimeDilation);
    }

    const int32 Handle = AddRequest(MoveTemp(Request));
    ApplyActorLayer(TargetActor);
    return Handle;
}

int32 UKataTimeDilationSubsystem::RequestWorldTimeDilation(float TimeDilation, int32 Priority, float Duration)
{
    AWorldSettings* WorldSettings = GetWorld() != nullptr ? GetWorld()->GetWorldSettings(false, false) : nullptr;
    if (WorldSettings == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("RequestWorldTimeDilation ignored: world settings are not available"));
        return INDEX_NONE;
    }
    if (!IsValidTimeDilation(TimeDilation))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("RequestWorldTimeDilation ignored: invalid time dilation %f"), TimeDilation);
        return INDEX_NONE;
    }

    FRequest Request;
    Request.bWorldLayer = true;
    Request.TimeDilation = TimeDilation;
    Request.Priority = Priority;
    Request.ExpireTime = Duration > 0.0f ? ElapsedTime + Duration : -1.0;

    if (!OriginalWorldTimeDilation.IsSet())
    {
        OriginalWorldTimeDilation = WorldSettings->TimeDilation;
    }

    const int32 Handle = AddRequest(MoveTemp(Request));
    ApplyWorldLayer();
    return Handle;
}

void UKataTimeDilationSubsystem::ReleaseRequest(int32 Handle)
{
    if (Handle == INDEX_NONE)
    {
        return;
    }
    const int32 Index = Requests.IndexOfByPredicate([Handle](const FRequest& Request)
    {
        return Request.Handle == Handle;
    });
    if (Index == INDEX_NONE)
    {
        return;
    }

    const bool bWorldLayer = Requests[Index].bWorldLayer;
    AActor* TargetActor = Requests[Index].TargetActor.Get();
    Requests.RemoveAt(Index);

    if (bWorldLayer)
    {
        ApplyWorldLayer();
    }
    else if (TargetActor != nullptr)
    {
        ApplyActorLayer(TargetActor);
    }
}

void UKataTimeDilationSubsystem::Deinitialize()
{
    // 월드가 남아 있는 채로 Subsystem만 정리되는 경우에도 바꿔 둔 엔진 값을 남기지 않는다.
    Requests.Reset();
    for (const TPair<TWeakObjectPtr<AActor>, float>& Pair : OriginalActorTimeDilations)
    {
        if (AActor* Actor = Pair.Key.Get())
        {
            Actor->CustomTimeDilation = Pair.Value;
        }
    }
    OriginalActorTimeDilations.Reset();
    ApplyWorldLayer();

    Super::Deinitialize();
}

void UKataTimeDilationSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Tick이 받는 시간에는 월드 배율이 곱해져 있다. 만료는 배율과 무관해야 하므로 되돌린다.
    const AWorldSettings* WorldSettings = GetWorld() != nullptr ? GetWorld()->GetWorldSettings(false, false) : nullptr;
    const float WorldTimeDilation = WorldSettings != nullptr ? WorldSettings->GetEffectiveTimeDilation() : 1.0f;
    if (WorldTimeDilation > UE_SMALL_NUMBER)
    {
        ElapsedTime += DeltaTime / WorldTimeDilation;
    }

    bool bWorldLayerChanged = false;
    TArray<TWeakObjectPtr<AActor>, TInlineAllocator<4>> ChangedActors;
    for (int32 Index = Requests.Num() - 1; Index >= 0; --Index)
    {
        const FRequest& Request = Requests[Index];
        const bool bTargetGone = !Request.bWorldLayer && !Request.TargetActor.IsValid();
        const bool bExpired = Request.ExpireTime >= 0.0 && ElapsedTime >= Request.ExpireTime;
        if (!bTargetGone && !bExpired)
        {
            continue;
        }
        if (Request.bWorldLayer)
        {
            bWorldLayerChanged = true;
        }
        else if (!bTargetGone)
        {
            ChangedActors.AddUnique(Request.TargetActor);
        }
        Requests.RemoveAt(Index);
    }

    for (const TWeakObjectPtr<AActor>& WeakActor : ChangedActors)
    {
        if (AActor* Actor = WeakActor.Get())
        {
            ApplyActorLayer(Actor);
        }
    }
    if (bWorldLayerChanged)
    {
        ApplyWorldLayer();
    }

    // 파괴된 액터의 원래 값은 되돌릴 곳이 없으므로 버린다.
    for (auto It = OriginalActorTimeDilations.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }
}

bool UKataTimeDilationSubsystem::IsTickable() const
{
    // 만료와 파괴된 대상 정리만 Tick에서 한다. 요청이 없으면 할 일이 없다.
    return !Requests.IsEmpty() || !OriginalActorTimeDilations.IsEmpty();
}

TStatId UKataTimeDilationSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UKataTimeDilationSubsystem, STATGROUP_Tickables);
}

bool UKataTimeDilationSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

int32 UKataTimeDilationSubsystem::AddRequest(FRequest&& Request)
{
    Request.Handle = NextHandle++;
    Request.Sequence = NextSequence++;
    const int32 Handle = Request.Handle;
    Requests.Add(MoveTemp(Request));
    return Handle;
}

const UKataTimeDilationSubsystem::FRequest* UKataTimeDilationSubsystem::FindWinningRequest(bool bWorldLayer, const AActor* Actor) const
{
    const FRequest* Winner = nullptr;
    for (const FRequest& Request : Requests)
    {
        if (Request.bWorldLayer != bWorldLayer || (!bWorldLayer && Request.TargetActor.Get() != Actor))
        {
            continue;
        }
        if (Winner == nullptr || Request.Priority > Winner->Priority
            || (Request.Priority == Winner->Priority && Request.Sequence > Winner->Sequence))
        {
            Winner = &Request;
        }
    }
    return Winner;
}

void UKataTimeDilationSubsystem::ApplyActorLayer(AActor* Actor)
{
    if (Actor == nullptr)
    {
        return;
    }
    if (const FRequest* Winner = FindWinningRequest(false, Actor))
    {
        Actor->CustomTimeDilation = Winner->TimeDilation;
        return;
    }
    float OriginalTimeDilation = 1.0f;
    if (OriginalActorTimeDilations.RemoveAndCopyValue(Actor, OriginalTimeDilation))
    {
        Actor->CustomTimeDilation = OriginalTimeDilation;
    }
}

void UKataTimeDilationSubsystem::ApplyWorldLayer()
{
    AWorldSettings* WorldSettings = GetWorld() != nullptr ? GetWorld()->GetWorldSettings(false, false) : nullptr;
    if (WorldSettings == nullptr)
    {
        return;
    }
    if (const FRequest* Winner = FindWinningRequest(true, nullptr))
    {
        WorldSettings->SetTimeDilation(Winner->TimeDilation);
        return;
    }
    if (OriginalWorldTimeDilation.IsSet())
    {
        WorldSettings->SetTimeDilation(OriginalWorldTimeDilation.GetValue());
        OriginalWorldTimeDilation.Reset();
    }
}
