#include "Targeting/Tasks/KataTargetingFilterTask_OwnedTags.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"

bool UKataTargetingFilterTask_OwnedTags::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
    const AActor* Target = TargetData.HitResult.GetActor();
    if (Target == nullptr || ExcludedTags.IsEmpty())
    {
        return false;
    }

    const UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
    return AbilitySystem != nullptr && AbilitySystem->HasAnyMatchingGameplayTags(ExcludedTags);
}
