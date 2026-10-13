#include "UI/KataHUDSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Attributes/KataAttributeSet_Base.h"
#include "GameFramework/Pawn.h"
#include "INotifyFieldValueChanged.h"
#include "KataFrameworkLog.h"
#include "UI/KataVM_PlayerStatus.h"

namespace
{
    /** 플레이어 상태 ViewModel이 따르는 Attribute. AttributeChangeHandles의 순서와 같다. */
    TArray<FGameplayAttribute> GetPlayerStatusAttributes()
    {
        return {
            UKataAttributeSet_Base::GetHealthAttribute(),
            UKataAttributeSet_Base::GetMaxHealthAttribute(),
            UKataAttributeSet_Base::GetStaminaAttribute(),
            UKataAttributeSet_Base::GetMaxStaminaAttribute(),
            UKataAttributeSet_Base::GetManaAttribute(),
            UKataAttributeSet_Base::GetMaxManaAttribute()
        };
    }

    void ApplyAttribute(UKataVM_PlayerStatus& Status, const FGameplayAttribute& Attribute, float Value)
    {
        if (Attribute == UKataAttributeSet_Base::GetHealthAttribute())
        {
            Status.SetHealth(Value);
        }
        else if (Attribute == UKataAttributeSet_Base::GetMaxHealthAttribute())
        {
            Status.SetMaxHealth(Value);
        }
        else if (Attribute == UKataAttributeSet_Base::GetStaminaAttribute())
        {
            Status.SetStamina(Value);
        }
        else if (Attribute == UKataAttributeSet_Base::GetMaxStaminaAttribute())
        {
            Status.SetMaxStamina(Value);
        }
        else if (Attribute == UKataAttributeSet_Base::GetManaAttribute())
        {
            Status.SetMana(Value);
        }
        else if (Attribute == UKataAttributeSet_Base::GetMaxManaAttribute())
        {
            Status.SetMaxMana(Value);
        }
    }
}

void UKataHUDSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PlayerStatus = NewObject<UKataVM_PlayerStatus>(this);
    ViewModels.Add(PlayerStatus);
}

void UKataHUDSubsystem::Deinitialize()
{
    ClearPlayerStatusBindings();
    PlayerStatus = nullptr;
    ViewModels.Reset();
    Super::Deinitialize();
}

UObject* UKataHUDSubsystem::FindOrAddViewModel(const UClass* ViewModelClass)
{
    if (!ViewModelClass)
    {
        return nullptr;
    }

    for (UObject* ViewModel : ViewModels)
    {
        // 정확히 같은 클래스만 재사용한다. 파생 클래스를 요청한 바인딩에 부모 인스턴스를 넘기지 않기 위해서다.
        if (ViewModel && ViewModel->GetClass() == ViewModelClass)
        {
            return ViewModel;
        }
    }

    if (!ViewModelClass->ImplementsInterface(UNotifyFieldValueChanged::StaticClass())
        || ViewModelClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("UKataHUDSubsystem: '%s' cannot be used as a HUD viewmodel."), *ViewModelClass->GetPathName());
        return nullptr;
    }

    UObject* NewViewModel = NewObject<UObject>(this, const_cast<UClass*>(ViewModelClass));
    ViewModels.Add(NewViewModel);
    return NewViewModel;
}

void UKataHUDSubsystem::SetObservedPawn(APawn* Pawn)
{
    ClearPlayerStatusBindings();
    if (!PlayerStatus)
    {
        return;
    }

    UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
    const bool bHasBaseSet = AbilitySystem != nullptr && AbilitySystem->HasAttributeSetForAttribute(UKataAttributeSet_Base::GetHealthAttribute());

    const TArray<FGameplayAttribute> Attributes = GetPlayerStatusAttributes();
    for (const FGameplayAttribute& Attribute : Attributes)
    {
        ApplyAttribute(*PlayerStatus, Attribute, bHasBaseSet ? AbilitySystem->GetNumericAttribute(Attribute) : 0.0f);
    }
    PlayerStatus->SetHasStatus(bHasBaseSet);

    if (!bHasBaseSet)
    {
        return;
    }

    ObservedAbilitySystem = AbilitySystem;
    AttributeChangeHandles.Reserve(Attributes.Num());
    for (const FGameplayAttribute& Attribute : Attributes)
    {
        AttributeChangeHandles.Add(AbilitySystem->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &UKataHUDSubsystem::HandleAttributeChanged));
    }
}

void UKataHUDSubsystem::ClearPlayerStatusBindings()
{
    if (UAbilitySystemComponent* AbilitySystem = ObservedAbilitySystem.Get())
    {
        const TArray<FGameplayAttribute> Attributes = GetPlayerStatusAttributes();
        for (int32 Index = 0; Index < AttributeChangeHandles.Num() && Index < Attributes.Num(); ++Index)
        {
            AbilitySystem->GetGameplayAttributeValueChangeDelegate(Attributes[Index]).Remove(AttributeChangeHandles[Index]);
        }
    }
    AttributeChangeHandles.Reset();
    ObservedAbilitySystem.Reset();
}

void UKataHUDSubsystem::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
    if (PlayerStatus)
    {
        ApplyAttribute(*PlayerStatus, Data.Attribute, Data.NewValue);
    }
}
