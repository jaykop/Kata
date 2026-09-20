#include "Runtime/KataComponent.h"

#include "AbilitySystemComponent.h"
#include "Definition/KataDefinition.h"
#include "Definition/KataAsset.h"
#include "Definition/KataResolvedDefinition.h"
#include "GAS/KataGasBridge.h"
#include "GameFramework/Actor.h"
#include "KataCondition.h"
#include "KataRuntimeLog.h"
#include "Runtime/KataInstance.h"

UKataComponent::UKataComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    // 물리·애니메이션 갱신보다 먼저 타임라인 경계를 처리한다.
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UKataComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (IsValid(ActiveInstance))
    {
        ActiveInstance->TickInstance(DeltaTime);
    }
}

void UKataComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(ActiveInstance))
    {
        // 소유자 파괴 시에도 태스크 자원을 한 번 정리한다.
        ActiveInstance->RequestEnd(EKataEndReason::OwnerInvalid);
    }
    ActiveInstance = nullptr;
    ResolvedDefinitionCache.Reset();

    Super::EndPlay(EndPlayReason);
}

bool UKataComponent::IsPlayingKata() const
{
    return IsValid(ActiveInstance) && ActiveInstance->IsRunning();
}

FGameplayTagContainer UKataComponent::GetActiveKataTags() const
{
    if (IsValid(ActiveInstance))
    {
        if (const UKataResolvedDefinition* Resolved = ActiveInstance->GetResolvedDefinition())
        {
            return Resolved->KataTags;
        }
    }
    return FGameplayTagContainer();
}

UKataResolvedDefinition* UKataComponent::GetOrResolveDefinition(TSubclassOf<UKataDefinition> DefinitionClass)
{
    if (DefinitionClass == nullptr)
    {
        return nullptr;
    }

    if (TObjectPtr<UKataResolvedDefinition>* Cached = ResolvedDefinitionCache.Find(DefinitionClass))
    {
        if (IsValid(*Cached))
        {
            return *Cached;
        }
    }

    UKataResolvedDefinition* Resolved = UKataDefinition::ResolveDefinition(DefinitionClass, this);
    if (Resolved != nullptr)
    {
        Resolved->LogDiagnostics();
        ResolvedDefinitionCache.Add(DefinitionClass, Resolved);
    }
    return Resolved;
}

void UKataComponent::ClearResolvedDefinitionCache()
{
    ResolvedDefinitionCache.Reset();
}

FKataContext UKataComponent::BuildContext(const FKataContext& InContext) const
{
    FKataContext Context = InContext;
    if (!Context.OwnerActor.IsValid())
    {
        Context.OwnerActor = GetOwner();
    }
    if (!Context.AvatarActor.IsValid())
    {
        Context.AvatarActor = Context.OwnerActor;
    }
    return Context;
}

EKataStartResult UKataComponent::CanPlayKata(TSubclassOf<UKataDefinition> DefinitionClass, const FKataContext& Context) const
{
    if (DefinitionClass == nullptr)
    {
        return EKataStartResult::InvalidDefinition;
    }

    const FKataContext ResolvedContext = BuildContext(Context);
    if (!ResolvedContext.HasValidOwner())
    {
        return EKataStartResult::InvalidContext;
    }

    const TObjectPtr<UKataResolvedDefinition>* Cached = ResolvedDefinitionCache.Find(DefinitionClass);
    const UKataResolvedDefinition* Resolved = (Cached != nullptr && IsValid(*Cached))
        ? Cached->Get()
        : UKataDefinition::ResolveDefinition(DefinitionClass, GetTransientPackage());

    return CanStartResolved(Resolved, ResolvedContext);
}

