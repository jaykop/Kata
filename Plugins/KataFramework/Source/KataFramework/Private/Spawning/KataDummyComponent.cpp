#include "Spawning/KataDummyComponent.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "Action/KataAction.h"
#include "Attributes/KataAttributeSet_Base.h"
#include "Attributes/KataCombatSettings.h"
#include "Character/KataCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Death/KataDeathComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectTypes.h"
#include "HitReaction/KataHitReactionAbility.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "KataFrameworkLog.h"
#include "KataGraph.h"
#include "KataGraphComponent.h"
#include "KataGraphInstance.h"
#include "Runtime/KataActionComponent.h"
#include "Runtime/KataActionInstance.h"
#include "Spawning/KataCharacterSpawner.h"

UKataDummyComponent::UKataDummyComponent()
{
    // 반복이나 피격 정보 표시가 필요할 때만 BeginPlay에서 켠다.
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
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
    StartRepeating();
    BindHitInfo();
    UpdateTickEnabled();
}

void UKataDummyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindHitInfo();
    StopRepeating();
    if (UKataDeathComponent* DeathComponent = FindOwnerDeathComponent())
    {
        DeathComponent->OnDeathNative.RemoveAll(this);
    }
    RemoveProtection();
    Super::EndPlay(EndPlayReason);
}

void UKataDummyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (bHitInfoActive)
    {
        DrawHitInfo();
    }
    if (!bRepeatActive)
    {
        return;
    }
    // ASC가 아직 이 캐릭터에 연결되지 않았으면 Action·Graph가 시작을 거절하므로 준비될 때까지 기다린다.
    const AKataCharacter* Character = Cast<AKataCharacter>(GetOwner());
    const UAbilitySystemComponent* AbilitySystem = Character != nullptr ? Character->GetAbilitySystemComponent() : nullptr;
    if (AbilitySystem == nullptr || AbilitySystem->GetAvatarActor() != Character)
    {
        return;
    }
    const UKataActionComponent* ActionComponent = Character->FindComponentByClass<UKataActionComponent>();
    const UKataGraphComponent* GraphComponent = Character->FindComponentByClass<UKataGraphComponent>();
    // 피격 반응처럼 이 컴포넌트가 시작하지 않은 실행도 끝날 때까지 기다린다.
    const bool bBusy = (ActionComponent != nullptr && ActionComponent->IsPlayingKata())
        || (GraphComponent != nullptr && GraphComponent->IsRunningGraph());
    if (bBusy)
    {
        RepeatIdleTime = 0.f;
        return;
    }
    if (bRepeatRunning)
    {
        // 실행이 끝났다. 복귀한 다음 틱부터 간격을 센다.
        bRepeatRunning = false;
        if (Settings.bReturnToStart)
        {
            ReturnToStart();
        }
        RepeatIdleTime = 0.f;
        return;
    }
    RepeatIdleTime += DeltaTime;
    if (RepeatIdleTime < FMath::Max(Settings.RepeatInterval, 0.f))
    {
        return;
    }
    RepeatIdleTime = 0.f;
    bRepeatRunning = PlayOnce();
}

void UKataDummyComponent::StartRepeating()
{
    if (Settings.RepeatMode == EKataDummyRepeatMode::None)
    {
        return;
    }
    const AActor* Owner = GetOwner();
    const bool bAction = Settings.RepeatMode == EKataDummyRepeatMode::Action;
    if (bAction ? Settings.RepeatAction == nullptr : Settings.RepeatGraph == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Dummy on %s will not repeat: no %s is set."),
            *GetNameSafe(Owner), bAction ? TEXT("action") : TEXT("graph"));
        return;
    }
    const bool bHasComponent = Owner != nullptr && (bAction
        ? Owner->FindComponentByClass<UKataActionComponent>() != nullptr
        : Owner->FindComponentByClass<UKataGraphComponent>() != nullptr);
    if (!bHasComponent)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Dummy on %s will not repeat: the owner has no Kata %s component."),
            *GetNameSafe(Owner), bAction ? TEXT("Action") : TEXT("Graph"));
        return;
    }
    bRepeatActive = true;
    bRepeatRunning = false;
    bWarnedEntryTrigger = false;
    // 첫 실행은 간격을 기다리지 않는다.
    RepeatIdleTime = FMath::Max(Settings.RepeatInterval, 0.f);
    UpdateTickEnabled();
}

