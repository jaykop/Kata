#include "Cheats/KataFrameworkCheatExtension.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Attributes/KataAttributeSet_Base.h"
#include "Attributes/KataAttributeSet_Stance.h"
#include "Attributes/KataCombatSettings.h"
#include "Death/KataDeathComponent.h"
#include "Engine/World.h"
#include "FunctionLibraries/KataFL_CheatTarget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "KataFrameworkLog.h"
#include "Player/KataGameMode.h"

#if UE_WITH_CHEAT_MANAGER
namespace KataFrameworkCheat
{
    /** 대상 인자를 해석해 액터를 모은다. 죽은 대상은 Nearest·All에서 뺀다. 인자를 알 수 없으면 false. */
    bool ResolveTargets(const APlayerController* PlayerController, const FString& Text, TArray<AActor*>& OutTargets, EKataCheatTarget& OutTarget)
    {
        if (!KataFL::ParseCheatTarget(Text, EKataCheatTarget::Target, OutTarget))
        {
            return false;
        }

        FGameplayTagContainer ExcludedTags;
        const FGameplayTag& DeadStatusTag = UKataCombatSettings::Get()->DeadStatusTag;
        if (DeadStatusTag.IsValid())
        {
            ExcludedTags.AddTag(DeadStatusTag);
        }
        OutTargets = KataFL::ResolveCheatTargets(PlayerController, OutTarget, ExcludedTags);
        return true;
    }

    const TCHAR* OnOff(bool bEnabled)
    {
        return bEnabled ? TEXT("on") : TEXT("off");
    }
}
#endif

FDelegateHandle UKataFrameworkCheatExtension::RegisterForCheatManagers()
{
#if UE_WITH_CHEAT_MANAGER
    return UCheatManager::RegisterForOnCheatManagerCreated(FOnCheatManagerCreated::FDelegate::CreateLambda([](UCheatManager* CheatManager)
    {
        CheatManager->AddCheatManagerExtension(NewObject<UKataFrameworkCheatExtension>(CheatManager));
    }));
#else
    return FDelegateHandle();
#endif
}

void UKataFrameworkCheatExtension::UnregisterFromCheatManagers(FDelegateHandle Handle)
{
#if UE_WITH_CHEAT_MANAGER
    if (Handle.IsValid())
    {
        UCheatManager::UnregisterFromOnCheatManagerCreated(Handle);
    }
#endif
}

void UKataFrameworkCheatExtension::KataKill(const FString& Target)
{
#if UE_WITH_CHEAT_MANAGER
    TArray<AActor*> Targets;
    EKataCheatTarget TargetKind;
    if (!KataFrameworkCheat::ResolveTargets(GetPlayerController(), Target, Targets, TargetKind))
    {
        Report(FString::Printf(TEXT("KataKill: unknown target '%s'. Use Self, Target, Nearest or All."), *Target));
        return;
    }

    int32 KilledCount = 0;
    for (AActor* Actor : Targets)
    {
        KilledCount += KillActor(Actor) ? 1 : 0;
    }
    Report(FString::Printf(TEXT("KataKill %s: killed %d of %d"), KataFL::GetCheatTargetName(TargetKind), KilledCount, Targets.Num()));
#endif
}

void UKataFrameworkCheatExtension::KataGroggy(const FString& Target)
{
#if UE_WITH_CHEAT_MANAGER
    TArray<AActor*> Targets;
    EKataCheatTarget TargetKind;
    if (!KataFrameworkCheat::ResolveTargets(GetPlayerController(), Target, Targets, TargetKind))
    {
        Report(FString::Printf(TEXT("KataGroggy: unknown target '%s'. Use Self, Target, Nearest or All."), *Target));
        return;
    }
    if (!UKataCombatSettings::Get()->GroggyEventTag.IsValid())
    {
        Report(TEXT("KataGroggy: Groggy Event Tag is empty in the Kata Combat settings."));
        return;
    }

    int32 GroggyCount = 0;
    for (AActor* Actor : Targets)
    {
        GroggyCount += MakeActorGroggy(Actor) ? 1 : 0;
    }
    Report(FString::Printf(TEXT("KataGroggy %s: sent to %d of %d"), KataFL::GetCheatTargetName(TargetKind), GroggyCount, Targets.Num()));
#endif
}

