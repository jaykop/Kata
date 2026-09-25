#include "Targeting/Tasks/KataTargetingViewUtils.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace KataTargetingView
{
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
}