void UKataDummyComponent::StopRepeating()
{
    bRepeatActive = false;
    bRepeatRunning = false;
    UpdateTickEnabled();
}

void UKataDummyComponent::UpdateTickEnabled()
{
    SetComponentTickEnabled(bRepeatActive || bHitInfoActive);
}

void UKataDummyComponent::BindHitInfo()
{
#if ENABLE_DRAW_DEBUG
    if (!Settings.bShowHitInfo)
    {
        return;
    }
    AKataCharacter* Character = Cast<AKataCharacter>(GetOwner());
    UAbilitySystemComponent* AbilitySystem = Character != nullptr ? Character->GetAbilitySystemComponent() : nullptr;
    if (AbilitySystem == nullptr)
    {
        return;
    }
    DamageChangedHandle = AbilitySystem->GetGameplayAttributeValueChangeDelegate(UKataAttributeSet_Base::GetDamageAttribute())
        .AddUObject(this, &UKataDummyComponent::HandleDamageChanged);

    // 플러그인은 반응 태그를 정의하지 않으므로 피해 GE 컴포넌트가 보내는 태그를 Kata Combat 설정에서 모은다.
    ReactionEventTags.Reset();
    if (const UKataCombatSettings* CombatSettings = UKataCombatSettings::Get())
    {
        ReactionEventTags.AddTag(CombatSettings->FlinchEventTag);
        ReactionEventTags.AddTag(CombatSettings->GroggyEventTag);
        for (const FKataImpactResponse& Response : CombatSettings->ImpactResponses)
        {
            ReactionEventTags.AddTag(Response.ReactionEventTag);
        }
    }
    if (!ReactionEventTags.IsEmpty())
    {
        ReactionEventHandle = AbilitySystem->AddGameplayEventTagContainerDelegate(ReactionEventTags,
            FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UKataDummyComponent::HandleReactionEvent));
    }
    bHitInfoActive = true;
#endif
}

void UKataDummyComponent::UnbindHitInfo()
{
#if ENABLE_DRAW_DEBUG
    bHitInfoActive = false;
    const AKataCharacter* Character = Cast<AKataCharacter>(GetOwner());
    if (UAbilitySystemComponent* AbilitySystem = Character != nullptr ? Character->GetAbilitySystemComponent() : nullptr)
    {
        if (DamageChangedHandle.IsValid())
        {
            AbilitySystem->GetGameplayAttributeValueChangeDelegate(UKataAttributeSet_Base::GetDamageAttribute()).Remove(DamageChangedHandle);
        }
        if (ReactionEventHandle.IsValid())
        {
            AbilitySystem->RemoveGameplayEventTagContainerDelegate(ReactionEventTags, ReactionEventHandle);
        }
    }
    DamageChangedHandle.Reset();
    ReactionEventHandle.Reset();
#endif
}

void UKataDummyComponent::HandleDamageChanged(const FOnAttributeChangeData& ChangeData)
{
    // 피해 실행이 메타 Attribute를 계산 피해로 올린 변경만 기록한다. 처리한 뒤 0으로 되돌리는 변경은 무시한다.
    if (ChangeData.NewValue <= 0.f)
    {
        return;
    }
    const FHitResult* HitResult = nullptr;
    const AActor* Attacker = nullptr;
    if (ChangeData.GEModData != nullptr)
    {
        const FGameplayEffectContextHandle& EffectContext = ChangeData.GEModData->EffectSpec.GetEffectContext();
        HitResult = EffectContext.GetHitResult();
        Attacker = EffectContext.GetOriginalInstigator();
    }
    RecordHit(HitResult, Attacker, ChangeData.NewValue, FGameplayTag());
}