EKataStartResult UKataComponent::CanStartResolved(const UKataResolvedDefinition* Resolved, const FKataContext& ResolvedContext) const
{
    if (!ResolvedContext.HasValidOwner())
    {
        return EKataStartResult::InvalidContext;
    }
    if (Resolved == nullptr)
    {
        return EKataStartResult::InvalidDefinition;
    }
    if (Resolved->HasErrors())
    {
        return EKataStartResult::ResolveFailed;
    }

    // GAS는 필수다. ASC 없이 태그·쿨다운 정책을 적용할 수 없다.
    UAbilitySystemComponent* AbilitySystem = ResolvedContext.ResolveAbilitySystem();
    if (AbilitySystem == nullptr)
    {
        return EKataStartResult::MissingAbilitySystem;
    }

    const EKataStartResult TagResult = KataGas::CheckActivationTags(*Resolved, *AbilitySystem);
    if (TagResult != EKataStartResult::Started)
    {
        return TagResult;
    }

    if (KataGas::IsOnCooldown(*Resolved, *AbilitySystem))
    {
        return EKataStartResult::OnCooldown;
    }

    if (IsPlayingKata())
    {
        // 실행 중인 Kata의 차단 정책과 새 Kata의 중단 권한을 구분해 판정한다.
        const UKataResolvedDefinition* Active = ActiveInstance->GetResolvedDefinition();
        const bool bBlockedByActive = Active != nullptr
            && !Active->BlockingPolicy.BlockedKataTags.IsEmpty()
            && Resolved->KataTags.HasAny(Active->BlockingPolicy.BlockedKataTags);

        if (bBlockedByActive || !Resolved->BlockingPolicy.bCanInterruptActiveKata)
        {
            return EKataStartResult::BlockedByActiveKata;
        }
    }

    if (Resolved->StartCondition != nullptr)
    {
        // 조건 평가는 부작용이 없다. 비용 지불과 효과 적용은 실행 처리에서 한다.
        const FKataConditionContext ConditionContext = ResolvedContext.ToConditionContext();
        if (!Resolved->StartCondition->IsSatisfied(ConditionContext))
        {
            return EKataStartResult::ConditionFailed;
        }
    }

    return EKataStartResult::Started;
}

EKataStartResult UKataComponent::PlayKata(TSubclassOf<UKataDefinition> DefinitionClass, const FKataContext& Context, UKataInstance*& OutInstance)
{
    return StartResolved(GetOrResolveDefinition(DefinitionClass), BuildContext(Context), OutInstance);
}

EKataStartResult UKataComponent::PlayKataAsset(UKataAsset* Asset, const FKataContext& Context, UKataInstance*& OutInstance)
{
    return StartResolved(Asset ? Asset->Resolve(this) : nullptr, BuildContext(Context), OutInstance);
}

EKataStartResult UKataComponent::PlayKataAssetOnSelf(UKataAsset* Asset, AActor* TargetActor, UKataInstance*& OutInstance)
{
    FKataContext Context;
    Context.TargetActor = TargetActor;
    return PlayKataAsset(Asset, Context, OutInstance);
}

EKataStartResult UKataComponent::CanPlayKataAsset(UKataAsset* Asset, const FKataContext& Context) const
{
    return CanStartResolved(Asset ? Asset->Resolve(GetTransientPackage()) : nullptr, BuildContext(Context));
}

EKataStartResult UKataComponent::StartResolved(UKataResolvedDefinition* Resolved, const FKataContext& ResolvedContext, UKataInstance*& OutInstance)
{
    OutInstance = nullptr;
    const EKataStartResult CheckResult = CanStartResolved(Resolved, ResolvedContext);
    if (CheckResult != EKataStartResult::Started)
    {
        return CheckResult;
    }

    if (IsPlayingKata())
    {
        // 여기까지 왔다면 새 Kata가 중단 권한을 가진 경우다.
        ActiveInstance->RequestEnd(EKataEndReason::Interrupted);
    }

    UKataInstance* Instance = NewObject<UKataInstance>(this);
    const EKataStartResult InitResult = Instance->InitializeInstance(Resolved, ResolvedContext);
    if (InitResult != EKataStartResult::Started)
    {
        return InitResult;
    }

    ActiveInstance = Instance;
    OutInstance = Instance;
    Instance->OnKataEnded.AddDynamic(this, &UKataComponent::HandleInstanceEnded);

    // 순간 태스크만 있는 타임라인은 StartInstance에서 바로 끝나므로 시작 알림을 먼저 보낸다.
    OnKataStarted.Broadcast(Instance);
    Instance->StartInstance();

    return EKataStartResult::Started;
}

EKataStartResult UKataComponent::PlayKataOnSelf(TSubclassOf<UKataDefinition> DefinitionClass, AActor* TargetActor, UKataInstance*& OutInstance)
{
    FKataContext Context;
    Context.OwnerActor = GetOwner();
    Context.AvatarActor = GetOwner();
    Context.TargetActor = TargetActor;
    return PlayKata(DefinitionClass, Context, OutInstance);
}

void UKataComponent::StopKata(EKataEndReason Reason)
{
    if (IsValid(ActiveInstance))
    {
        ActiveInstance->RequestEnd(Reason);
    }
}

void UKataComponent::HandleInstanceEnded(UKataInstance* Instance, EKataEndReason EndReason)
{
    if (Instance == nullptr)
    {
        return;
    }

    Instance->OnKataEnded.RemoveDynamic(this, &UKataComponent::HandleInstanceEnded);

    if (ActiveInstance == Instance)
    {
        ActiveInstance = nullptr;
    }

    OnKataEnded.Broadcast(Instance, EndReason);
}