void UKataFrameworkCheatExtension::KataGod(const FString& Enable)
{
#if UE_WITH_CHEAT_MANAGER
    bool bEnabled = bGod;
    if (!KataFL::ParseCheatToggle(Enable, bGod, bEnabled))
    {
        Report(FString::Printf(TEXT("KataGod: unknown value '%s'. Use 0 or 1."), *Enable));
        return;
    }
    SetGodEnabled(bEnabled);
    Report(FString::Printf(TEXT("KataGod %s"), KataFrameworkCheat::OnOff(bGod)));
#endif
}

void UKataFrameworkCheatExtension::KataNoDamage(const FString& Enable)
{
#if UE_WITH_CHEAT_MANAGER
    bool bEnabled = bNoDamage;
    if (!KataFL::ParseCheatToggle(Enable, bNoDamage, bEnabled))
    {
        Report(FString::Printf(TEXT("KataNoDamage: unknown value '%s'. Use 0 or 1."), *Enable));
        return;
    }
    SetNoDamageEnabled(bEnabled);
    Report(FString::Printf(TEXT("KataNoDamage %s"), KataFrameworkCheat::OnOff(bNoDamage)));
#endif
}

void UKataFrameworkCheatExtension::KataInfiniteStamina(const FString& Enable)
{
#if UE_WITH_CHEAT_MANAGER
    bool bEnabled = bInfiniteStamina;
    if (!KataFL::ParseCheatToggle(Enable, bInfiniteStamina, bEnabled))
    {
        Report(FString::Printf(TEXT("KataInfiniteStamina: unknown value '%s'. Use 0 or 1."), *Enable));
        return;
    }
    SetInfiniteStaminaEnabled(bEnabled);
    Report(FString::Printf(TEXT("KataInfiniteStamina %s"), KataFrameworkCheat::OnOff(bInfiniteStamina)));
#endif
}

void UKataFrameworkCheatExtension::KataRespawn()
{
#if UE_WITH_CHEAT_MANAGER
    Report(RespawnPlayer() ? TEXT("KataRespawn requested") : TEXT("KataRespawn: the current game mode is not a Kata Game Mode."));
#endif
}

bool UKataFrameworkCheatExtension::KillActor(AActor* Actor) const
{
#if UE_WITH_CHEAT_MANAGER
    UKataDeathComponent* DeathComponent = Actor != nullptr ? Actor->FindComponentByClass<UKataDeathComponent>() : nullptr;
    APawn* Instigator = GetPlayerController() != nullptr ? GetPlayerController()->GetPawn() : nullptr;
    return DeathComponent != nullptr && DeathComponent->Kill(Instigator);
#else
    return false;
#endif
}

bool UKataFrameworkCheatExtension::MakeActorGroggy(AActor* Actor) const
{
#if UE_WITH_CHEAT_MANAGER
    const FGameplayTag& GroggyEventTag = UKataCombatSettings::Get()->GroggyEventTag;
    UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
    const UKataAttributeSet_Stance* Stance = AbilitySystem != nullptr ? AbilitySystem->GetSet<UKataAttributeSet_Stance>() : nullptr;
    if (!GroggyEventTag.IsValid() || Stance == nullptr || Stance->GetMaxGroggy() <= 0.0f)
    {
        return false;
    }

    // 피격 반응 GE 컴포넌트가 Groggy 가득 참일 때 하는 일과 같게 맞춘다. 게이지를 채워 두어야 반응 종료 시 초기화 흐름도 같아진다.
    AbilitySystem->SetNumericAttributeBase(UKataAttributeSet_Stance::GetGroggyAttribute(), Stance->GetMaxGroggy());

    FGameplayEventData Payload;
    Payload.EventTag = GroggyEventTag;
    Payload.Instigator = GetPlayerController() != nullptr ? GetPlayerController()->GetPawn() : nullptr;
    Payload.Target = Actor;
    AbilitySystem->HandleGameplayEvent(GroggyEventTag, &Payload);
    return true;
#else
    return false;
#endif
}