void UKataDummyComponent::HandleReactionEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
    const FHitResult* HitResult = Payload != nullptr ? Payload->ContextHandle.GetHitResult() : nullptr;
    const AActor* Attacker = Payload != nullptr ? Payload->Instigator.Get() : nullptr;
    RecordHit(HitResult, Attacker, -1.f, EventTag);
}

void UKataDummyComponent::RecordHit(const FHitResult* HitResult, const AActor* Attacker, float Damage, FGameplayTag ReactionTag)
{
    const UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }
    // 피해 실행 뒤 같은 GE 처리 안에서 반응 이벤트가 오므로 같은 월드 시간이면 한 피격으로 합친다.
    const double Now = World->GetTimeSeconds();
    const bool bSameHit = LastHit.Time == Now;
    if (!bSameHit)
    {
        LastHit = FHitInfo();
        LastHit.Time = Now;
    }
    if (Damage >= 0.f)
    {
        LastHit.Damage = Damage;
    }
    if (ReactionTag.IsValid())
    {
        LastHit.ReactionTag = ReactionTag;
    }
    if (!bSameHit || LastHit.Direction.IsEmpty())
    {
        // 반응 Ability와 같은 판정을 써서 재생된 반응 방향과 표시가 어긋나지 않게 한다.
        const UKataCombatSettings* CombatSettings = UKataCombatSettings::Get();
        const EKataHitDirection Direction = UKataHitReactionAbility::ResolveHitDirection(GetOwner(),
            HitResult != nullptr ? *HitResult : FHitResult(), Attacker, CombatSettings != nullptr ? CombatSettings->MinHorizontalDirectionRatio : 0.5f);
        LastHit.Direction = StaticEnum<EKataHitDirection>()->GetNameStringByValue(static_cast<int64>(Direction));
    }
}

void UKataDummyComponent::DrawHitInfo() const
{
#if ENABLE_DRAW_DEBUG
    const UWorld* World = GetWorld();
    const AKataCharacter* Character = Cast<AKataCharacter>(GetOwner());
    if (World == nullptr || Character == nullptr || LastHit.Time < 0.0)
    {
        return;
    }
    if (Settings.HitInfoDuration > 0.f && World->GetTimeSeconds() - LastHit.Time > Settings.HitInfoDuration)
    {
        return;
    }
    const UAbilitySystemComponent* AbilitySystem = Character->GetAbilitySystemComponent();
    const UKataAttributeSet_Base* BaseSet = AbilitySystem != nullptr ? AbilitySystem->GetSet<UKataAttributeSet_Base>() : nullptr;
    const FString Reaction = LastHit.ReactionTag.IsValid() ? LastHit.ReactionTag.ToString() : FString(TEXT("-"));
    const FString DamageText = LastHit.Damage >= 0.f ? FString::Printf(TEXT("%.1f"), LastHit.Damage) : FString(TEXT("-"));
    const FString HealthText = BaseSet != nullptr
        ? FString::Printf(TEXT("%.0f/%.0f"), BaseSet->GetHealth(), BaseSet->GetMaxHealth()) : FString(TEXT("-"));

    const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
    const float HalfHeight = Capsule != nullptr ? Capsule->GetScaledCapsuleHalfHeight() : 0.f;
    const FVector TextOrigin = Character->GetActorLocation() + FVector(0.0, 0.0, HalfHeight + 30.0);
    // 두 줄은 높이를 달리해 따로 그려 디버그 문자열의 줄바꿈 처리에 기대지 않는다.
    // DrawDebugString의 Duration -1은 HUD가 만료 없음으로 처리해 매 프레임 쌓이므로, 한 번 그리고 지워지는 0을 쓴다.
    DrawDebugString(World, TextOrigin + FVector(0.0, 0.0, 20.0), FString::Printf(TEXT("%s / %s"), *Reaction, *LastHit.Direction),
        nullptr, FColor::White, 0.f, true);
    DrawDebugString(World, TextOrigin, FString::Printf(TEXT("Damage %s / HP %s"), *DamageText, *HealthText),
        nullptr, FColor::White, 0.f, true);
#endif
}

