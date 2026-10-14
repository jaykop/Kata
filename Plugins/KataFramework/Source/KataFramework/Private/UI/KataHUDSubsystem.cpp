#include "UI/KataHUDSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Attributes/KataAttributeSet_Base.h"
#include "Attributes/KataCombatEventSubsystem.h"
#include "Death/KataDeathComponent.h"
#include "Engine/World.h"
#include "FunctionLibraries/KataFL_Faction.h"
#include "GameFramework/Pawn.h"
#include "INotifyFieldValueChanged.h"
#include "KataFrameworkLog.h"
#include "UI/KataHUDSettings.h"
#include "UI/KataVM_EnemyHealth.h"
#include "UI/KataVM_EnemyHealthList.h"
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
    EnemyHealthList = NewObject<UKataVM_EnemyHealthList>(this);
    ViewModels.Add(EnemyHealthList);
}

void UKataHUDSubsystem::Deinitialize()
{
    ClearPlayerStatusBindings();
    ClearCombatEventBinding();
    PlayerStatus = nullptr;
    EnemyHealthList = nullptr;
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
    ObservedPawn = Pawn;
    RefreshCombatEventBinding(Pawn);
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

void UKataHUDSubsystem::SetLockTarget(AActor* Target)
{
    AActor* OldTarget = LockTarget.Get();
    if (OldTarget == Target)
    {
        return;
    }
    LockTarget = Target;

    if (FKataEnemyBarTracking* OldBar = FindEnemyBar(OldTarget))
    {
        OldBar->ViewModel->SetLockedOn(false);
        OldBar->HideAtTime = GetNow() + UKataHUDSettings::Get()->DisplayDuration;
    }
    // 락온은 대상을 직접 고른 것이므로 거리 제한을 두지 않는다.
    if (FKataEnemyBarTracking* NewBar = Target != nullptr ? FindOrAddEnemyBar(Target, true) : nullptr)
    {
        NewBar->ViewModel->SetLockedOn(true);
    }
}

void UKataHUDSubsystem::RefreshCombatEventBinding(APawn* Pawn)
{
    UKataCombatEventSubsystem* NewCombatEvents = Pawn != nullptr ? UKataCombatEventSubsystem::Get(Pawn) : nullptr;
    // 재시작 중 폰이 잠시 nullptr이 되어도 같은 월드라면 표시 중인 바를 지우지 않도록 구독을 유지한다.
    if (NewCombatEvents == nullptr || NewCombatEvents == CombatEvents.Get())
    {
        return;
    }
    ClearCombatEventBinding();
    CombatEvents = NewCombatEvents;
    DamageAppliedHandle = NewCombatEvents->OnDamageApplied.AddUObject(this, &UKataHUDSubsystem::HandleDamageApplied);
}

void UKataHUDSubsystem::ClearCombatEventBinding()
{
    if (UKataCombatEventSubsystem* OldCombatEvents = CombatEvents.Get())
    {
        OldCombatEvents->OnDamageApplied.Remove(DamageAppliedHandle);
    }
    DamageAppliedHandle.Reset();
    CombatEvents.Reset();
    ClearEnemyBars();
}

void UKataHUDSubsystem::HandleDamageApplied(AActor* Target, AActor* DamageInstigator, float Damage)
{
    const APawn* Pawn = ObservedPawn.Get();
    if (Pawn == nullptr || Target == nullptr || Target == Pawn)
    {
        return;
    }
    if (UKataHUDSettings::Get()->DamageSource == EKataEnemyBarDamageSource::PlayerOnly
        && DamageInstigator != Pawn && DamageInstigator != Pawn->GetController())
    {
        return;
    }
    if (FKataEnemyBarTracking* Bar = FindOrAddEnemyBar(Target, false))
    {
        Bar->HideAtTime = GetNow() + UKataHUDSettings::Get()->DisplayDuration;
    }
}

FKataEnemyBarTracking* UKataHUDSubsystem::FindEnemyBar(const AActor* Target)
{
    if (Target == nullptr)
    {
        return nullptr;
    }
    return EnemyBars.FindByPredicate([Target](const FKataEnemyBarTracking& Bar)
    {
        return Bar.ViewModel != nullptr && Bar.ViewModel->GetTargetActor() == Target;
    });
}

FKataEnemyBarTracking* UKataHUDSubsystem::FindOrAddEnemyBar(AActor* Target, bool bIgnoreDistance)
{
    if (FKataEnemyBarTracking* Existing = FindEnemyBar(Target))
    {
        return Existing;
    }

    const APawn* Pawn = ObservedPawn.Get();
    const UKataHUDSettings* Settings = UKataHUDSettings::Get();
    UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
    UKataDeathComponent* DeathComponent = Target->FindComponentByClass<UKataDeathComponent>();
    if (Pawn == nullptr || EnemyHealthList == nullptr
        || !UKataFL_Faction::IsHostile(Pawn, Target)
        || AbilitySystem == nullptr || !AbilitySystem->HasAttributeSetForAttribute(UKataAttributeSet_Base::GetHealthAttribute())
        || (DeathComponent != nullptr && DeathComponent->IsDead())
        || (!bIgnoreDistance && FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation()) > Settings->MaxDistance))
    {
        return nullptr;
    }

    // 자리가 없으면 락온하지 않은 항목 중 가장 먼저 사라질 항목을 지운다. 모두 락온 중이면 새로 띄우지 않는다.
    if (EnemyBars.Num() >= FMath::Max(Settings->MaxEnemyBars, 1))
    {
        int32 EvictIndex = INDEX_NONE;
        for (int32 Index = 0; Index < EnemyBars.Num(); ++Index)
        {
            if (!EnemyBars[Index].ViewModel->IsLockedOn()
                && (EvictIndex == INDEX_NONE || EnemyBars[Index].HideAtTime < EnemyBars[EvictIndex].HideAtTime))
            {
                EvictIndex = Index;
            }
        }
        if (EvictIndex == INDEX_NONE)
        {
            return nullptr;
        }
        RemoveEnemyBarAt(EvictIndex);
    }

    UKataVM_EnemyHealth* ViewModel = NewObject<UKataVM_EnemyHealth>(this);
    ViewModel->SetTargetActor(Target);
    ViewModel->SetHealth(AbilitySystem->GetNumericAttribute(UKataAttributeSet_Base::GetHealthAttribute()));
    ViewModel->SetMaxHealth(AbilitySystem->GetNumericAttribute(UKataAttributeSet_Base::GetMaxHealthAttribute()));

    FKataEnemyBarTracking& Bar = EnemyBars.AddDefaulted_GetRef();
    Bar.ViewModel = ViewModel;
    Bar.AbilitySystem = AbilitySystem;
    Bar.HideAtTime = GetNow() + Settings->DisplayDuration;

    // Attribute 콜백 인자에는 ASC가 없으므로, 어느 항목의 값인지 알 수 있도록 항목 ViewModel을 약한 참조로 묶는다.
    TWeakObjectPtr<UKataVM_EnemyHealth> WeakViewModel = ViewModel;
    Bar.HealthHandle = AbilitySystem->GetGameplayAttributeValueChangeDelegate(UKataAttributeSet_Base::GetHealthAttribute())
        .AddWeakLambda(this, [WeakViewModel](const FOnAttributeChangeData& Data)
        {
            if (UKataVM_EnemyHealth* Entry = WeakViewModel.Get())
            {
                Entry->SetHealth(Data.NewValue);
            }
        });
    Bar.MaxHealthHandle = AbilitySystem->GetGameplayAttributeValueChangeDelegate(UKataAttributeSet_Base::GetMaxHealthAttribute())
        .AddWeakLambda(this, [WeakViewModel](const FOnAttributeChangeData& Data)
        {
            if (UKataVM_EnemyHealth* Entry = WeakViewModel.Get())
            {
                Entry->SetMaxHealth(Data.NewValue);
            }
        });
    if (DeathComponent != nullptr)
    {
        Bar.DeathComponent = DeathComponent;
        Bar.DeathHandle = DeathComponent->OnDeathNative.AddWeakLambda(this, [this, WeakViewModel](UKataDeathComponent*)
        {
            UKataVM_EnemyHealth* Entry = WeakViewModel.Get();
            if (FKataEnemyBarTracking* DeadBar = Entry != nullptr ? FindEnemyBar(Entry->GetTargetActor()) : nullptr)
            {
                Entry->SetDead(true);
                DeadBar->DeadHideAtTime = GetNow() + UKataHUDSettings::Get()->DeadHideDelay;
            }
        });
    }

    EnemyHealthList->AddEntry(ViewModel);
    return &Bar;
}