void UKataFrameworkCheatExtension::SetGodEnabled(bool bEnabled)
{
#if UE_WITH_CHEAT_MANAGER
    bGod = bEnabled;
    SyncPawn(GetPlayerController() != nullptr ? GetPlayerController()->GetPawn() : nullptr, true);
#endif
}

void UKataFrameworkCheatExtension::SetNoDamageEnabled(bool bEnabled)
{
#if UE_WITH_CHEAT_MANAGER
    bNoDamage = bEnabled;
    SyncPawn(GetPlayerController() != nullptr ? GetPlayerController()->GetPawn() : nullptr, true);
#endif
}

void UKataFrameworkCheatExtension::SetInfiniteStaminaEnabled(bool bEnabled)
{
#if UE_WITH_CHEAT_MANAGER
    bInfiniteStamina = bEnabled;
    SyncPawn(GetPlayerController() != nullptr ? GetPlayerController()->GetPawn() : nullptr, true);
#endif
}

bool UKataFrameworkCheatExtension::RespawnPlayer() const
{
#if UE_WITH_CHEAT_MANAGER
    APlayerController* PlayerController = GetPlayerController();
    UWorld* World = GetWorld();
    AKataGameMode* GameMode = World != nullptr ? World->GetAuthGameMode<AKataGameMode>() : nullptr;
    if (PlayerController == nullptr || GameMode == nullptr)
    {
        return false;
    }
    GameMode->RestartPlayerNow(PlayerController);
    return true;
#else
    return false;
#endif
}

void UKataFrameworkCheatExtension::AddedToCheatManager_Implementation()
{
    Super::AddedToCheatManager_Implementation();

#if UE_WITH_CHEAT_MANAGER
    if (APlayerController* PlayerController = GetPlayerController())
    {
        PlayerController->OnPossessedPawnChanged.AddUniqueDynamic(this, &UKataFrameworkCheatExtension::HandlePossessedPawnChanged);
    }
#endif
}

void UKataFrameworkCheatExtension::RemovedFromCheatManager_Implementation()
{
#if UE_WITH_CHEAT_MANAGER
    if (APlayerController* PlayerController = GetPlayerController())
    {
        PlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &UKataFrameworkCheatExtension::HandlePossessedPawnChanged);
        SyncPawn(PlayerController->GetPawn(), false);
    }
    UnbindStamina();
#endif

    Super::RemovedFromCheatManager_Implementation();
}

void UKataFrameworkCheatExtension::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
#if UE_WITH_CHEAT_MANAGER
    // 사망 뒤 재생성되면 새 캐릭터는 태그 없이 시작하므로 켜 둔 토글을 다시 건다. 시체에 남은 치트 태그는 걷어 낸다.
    SyncPawn(OldPawn, false);
    SyncPawn(NewPawn, true);
#endif
}

void UKataFrameworkCheatExtension::SyncPawn(APawn* Pawn, bool bApply)
{
#if UE_WITH_CHEAT_MANAGER
    UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
    if (AbilitySystem == nullptr)
    {
        // Pawn이 없으면 이전 Pawn의 구독을 붙들고 있을 이유가 없다. 새 Pawn을 받으면 다시 구독한다.
        UnbindStamina();
        return;
    }

    const UKataCombatSettings* Settings = UKataCombatSettings::Get();
    SyncLooseTag(*AbilitySystem, Settings->CheatInvincibleTag, bApply && bGod, TEXT("Cheat Invincible Tag"));
    SyncLooseTag(*AbilitySystem, Settings->CheatIgnoreDamageTag, bApply && bNoDamage, TEXT("Cheat Ignore Damage Tag"));

    if (bApply && bInfiniteStamina)
    {
        BindStamina(*AbilitySystem);
    }
    else if (StaminaAbilitySystem.Get() == AbilitySystem)
    {
        UnbindStamina();
    }
#endif
}

