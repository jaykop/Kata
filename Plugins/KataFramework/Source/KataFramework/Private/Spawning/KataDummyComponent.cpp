#include "Spawning/KataDummyComponent.h"

#include "AbilitySystemComponent.h"
#include "Attributes/KataCombatSettings.h"
#include "Character/KataCharacter.h"
#include "Death/KataDeathComponent.h"
#include "KataFrameworkLog.h"
#include "Spawning/KataCharacterSpawner.h"

UKataDummyComponent::UKataDummyComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UKataDummyComponent* UKataDummyComponent::AddDummyComponent(AKataCharacter* Character, const FKataDummySettings& InSettings,
    AKataCharacterSpawner* InSpawner)
{
    if (!IsValid(Character))
    {
        return nullptr;
    }
    if (UKataDummyComponent* Existing = Character->FindComponentByClass<UKataDummyComponent>())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Character %s already has a dummy component. The new dummy settings were ignored."),
            *Character->GetName());
        return Existing;
    }
    UKataDummyComponent* Dummy = NewObject<UKataDummyComponent>(Character, NAME_None, RF_Transient);
    Dummy->Settings = InSettings;
    Dummy->StartTransform = Character->GetActorTransform();
    Dummy->Spawner = InSpawner;
    Character->AddInstanceComponent(Dummy);
    // 생성 완료 뒤에 붙이면 캐릭터가 이미 BeginPlay를 마쳤으므로 등록하는 즉시 이 컴포넌트의 BeginPlay가 실행된다.
    Dummy->RegisterComponent();
    return Dummy;
}

void UKataDummyComponent::BeginPlay()
{
    Super::BeginPlay();

    ApplyProtection();
    if (UKataDeathComponent* DeathComponent = FindOwnerDeathComponent())
    {
        DeathComponent->OnDeathNative.AddUObject(this, &UKataDummyComponent::HandleOwnerDied);
    }
    else if (Settings.bRespawnOnDeath)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Dummy on %s will not respawn: the owner has no death component."), *GetNameSafe(GetOwner()));
    }
}

void UKataDummyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UKataDeathComponent* DeathComponent = FindOwnerDeathComponent())
    {
        DeathComponent->OnDeathNative.RemoveAll(this);
    }
    RemoveProtection();
    Super::EndPlay(EndPlayReason);
}

void UKataDummyComponent::ApplyProtection()
{
    const UKataCombatSettings* CombatSettings = UKataCombatSettings::Get();
    FGameplayTag ProtectionTag;
    switch (Settings.Protection)
    {
    case EKataDummyProtection::Invincible:
        ProtectionTag = CombatSettings != nullptr ? CombatSettings->InvincibleTag : FGameplayTag();
        break;
    case EKataDummyProtection::IgnoreDamage:
        ProtectionTag = CombatSettings != nullptr ? CombatSettings->IgnoreDamageTag : FGameplayTag();
        break;
    default:
        return;
    }
    if (!ProtectionTag.IsValid())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Dummy on %s skipped %s protection: the tag is not set in Kata Combat settings."),
            *GetNameSafe(GetOwner()), *UEnum::GetValueAsString(Settings.Protection));
        return;
    }
    const AKataCharacter* Character = Cast<AKataCharacter>(GetOwner());
    UAbilitySystemComponent* AbilitySystem = Character != nullptr ? Character->GetAbilitySystemComponent() : nullptr;
    if (AbilitySystem == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Dummy on %s skipped protection: the owner has no ability system."), *GetNameSafe(GetOwner()));
        return;
    }
    // 개수를 1로 덮어쓰지 않고 더해서, 같은 태그를 다른 경로가 붙였어도 그 기여를 지우지 않는다.
    AbilitySystem->AddLooseGameplayTag(ProtectionTag);
    AppliedProtectionTag = ProtectionTag;
}

void UKataDummyComponent::RemoveProtection()
{
    if (!AppliedProtectionTag.IsValid())
    {
        return;
    }
    const AKataCharacter* Character = Cast<AKataCharacter>(GetOwner());
    if (UAbilitySystemComponent* AbilitySystem = Character != nullptr ? Character->GetAbilitySystemComponent() : nullptr)
    {
        AbilitySystem->RemoveLooseGameplayTag(AppliedProtectionTag);
    }
    AppliedProtectionTag = FGameplayTag();
}

void UKataDummyComponent::HandleOwnerDied(UKataDeathComponent* DeathComponent)
{
    if (!Settings.bRespawnOnDeath)
    {
        return;
    }
    AKataCharacterSpawner* OwningSpawner = Spawner.Get();
    if (OwningSpawner == nullptr)
    {
        UE_LOG(LogKataFramework, Log, TEXT("Dummy on %s will not respawn: its spawner is gone or it was not spawned by a spawner."),
            *GetNameSafe(GetOwner()));
        return;
    }
    OwningSpawner->ScheduleRespawn(StartTransform, Settings.RespawnDelay);
}

UKataDeathComponent* UKataDummyComponent::FindOwnerDeathComponent() const
{
    const AKataCharacter* Character = Cast<AKataCharacter>(GetOwner());
    return Character != nullptr ? Character->GetDeathComponent() : nullptr;
}