void UKataHUDSubsystem::RemoveEnemyBarAt(int32 Index)
{
    if (!EnemyBars.IsValidIndex(Index))
    {
        return;
    }
    FKataEnemyBarTracking& Bar = EnemyBars[Index];
    if (UAbilitySystemComponent* AbilitySystem = Bar.AbilitySystem.Get())
    {
        AbilitySystem->GetGameplayAttributeValueChangeDelegate(UKataAttributeSet_Base::GetHealthAttribute()).Remove(Bar.HealthHandle);
        AbilitySystem->GetGameplayAttributeValueChangeDelegate(UKataAttributeSet_Base::GetMaxHealthAttribute()).Remove(Bar.MaxHealthHandle);
    }
    if (UKataDeathComponent* DeathComponent = Bar.DeathComponent.Get())
    {
        DeathComponent->OnDeathNative.Remove(Bar.DeathHandle);
    }
    if (EnemyHealthList != nullptr)
    {
        EnemyHealthList->RemoveEntry(Bar.ViewModel);
    }
    EnemyBars.RemoveAt(Index);
}

void UKataHUDSubsystem::ClearEnemyBars()
{
    for (int32 Index = EnemyBars.Num() - 1; Index >= 0; --Index)
    {
        RemoveEnemyBarAt(Index);
    }
}

double UKataHUDSubsystem::GetNow() const
{
    const UWorld* World = CombatEvents.IsValid() ? CombatEvents->GetWorld() : nullptr;
    return World != nullptr ? World->GetRealTimeSeconds() : 0.0;
}

void UKataHUDSubsystem::Tick(float DeltaTime)
{
    const double Now = GetNow();
    const APawn* Pawn = ObservedPawn.Get();
    const float MaxDistance = UKataHUDSettings::Get()->MaxDistance;
    for (int32 Index = EnemyBars.Num() - 1; Index >= 0; --Index)
    {
        const FKataEnemyBarTracking& Bar = EnemyBars[Index];
        const AActor* Target = Bar.ViewModel != nullptr ? Bar.ViewModel->GetTargetActor() : nullptr;
        const bool bLocked = Bar.ViewModel != nullptr && Bar.ViewModel->IsLockedOn();
        const bool bExpired = Target == nullptr
            || (Bar.DeadHideAtTime > 0.0 && Now >= Bar.DeadHideAtTime)
            || (!bLocked && Now >= Bar.HideAtTime)
            || (!bLocked && Pawn != nullptr && FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation()) > MaxDistance);
        if (bExpired)
        {
            RemoveEnemyBarAt(Index);
        }
    }
}

bool UKataHUDSubsystem::IsTickable() const
{
    return !HasAnyFlags(RF_ClassDefaultObject) && EnemyBars.Num() > 0;
}

TStatId UKataHUDSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UKataHUDSubsystem, STATGROUP_Tickables);
}
