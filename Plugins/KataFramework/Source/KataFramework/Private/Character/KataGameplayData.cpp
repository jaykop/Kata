#include "Character/KataGameplayData.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "KataFrameworkLog.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "KataGameplayData"

FKataGameplayDataHandles UKataGameplayData::ApplyAll(UAbilitySystemComponent* AbilitySystem, TConstArrayView<TObjectPtr<UKataGameplayData>> DataList,
    const FGameplayTagContainer& IdentityTags, float Level)
{
    FKataGameplayDataHandles Handles;
    if (AbilitySystem == nullptr)
    {
        return Handles;
    }

    // 다른 에셋이 추가하는 세트의 초기값도 넣을 수 있도록 모든 세트를 먼저 추가한다.
    for (const UKataGameplayData* Data : DataList)
    {
        if (Data == nullptr)
        {
            continue;
        }
        for (const TSubclassOf<UAttributeSet>& SetClass : Data->AttributeSets)
        {
            if (SetClass == nullptr || AbilitySystem->GetAttributeSet(SetClass) != nullptr)
            {
                continue;
            }
            // ASC가 SpawnedAttributes로 참조하므로 소유 액터를 Outer로 만들어 액터와 수명을 같이한다.
            UAttributeSet* NewSet = NewObject<UAttributeSet>(AbilitySystem->GetOwner(), SetClass);
            AbilitySystem->AddSpawnedAttribute(NewSet);
            Handles.AddedAttributeSets.Add(NewSet);
        }
    }

    // 에셋 사이의 중복은 뒤쪽 값으로 덮어쓴다.
    TArray<TPair<FGameplayAttribute, float>> MergedValues;
    for (const UKataGameplayData* Data : DataList)
    {
        if (Data == nullptr)
        {
            continue;
        }
        const FString ContextString = GetNameSafe(Data);
        for (const FKataAttributeInitValue& Entry : Data->InitialValues)
        {
            if (!Entry.Attribute.IsValid())
            {
                continue;
            }
            const float Value = Entry.Value.GetValueAtLevel(Level, &ContextString);
            TPair<FGameplayAttribute, float>* Existing = MergedValues.FindByPredicate(
                [&Entry](const TPair<FGameplayAttribute, float>& Pair) { return Pair.Key == Entry.Attribute; });
            if (Existing != nullptr)
            {
                UE_LOG(LogKataFramework, Warning, TEXT("Kata gameplay data '%s' overrides the initial value of '%s' set by an earlier entry"),
                    *ContextString, *Entry.Attribute.GetName());
                Existing->Value = Value;
            }
            else
            {
                MergedValues.Emplace(Entry.Attribute, Value);
            }
        }
    }

    // 현재값은 최대값으로 제한되므로 순서에 따라 첫 설정에서 잘릴 수 있다.
    // 어떤 Attribute가 최대값인지 일반적으로 알 수 없어서, 모든 값을 두 번 넣어 두 번째에 최대값이 정해진 상태로 현재값을 다시 넣는다.
    for (int32 Pass = 0; Pass < 2; ++Pass)
    {
        for (const TPair<FGameplayAttribute, float>& Pair : MergedValues)
        {
            if (!AbilitySystem->HasAttributeSetForAttribute(Pair.Key))
            {
                if (Pass == 0)
                {
                    UE_LOG(LogKataFramework, Warning, TEXT("Kata gameplay data skipped '%s' because '%s' has no attribute set for it"),
                        *Pair.Key.GetName(), *GetNameSafe(AbilitySystem->GetOwner()));
                }
                continue;
            }
            AbilitySystem->SetNumericAttributeBase(Pair.Key, Pair.Value);
        }
    }

    for (const UKataGameplayData* Data : DataList)
    {
        if (Data == nullptr)
        {
            continue;
        }
        for (const FKataGrantedEffect& Entry : Data->GrantedEffects)
        {
            if (Entry.EffectClass == nullptr)
            {
                continue;
            }
            FGameplayEffectContextHandle EffectContext = AbilitySystem->MakeEffectContext();
            EffectContext.AddSourceObject(Data);
            const FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(Entry.EffectClass, Entry.Level, EffectContext);
            if (!Spec.IsValid())
            {
                UE_LOG(LogKataFramework, Warning, TEXT("Kata gameplay data '%s' failed to build a spec for '%s'"),
                    *GetNameSafe(Data), *GetNameSafe(Entry.EffectClass));
                continue;
            }
            const FActiveGameplayEffectHandle EffectHandle = AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
            if (EffectHandle.IsValid())
            {
                Handles.EffectHandles.Add(EffectHandle);
            }
        }
    }

    for (const UKataGameplayData* Data : DataList)
    {
        if (Data == nullptr)
        {
            continue;
        }
        for (const FKataGrantedAbility& Entry : Data->GrantedAbilities)
        {
            if (Entry.AbilityClass == nullptr)
            {
                continue;
            }
            // Source Object에 출처 에셋을 넣어 디버거에서 어느 Gameplay Data가 부여했는지 보이게 한다.
            const FGameplayAbilitySpecHandle AbilityHandle = AbilitySystem->GiveAbility(
                FGameplayAbilitySpec(Entry.AbilityClass, Entry.Level, INDEX_NONE, const_cast<UKataGameplayData*>(Data)));
            if (AbilityHandle.IsValid())
            {
                Handles.AbilityHandles.Add(AbilityHandle);
            }
        }
    }

    if (!IdentityTags.IsEmpty())
    {
        AbilitySystem->AddLooseGameplayTags(IdentityTags);
        Handles.GrantedTags.AppendTags(IdentityTags);
    }

    return Handles;
}

