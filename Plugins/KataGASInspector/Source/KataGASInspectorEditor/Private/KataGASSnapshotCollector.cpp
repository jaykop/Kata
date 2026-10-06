#include "KataGASSnapshotCollector.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "GameplayTagsManager.h"
#include "KataGASAbilityMetadata.h"

namespace KataGASInspection
{
    TSharedPtr<FKataGASInspectionRow> MakeRow(EKataGASInspectionPage Page, const FString& Key, const FString& Name)
    {
        TSharedPtr<FKataGASInspectionRow> Row = MakeShared<FKataGASInspectionRow>();
        Row->Page = Page;
        Row->Key = Key;
        Row->Name = Name;
        return Row;
    }

    FString ObjectLabel(const UObject* Object)
    {
        if (const AActor* Actor = Cast<AActor>(Object))
        {
            return Actor->GetActorNameOrLabel();
        }
        return GetNameSafe(Object);
    }

    void AddTagRows(UAbilitySystemComponent& ASC, const FGameplayTagContainer& Tags, bool bOwned,
        TArray<TSharedPtr<FKataGASInspectionRow>>& Rows)
    {
        for (const FGameplayTag& Tag : Tags)
        {
            const FString TagName = Tag.ToString();
            TSharedPtr<FKataGASInspectionRow> Row = MakeRow(EKataGASInspectionPage::Tags,
                (bOwned ? TEXT("owned/") : TEXT("blocked/")) + TagName, TagName);
            Row->State = bOwned ? TEXT("Owned") : TEXT("Ability blocked");
            Row->Value = bOwned ? FString::FromInt(ASC.GetTagCount(Tag)) : TEXT("—");
            Row->Tags = TagName;
            FString Comment;
            FName Source;
            bool bExplicit = false;
            bool bRestricted = false;
            bool bAllowsChildren = false;
            if (UGameplayTagsManager::Get().GetTagEditorData(Tag.GetTagName(), Comment, Source,
                bExplicit, bRestricted, bAllowsChildren))
            {
                Row->Source = bExplicit ? Source.ToString() : TEXT("Implicit parent");
                Row->Detail = Comment;
            }
            Row->Detail += bOwned ? TEXT("\nOwned count includes hierarchical matches.")
                : TEXT("\nThis is an ability-blocking tag, not an owned tag count.");
            Rows.Add(Row);
        }
    }
}

