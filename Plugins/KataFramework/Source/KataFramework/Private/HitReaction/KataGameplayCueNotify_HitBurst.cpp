#include "HitReaction/KataGameplayCueNotify_HitBurst.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UKataGameplayCueNotify_HitBurst::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const
{
    // 기본 효과와 Blueprint OnBurst는 엔진 Burst가 처리한다.
    Super::OnExecute_Implementation(Target, Parameters);

    const FKataHitBurstTagEffects* Match = TagBurstEffects.FindByPredicate([&Parameters](const FKataHitBurstTagEffects& Entry)
    {
        return Entry.Tag.IsValid() && Parameters.AggregatedTargetTags.HasTag(Entry.Tag);
    });
    if (Match == nullptr)
    {
        return false;
    }

    // 엔진 Burst와 같은 Spawn Context를 만들어, 태그별 효과도 기본 Spawn Condition·Placement를 따르게 한다.
    UWorld* World = Target != nullptr ? Target->GetWorld() : GetWorld();
    FGameplayCueNotify_SpawnContext SpawnContext(World, Target, Parameters);
    SpawnContext.SetDefaultSpawnCondition(&DefaultSpawnCondition);
    SpawnContext.SetDefaultPlacementInfo(&DefaultPlacementInfo);
    if (DefaultSpawnCondition.ShouldSpawn(SpawnContext))
    {
        FGameplayCueNotify_SpawnResult SpawnResult;
        Match->Effects.ExecuteEffects(SpawnContext, SpawnResult);
    }
    return false;
}

#if WITH_EDITOR
EDataValidationResult UKataGameplayCueNotify_HitBurst::IsDataValid(FDataValidationContext& Context) const
{
    Super::IsDataValid(Context);

    for (int32 Index = 0; Index < TagBurstEffects.Num(); ++Index)
    {
        const FKataHitBurstTagEffects& Entry = TagBurstEffects[Index];
        if (!Entry.Tag.IsValid())
        {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT("TagBurstEffects[%d] has no Tag and never plays."), Index)));
        }
        Entry.Effects.ValidateAssociatedAssets(this, FString::Printf(TEXT("TagBurstEffects[%d]"), Index), Context);
    }
    return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
