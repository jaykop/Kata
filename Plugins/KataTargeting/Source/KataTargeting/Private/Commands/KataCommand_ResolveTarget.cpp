#include "Commands/KataCommand_ResolveTarget.h"

#include "GameFramework/Actor.h"
#include "Runtime/KataActionInstance.h"
#include "Targeting/KataTargetingComponent.h"

void UKataCommand_ResolveTarget::Execute_Implementation(UKataActionInstance* Instance)
{
    if (Instance == nullptr)
    {
        return;
    }

    const FKataContext& Context = Instance->GetContextRef();
    AActor* Actor = Context.GetAvatarActor();
    UKataTargetingComponent* Targeting = Actor != nullptr ? Actor->FindComponentByClass<UKataTargetingComponent>() : nullptr;
    if (Targeting == nullptr)
    {
        return;
    }

    AActor* CurrentTarget = Context.GetTargetActor();
    if (bKeepValidTarget && IsValid(CurrentTarget) && CurrentTarget != Actor && Targeting->CanKeepActionTarget(CurrentTarget))
    {
        return;
    }

    Instance->SetTargetActor(Targeting->ResolveActionTarget());
}