#if WITH_EDITOR
EDataValidationResult UKataGameplayData::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    TSet<UClass*> SeenSetClasses;
    for (int32 Index = 0; Index < AttributeSets.Num(); ++Index)
    {
        UClass* SetClass = AttributeSets[Index].Get();
        if (SetClass == nullptr)
        {
            Context.AddError(FText::Format(LOCTEXT("EmptySet", "Attribute Sets[{0}] is empty"), Index));
            Result = EDataValidationResult::Invalid;
        }
        else if (SeenSetClasses.Contains(SetClass))
        {
            Context.AddError(FText::Format(LOCTEXT("DuplicateSet", "Attribute Sets lists '{0}' more than once"), FText::FromString(SetClass->GetName())));
            Result = EDataValidationResult::Invalid;
        }
        else
        {
            SeenSetClasses.Add(SetClass);
        }
    }

    for (int32 Index = 0; Index < GrantedAbilities.Num(); ++Index)
    {
        if (GrantedAbilities[Index].AbilityClass == nullptr)
        {
            Context.AddError(FText::Format(LOCTEXT("EmptyAbility", "Granted Abilities[{0}] has no ability class"), Index));
            Result = EDataValidationResult::Invalid;
        }
    }
    for (int32 Index = 0; Index < GrantedEffects.Num(); ++Index)
    {
        if (GrantedEffects[Index].EffectClass == nullptr)
        {
            Context.AddError(FText::Format(LOCTEXT("EmptyEffect", "Granted Effects[{0}] has no effect class"), Index));
            Result = EDataValidationResult::Invalid;
        }
    }

    TArray<FGameplayAttribute> SeenAttributes;
    for (int32 Index = 0; Index < InitialValues.Num(); ++Index)
    {
        const FGameplayAttribute& Attribute = InitialValues[Index].Attribute;
        if (!Attribute.IsValid())
        {
            Context.AddError(FText::Format(LOCTEXT("EmptyAttribute", "Initial Values[{0}] has no attribute"), Index));
            Result = EDataValidationResult::Invalid;
            continue;
        }
        if (SeenAttributes.Contains(Attribute))
        {
            Context.AddError(FText::Format(LOCTEXT("DuplicateAttribute", "Initial Values lists '{0}' more than once"), FText::FromString(Attribute.GetName())));
            Result = EDataValidationResult::Invalid;
            continue;
        }
        SeenAttributes.Add(Attribute);

        // 세트를 다른 Gameplay Data가 추가하는 조합도 허용하므로 오류가 아니라 경고로 알린다.
        UClass* OwnerClass = Attribute.GetAttributeSetClass();
        bool bSetListedHere = false;
        for (const UClass* SetClass : SeenSetClasses)
        {
            if (OwnerClass != nullptr && SetClass->IsChildOf(OwnerClass))
            {
                bSetListedHere = true;
                break;
            }
        }
        if (!bSetListedHere)
        {
            Context.AddWarning(FText::Format(
                LOCTEXT("SetNotListed", "'{0}' belongs to a set not listed here. It applies only if another Gameplay Data on the same character adds that set"),
                FText::FromString(Attribute.GetName())));
        }
    }

    return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