bool UKataDummyComponent::PlayOnce()
{
    AActor* Owner = GetOwner();
    const UWorld* World = GetWorld();
    const APlayerController* PlayerController = World != nullptr ? World->GetFirstPlayerController() : nullptr;
    AActor* Target = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
    // 재시작을 기다리는 동안처럼 플레이어 Pawn이 없으면 이번 간격은 쉬고 다음 간격에 다시 시도한다.
    if (Owner == nullptr || Target == nullptr)
    {
        return false;
    }

    if (Settings.RepeatMode == EKataDummyRepeatMode::Action)
    {
        UKataActionComponent* ActionComponent = Owner->FindComponentByClass<UKataActionComponent>();
        if (ActionComponent == nullptr || Settings.RepeatAction == nullptr)
        {
            StopRepeating();
            return false;
        }
        UKataActionInstance* Instance = nullptr;
        const EKataStartResult Result = ActionComponent->PlayKataActionOnSelf(Settings.RepeatAction, Target, Instance);
        // 조건·비용 거절은 다음 간격에 다시 시도한다.
        UE_CLOG(Result != EKataStartResult::Started, LogKataFramework, Verbose, TEXT("Dummy repeat action '%s' rejected on %s: %s"),
            *Settings.RepeatAction->GetName(), *Owner->GetName(), *UEnum::GetValueAsString(Result));
        return Result == EKataStartResult::Started;
    }

    UKataGraphComponent* GraphComponent = Owner->FindComponentByClass<UKataGraphComponent>();
    if (GraphComponent == nullptr || Settings.RepeatGraph == nullptr)
    {
        StopRepeating();
        return false;
    }
    UKataGraphInstance* Instance = nullptr;
    if (!GraphComponent->StartGraphOnSelf(Settings.RepeatGraph, Target, Instance) || !IsValid(Instance))
    {
        return false;
    }
    if (Instance->GetState() == EKataGraphInstanceState::WaitingForEntry && Settings.EntryTrigger.IsValid())
    {
        Instance->SendTrigger(Settings.EntryTrigger);
    }
    // 진입 대기에 머물면 그래프가 계속 실행 중으로 남아 반복이 멈추므로 정지시키고 원인을 한 번만 알린다.
    if (Instance->GetState() == EKataGraphInstanceState::WaitingForEntry)
    {
        GraphComponent->StopGraph(EKataEndReason::Cancelled);
        if (!bWarnedEntryTrigger)
        {
            bWarnedEntryTrigger = true;
            UE_LOG(LogKataFramework, Warning, TEXT("Dummy graph '%s' on %s is waiting for an entry trigger. Set an Entry Trigger that the graph accepts (current: %s)."),
                *Settings.RepeatGraph->GetName(), *Owner->GetName(), *Settings.EntryTrigger.ToString());
        }
        return false;
    }
    return true;
}

void UKataDummyComponent::ReturnToStart()
{
    AKataCharacter* Character = Cast<AKataCharacter>(GetOwner());
    if (Character == nullptr)
    {
        return;
    }
    // 남은 속도가 순간이동 뒤에도 이어지지 않게 먼저 멈춘다.
    if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }
    const FRotator StartRotation = StartTransform.Rotator();
    if (!Character->TeleportTo(StartTransform.GetLocation(), StartRotation))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Dummy on %s could not return to its start location. It stays where it is."),
            *Character->GetName());
        return;
    }
    // Controller 회전을 쓰는 캐릭터는 다음 틱에 Controller 방향으로 돌아가므로 함께 맞춘다.
    if (AController* Controller = Character->GetController())
    {
        Controller->SetControlRotation(StartRotation);
    }
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
    StopRepeating();
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
