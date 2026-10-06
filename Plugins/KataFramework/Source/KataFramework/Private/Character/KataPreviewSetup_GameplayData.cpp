#include "Character/KataPreviewSetup_GameplayData.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/KataGameplayData.h"
#include "GameFramework/Actor.h"
#include "KataFrameworkLog.h"

void UKataPreviewSetup_GameplayData::ApplyToPreview(AActor* PreviewActor) const
{
    if (PreviewActor == nullptr || (GameplayData.IsEmpty() && IdentityTags.IsEmpty()))
    {
        return;
    }

    UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PreviewActor);
    if (AbilitySystem == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Kata gameplay data preview setup found no ability system on '%s'"), *GetNameSafe(PreviewActor));
        return;
    }

    // 프리뷰 액터는 프리뷰를 다시 만들 때 함께 사라지므로 적용 결과를 보관하지 않는다.
    UKataGameplayData::ApplyAll(AbilitySystem, GameplayData, IdentityTags, Level);
}
