#include "Runtime/KataComponent.h"

#include "AbilitySystemComponent.h"
#include "Action/KataAction.h"
#include "Action/KataResolvedAction.h"
#include "GAS/KataGasBridge.h"
#include "GameFramework/Actor.h"
#include "KataCondition.h"
#include "KataRuntimeLog.h"
#include "Runtime/KataActionInstance.h"
#include "Runtime/KataExecutionWorldSubsystem.h"

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

    if (!bUsesWorldExecutionSubsystem && IsValid(ActiveInstance))
    {
        ActiveInstance->TickInstance(DeltaTime);
    }
}

void UKataComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UKataActionInstance* EndingInstance = ActiveInstance;
    if (IsValid(ActiveInstance))
    {
        // 소유자 파괴 시에도 태스크 자원을 한 번 정리한다.
        ActiveInstance->RequestEnd(EKataEndReason::OwnerInvalid);
    }
    UnregisterFromExecutionSubsystem(EndingInstance);
    ActiveInstance = nullptr;

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
        if (const UKataResolvedAction* Resolved = ActiveInstance->GetResolvedDefinition())
        {
            return Resolved->KataTags;
        }
    }
    return FGameplayTagContainer();
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

EKataStartResult UKataComponent::CanStartResolved(const UKataResolvedAction* Resolved, const FKataContext& ResolvedContext) const
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
        const UKataResolvedAction* Active = ActiveInstance->GetResolvedDefinition();
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

EKataStartResult UKataComponent::PlayKataAction(UKataAction* Asset, const FKataContext& Context, UKataActionInstance*& OutInstance)
{
    return StartResolved(Asset ? Asset->Resolve(this) : nullptr, BuildContext(Context), OutInstance);
}

EKataStartResult UKataComponent::PlayKataActionOnSelf(UKataAction* Asset, AActor* TargetActor, UKataActionInstance*& OutInstance)
{
    FKataContext Context;
    Context.TargetActor = TargetActor;
    return PlayKataAction(Asset, Context, OutInstance);
}

EKataStartResult UKataComponent::CanPlayKataAction(UKataAction* Asset, const FKataContext& Context) const
{
    return CanStartResolved(Asset ? Asset->Resolve(GetTransientPackage()) : nullptr, BuildContext(Context));
}

EKataStartResult UKataComponent::StartResolved(UKataResolvedAction* Resolved, const FKataContext& ResolvedContext, UKataActionInstance*& OutInstance)
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

    UKataActionInstance* Instance = NewObject<UKataActionInstance>(this);
    const EKataStartResult InitResult = Instance->InitializeInstance(Resolved, ResolvedContext);
    if (InitResult != EKataStartResult::Started)
    {
        return InitResult;
    }

    ActiveInstance = Instance;
    OutInstance = Instance;
    Instance->OnKataEnded.AddDynamic(this, &UKataComponent::HandleInstanceEnded);
    RegisterWithExecutionSubsystem(Instance);

    // 순간 태스크만 있는 타임라인은 StartInstance에서 바로 끝나므로 시작 알림을 먼저 보낸다.
    OnKataStarted.Broadcast(Instance);
    Instance->StartInstance();

    return EKataStartResult::Started;
}

void UKataComponent::StopKata(EKataEndReason Reason)
{
    if (IsValid(ActiveInstance))
    {
        ActiveInstance->RequestEnd(Reason);
    }
}

void UKataComponent::HandleInstanceEnded(UKataActionInstance* Instance, EKataEndReason EndReason)
{
    if (Instance == nullptr)
    {
        return;
    }

    Instance->OnKataEnded.RemoveDynamic(this, &UKataComponent::HandleInstanceEnded);
    UnregisterFromExecutionSubsystem(Instance);

    if (ActiveInstance == Instance)
    {
        ActiveInstance = nullptr;
    }

    OnKataEnded.Broadcast(Instance, EndReason);
}

void UKataComponent::RegisterWithExecutionSubsystem(UKataActionInstance* Instance)
{
    bUsesWorldExecutionSubsystem = false;
    if (!IsValid(Instance))
    {
        return;
    }
    if (UWorld* World = GetWorld())
    {
        if (UKataExecutionWorldSubsystem* Subsystem = World->GetSubsystem<UKataExecutionWorldSubsystem>())
        {
            Subsystem->RegisterInstance(Instance, ExecutionPriority);
            bUsesWorldExecutionSubsystem = true;
        }
    }
}

void UKataComponent::UnregisterFromExecutionSubsystem(UKataActionInstance* Instance)
{
    if (UWorld* World = GetWorld())
    {
        if (UKataExecutionWorldSubsystem* Subsystem = World->GetSubsystem<UKataExecutionWorldSubsystem>())
        {
            Subsystem->UnregisterInstance(Instance);
        }
    }
    bUsesWorldExecutionSubsystem = false;
}
