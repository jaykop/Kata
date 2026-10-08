#include "Targeting/Tasks/KataTargetingViewUtils.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Targeting/KataTargetPointComponent.h"

namespace KataTargetingView
{
    UKataTargetPointComponent* GetTargetPoint(const FTargetingDefaultResultData& TargetData)
    {
        return Cast<UKataTargetPointComponent>(TargetData.HitResult.Component.Get());
    }

    bool GetTargetLocation(const FTargetingDefaultResultData& TargetData, FVector& OutLocation)
    {
        if (const UKataTargetPointComponent* Point = GetTargetPoint(TargetData))
        {
            OutLocation = Point->GetComponentLocation();
            return true;
        }
        if (const AActor* Actor = TargetData.HitResult.GetActor())
        {
            OutLocation = Actor->GetActorLocation();
            return true;
        }
        return false;
    }

    AActor* GetSourceActor(const FTargetingRequestHandle& TargetingHandle)
    {
        const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
        return SourceContext != nullptr ? SourceContext->SourceActor.Get() : nullptr;
    }

    bool GetViewPoint(const AActor* SourceActor, FVector& OutLocation, FRotator& OutRotation)
    {
        if (SourceActor == nullptr)
        {
            return false;
        }

        // 화면 기준 판정은 캐릭터의 시선이 아니라 플레이어가 보는 카메라를 따라야 한다.
        if (const APawn* Pawn = Cast<APawn>(SourceActor))
        {
            if (const APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
            {
                PlayerController->GetPlayerViewPoint(OutLocation, OutRotation);
                return true;
            }
        }

        SourceActor->GetActorEyesViewPoint(OutLocation, OutRotation);
        return true;
    }

    namespace
    {
        void AddIgnoredActorTree(FCollisionQueryParams& Params, const AActor* Actor)
        {
            if (Actor == nullptr)
            {
                return;
            }
            Params.AddIgnoredActor(Actor);
            TArray<AActor*> AttachedActors;
            Actor->GetAttachedActors(AttachedActors, true, true);
            Params.AddIgnoredActors(AttachedActors);
        }
    }

    bool HasLineOfSight(const AActor* SourceActor, const AActor* TargetActor, const FVector& ViewLocation,
        const FVector& TargetLocation, ECollisionChannel TraceChannel)
    {
        const UWorld* World = SourceActor != nullptr ? SourceActor->GetWorld() : nullptr;
        if (World == nullptr)
        {
            return true;
        }

        // 3인칭 카메라는 캐릭터 뒤에 있으므로 실행 주체를 무시해야 자기 몸에 가리지 않는다.
        // 지점은 대상 몸 안에 있는 경우가 많아 대상 액터도 무시한다.
        FCollisionQueryParams Params(SCENE_QUERY_STAT(KataTargetingLineOfSight), false);
        AddIgnoredActorTree(Params, SourceActor);
        AddIgnoredActorTree(Params, TargetActor);
        return !World->LineTraceTestByChannel(ViewLocation, TargetLocation, TraceChannel, Params);
    }
}