void UKataFrameworkCheatExtension::SyncLooseTag(UAbilitySystemComponent& AbilitySystem, const FGameplayTag& Tag, bool bEnabled, const TCHAR* SettingName) const
{
#if UE_WITH_CHEAT_MANAGER
    if (!Tag.IsValid())
    {
        if (bEnabled)
        {
            Report(FString::Printf(TEXT("%s is empty in the Kata Combat settings. The cheat has no effect."), SettingName));
        }
        return;
    }

    // 여러 번 켜도 태그가 쌓이지 않도록 개수를 직접 0 또는 1로 둔다.
    AbilitySystem.SetLooseGameplayTagCount(Tag, bEnabled ? 1 : 0);
#endif
}

void UKataFrameworkCheatExtension::BindStamina(UAbilitySystemComponent& AbilitySystem)
{
#if UE_WITH_CHEAT_MANAGER
    if (StaminaAbilitySystem.Get() != &AbilitySystem)
    {
        UnbindStamina();
        StaminaAbilitySystem = &AbilitySystem;
        StaminaChangedHandle = AbilitySystem.GetGameplayAttributeValueChangeDelegate(UKataAttributeSet_Base::GetStaminaAttribute())
            .AddUObject(this, &UKataFrameworkCheatExtension::HandleStaminaChanged);
    }

    // 세트가 아직 부여되지 않았으면 채울 값이 없다. 부여 뒤 첫 감소부터 되돌린다.
    if (const UKataAttributeSet_Base* Base = AbilitySystem.GetSet<UKataAttributeSet_Base>())
    {
        AbilitySystem.SetNumericAttributeBase(UKataAttributeSet_Base::GetStaminaAttribute(), Base->GetMaxStamina());
    }
#endif
}

void UKataFrameworkCheatExtension::UnbindStamina()
{
#if UE_WITH_CHEAT_MANAGER
    if (UAbilitySystemComponent* AbilitySystem = StaminaAbilitySystem.Get())
    {
        AbilitySystem->GetGameplayAttributeValueChangeDelegate(UKataAttributeSet_Base::GetStaminaAttribute()).Remove(StaminaChangedHandle);
    }
    StaminaAbilitySystem.Reset();
    StaminaChangedHandle.Reset();
#endif
}

void UKataFrameworkCheatExtension::HandleStaminaChanged(const FOnAttributeChangeData& Data)
{
#if UE_WITH_CHEAT_MANAGER
    // 되돌린 값으로 다시 불려도 증가이므로 반복되지 않는다.
    UAbilitySystemComponent* AbilitySystem = StaminaAbilitySystem.Get();
    if (!bInfiniteStamina || AbilitySystem == nullptr || Data.NewValue >= Data.OldValue)
    {
        return;
    }
    if (const UKataAttributeSet_Base* Base = AbilitySystem->GetSet<UKataAttributeSet_Base>())
    {
        AbilitySystem->SetNumericAttributeBase(UKataAttributeSet_Base::GetStaminaAttribute(), Base->GetMaxStamina());
    }
#endif
}

void UKataFrameworkCheatExtension::Report(const FString& Message) const
{
#if UE_WITH_CHEAT_MANAGER
    UE_LOG(LogKataFramework, Display, TEXT("%s"), *Message);
    if (APlayerController* PlayerController = GetPlayerController())
    {
        PlayerController->ClientMessage(Message);
    }
#endif
}