FKataGASInspectionSnapshot FKataGASSnapshotCollector::Collect(UAbilitySystemComponent& ASC, const FString& WorldLabel)
{
    using namespace KataGASInspection;
    check(IsInGameThread());
    FKataGASInspectionSnapshot Snapshot;
    Snapshot.CapturedAt = FDateTime::Now();
    Snapshot.Target = FString::Printf(TEXT("%s | %s | Owner: %s | Avatar: %s"),
        *ObjectLabel(ASC.GetOwner()), *ASC.GetName(), *ObjectLabel(ASC.GetOwnerActor()),
        *ObjectLabel(ASC.GetAvatarActor_Direct()));
    Snapshot.World = WorldLabel;
    Snapshot.TargetPath = ASC.GetPathName();
    const bool bReady = ASC.AbilityActorInfo.IsValid()
        && ASC.AbilityActorInfo->OwnerActor.IsValid()
        && ASC.AbilityActorInfo->AbilitySystemComponent.Get() == &ASC;
    Snapshot.bActorInfoReady = bReady;
    Snapshot.Readiness = bReady ? TEXT("ActorInfo ready; activation eligibility is not evaluated.")
        : TEXT("ActorInfo not initialized; showing available component data.");

    FGameplayTagContainer OwnedTags;
    ASC.GetOwnedGameplayTags(OwnedTags);
    AddTagRows(ASC, OwnedTags, true, Snapshot.Rows);
    FGameplayTagContainer BlockedTags;
    ASC.GetBlockedAbilityTags(BlockedTags);
    AddTagRows(ASC, BlockedTags, false, Snapshot.Rows);

    TArray<FGameplayAttribute> Attributes;
    ASC.GetAllAttributes(Attributes);
    TSet<FString> AttributeKeys;
    for (const FGameplayAttribute& Attribute : Attributes)
    {
        if (!Attribute.IsValid())
        {
            continue;
        }
        const FString Key = Attribute.GetAttributeSetClass()->GetPathName() + TEXT(".") + Attribute.GetName();
        if (AttributeKeys.Contains(Key))
        {
            continue;
        }
        AttributeKeys.Add(Key);
        TSharedPtr<FKataGASInspectionRow> Row = MakeRow(EKataGASInspectionPage::Attributes, Key, Attribute.GetName());
        bool bFound = false;
        const float Current = ASC.GetGameplayAttributeValue(Attribute, bFound);
        Row->State = bFound ? TEXT("Available") : TEXT("Unavailable");
        Row->Value = bFound ? FString::Printf(TEXT("%g"), Current)
            : TEXT("Attribute set unavailable");
        Row->Source = Attribute.GetAttributeSetClass()->GetPathName();
        Row->Detail = Key;
        Snapshot.Rows.Add(Row);
    }

    // spec 참조는 수집 중에만 사용한다. 확장 getter에서 목록이 변경되어도 순회가 유지되도록 잠근다.
    FScopedAbilityListLock AbilityLock(ASC);
    for (const FGameplayAbilitySpec& Spec : ASC.GetActivatableAbilities())
    {
        if (!IsValid(Spec.Ability))
        {
            continue;
        }
        const UGameplayAbility* Ability = Spec.Ability;
        const FString Key = TEXT("ability/") + Spec.Handle.ToString();
        TSharedPtr<FKataGASInspectionRow> Row = MakeRow(EKataGASInspectionPage::Abilities, Key, Ability->GetClass()->GetName());
        Row->bActive = Spec.IsActive();
        const bool bInputBlocked = ASC.IsAbilityInputBlocked(Spec.InputID);
        const bool bTagsBlocked = ASC.AreAbilityTagsBlocked(Ability->GetAssetTags());
        Row->bBlocked = bInputBlocked || bTagsBlocked;
        Row->State = Row->bActive ? TEXT("Active") : TEXT("Idle");
        if (bInputBlocked)
        {
            Row->State += TEXT(" | Input blocked");
        }
        if (bTagsBlocked)
        {
            Row->State += TEXT(" | Tags blocked");
        }
        Row->Value = FString::Printf(TEXT("Level: %d   Active: %d   Input: %d"), Spec.Level, Spec.ActiveCount, Spec.InputID);
        Row->Tags = Ability->GetAssetTags().ToStringSimple();
        Row->Source = Ability->GetClass()->GetPathName();
        Row->AssetPath = FKataGASAbilityMetadata::GetSourceAsset(Ability->GetClass());
        Row->Detail = TEXT("Activation eligibility: not evaluated. Idle does not imply activatable.");
        if (bReady)
        {
            Row->Detail += FString::Printf(TEXT("\nCooldown remaining: %g s"),
                Ability->GetCooldownTimeRemaining(ASC.AbilityActorInfo.Get()));
        }

        TArray<FAbilityTriggerData> Triggers;
        if (FKataGASAbilityMetadata::ReadTriggers(Ability, Triggers))
        {
            for (int32 Index = 0; Index < Triggers.Num(); ++Index)
            {
                const FAbilityTriggerData& Trigger = Triggers[Index];
                Row->Detail += FString::Printf(TEXT("\nTrigger: %s | %s"), *Trigger.TriggerTag.ToString(),
                    *UEnum::GetValueAsString(Trigger.TriggerSource));
            }
        }
        else
        {
            Row->Detail += TEXT("\nTrigger metadata: unsupported schema.");
        }

        for (int32 Index = 0; Index < Spec.DynamicAbilityTriggers.Num(); ++Index)
        {
            const FAbilityTriggerData& Trigger = Spec.DynamicAbilityTriggers[Index];
            Row->Detail += FString::Printf(TEXT("\nDynamic trigger: %s | %s"), *Trigger.TriggerTag.ToString(),
                *UEnum::GetValueAsString(Trigger.TriggerSource));
        }

        for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
        {
            if (!IsValid(Instance) || !Instance->IsActive())
            {
                continue;
            }
            TSharedPtr<FKataGASInspectionRow> Child = MakeRow(EKataGASInspectionPage::Abilities,
                Key + TEXT("/instance/") + FString::FromInt(Instance->GetUniqueID()), Instance->GetName());
            Child->State = TEXT("Active instance");
            Child->Source = Instance->GetClass()->GetPathName();
            if (!FKataGASAbilityMetadata::ReadTasks(Instance, Child->Key, Child->Children))
            {
                Child->Detail = TEXT("Tasks: unsupported schema.");
            }
            Row->Children.Add(Child);
        }
        // 기존 NonInstanced 에셋도 진단해야 하므로 해당 비교에서만 폐기 예정 경고를 억제한다.
        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        const bool bNonInstanced = Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::NonInstanced;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
        if (bNonInstanced)
        {
            Row->Detail += TEXT("\nNon-instanced ability: no per-execution instance/task list.");
        }
        Snapshot.Rows.Add(Row);
    }

    UWorld* World = ASC.GetWorld();
    for (const FActiveGameplayEffectHandle& Handle : ASC.GetActiveEffects(FGameplayEffectQuery()))
    {
        const FActiveGameplayEffect* Effect = ASC.GetActiveGameplayEffect(Handle);
        if (!Effect || !IsValid(Effect->Spec.Def))
        {
            continue;
        }
        const FGameplayEffectSpec& Spec = Effect->Spec;
        TSharedPtr<FKataGASInspectionRow> Row = MakeRow(EKataGASInspectionPage::Effects,
            TEXT("effect/") + Handle.ToString(), Spec.Def->GetClass()->GetName());
        Row->State = Effect->bIsInhibited ? TEXT("Inhibited") : TEXT("Applied");
        Row->Value = FString::Printf(TEXT("Level: %g   Stacks: %d   Period: %g s"), Spec.GetLevel(), Spec.GetStackCount(), Effect->GetPeriod());
        if (Spec.Def->DurationPolicy == EGameplayEffectDurationType::Infinite)
        {
            Row->Value += TEXT("   Infinite");
        }
        else if (World)
        {
            Row->Value += FString::Printf(TEXT("   Remaining: %g / %g s"),
                Effect->GetTimeRemaining(World->GetTimeSeconds()), Effect->GetDuration());
        }
        FGameplayTagContainer GrantedTags;
        Spec.GetAllGrantedTags(GrantedTags);
        Row->Tags = GrantedTags.ToStringSimple();
        Row->Source = ObjectLabel(Spec.GetContext().GetInstigator());
        Row->AssetPath = FKataGASAbilityMetadata::GetSourceAsset(Spec.Def->GetClass());
        FGameplayTagContainer AssetTags;
        Spec.GetAllAssetTags(AssetTags);
        Row->Detail = FString::Printf(TEXT("Definition: %s\nSource object: %s\nEffect causer: %s\nAsset tags: %s\nModifier magnitudes are not final attribute contributions."),
            *Spec.Def->GetClass()->GetPathName(), *ObjectLabel(Spec.GetContext().GetSourceObject()),
            *ObjectLabel(Spec.GetContext().GetEffectCauser()), *AssetTags.ToStringSimple());
        const int32 ModifierCount = FMath::Min(Spec.Modifiers.Num(), Spec.Def->Modifiers.Num());
        for (int32 Index = 0; Index < ModifierCount; ++Index)
        {
            const FGameplayModifierInfo& Modifier = Spec.Def->Modifiers[Index];
            TSharedPtr<FKataGASInspectionRow> Child = MakeRow(EKataGASInspectionPage::Effects,
                Row->Key + TEXT("/modifier/") + FString::FromInt(Index), Modifier.Attribute.GetName());
            Child->State = UEnum::GetValueAsString(Modifier.ModifierOp);
            Child->Value = FString::Printf(TEXT("Evaluated magnitude: %g"), Spec.GetModifierMagnitude(Index));
            Child->Source = Modifier.Attribute.IsValid() ? Modifier.Attribute.GetAttributeSetClass()->GetPathName() : TEXT("Invalid attribute");
            Row->Children.Add(Child);
        }
        if (Spec.Modifiers.Num() != Spec.Def->Modifiers.Num())
        {
            Row->Detail += TEXT("\nModifier schema changed; only matching indices are displayed.");
        }
        Snapshot.Rows.Add(Row);
    }
    return Snapshot;
}
