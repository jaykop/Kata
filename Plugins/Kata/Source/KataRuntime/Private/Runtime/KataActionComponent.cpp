#include "Runtime/KataActionComponent.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Action/KataAction.h"
#include "Action/KataResolvedAction.h"
#include "Engine/World.h"
#include "GAS/KataGasBridge.h"
#include "GameFramework/Actor.h"
#include "KataCondition.h"
#include "KataRuntimeLog.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Runtime/KataActionInstance.h"
#include "Runtime/KataExecutionWorldSubsystem.h"

#if KATA_WITH_ACTION_TRACE
namespace
{
    // 실행 이력의 Detail 값. 로그를 거르는 쪽이 쓰는 식별자이므로 이름을 바꾸면 manual도 함께 고친다.
    const FName KataTraceDetailCancelAll(TEXT("All"));
    const FName KataTraceDetailCancelFiltered(TEXT("Filtered"));
    const FName KataTraceDetailTransitionSourceChanged(TEXT("TransitionSourceChanged"));
    const FName KataTraceDetailBlockedByActiveTags(TEXT("BlockedByActiveTags"));
    const FName KataTraceDetailCannotInterrupt(TEXT("CannotInterrupt"));

    /** Container에서 Predicate를 만족하는 첫 태그. 없으면 빈 태그다. */
    template <typename PredicateType>
    FGameplayTag FindFirstTag(const FGameplayTagContainer& Container, PredicateType Predicate)
    {
        for (const FGameplayTag& Tag : Container)
        {
            if (Predicate(Tag))
            {
                return Tag;
            }
        }
        return FGameplayTag();
    }
}
#endif

UKataActionComponent::UKataActionComponent()
{
    // Game·PIE 월드에서는 실행 Subsystem이 인스턴스를 진행한다. 컴포넌트 Tick은 Subsystem이 없는 월드의 대체 경로이므로
    // 그런 월드에서 인스턴스가 실행되는 동안에만 켠다.
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;

    // 물리·애니메이션 갱신보다 먼저 타임라인 경계를 처리한다.
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UKataActionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bUsesWorldExecutionSubsystem && IsValid(ActiveInstance))
    {
        ActiveInstance->TickInstance(DeltaTime);
    }
}

void UKataActionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

bool UKataActionComponent::IsPlayingKata() const
{
    return IsValid(ActiveInstance) && ActiveInstance->IsRunning();
}

FGameplayTagContainer UKataActionComponent::GetActiveKataTags() const
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

FKataContext UKataActionComponent::BuildContext(const FKataContext& InContext) const
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

EKataStartResult UKataActionComponent::CanStartResolved(const UKataResolvedAction* Resolved, const FKataContext& ResolvedContext,
    const UKataActionInstance* ExpectedInstance) const
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

    // 그래프 전이는 교체될 현재 액션의 차단 정책을 우회하므로, 그 액션이 ASC에 건 차단 태그도 판정에서 뺀다.
    const UKataResolvedAction* TransitionSource = ExpectedInstance != nullptr ? ExpectedInstance->GetResolvedDefinition() : nullptr;
    const EKataStartResult BlockedResult = KataGas::CheckBlockedAbilityTags(*Resolved, *AbilitySystem, TransitionSource);
    if (BlockedResult != EKataStartResult::Started)
    {
        return BlockedResult;
    }

    if (KataGas::IsOnCooldown(*Resolved, *AbilitySystem))
    {
        return EKataStartResult::OnCooldown;
    }

    if (ExpectedInstance != nullptr && (!IsPlayingKata() || ActiveInstance != ExpectedInstance))
    {
        return EKataStartResult::BlockedByActiveKata;
    }

    if (IsPlayingKata() && ExpectedInstance == nullptr)
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

EKataStartResult UKataActionComponent::PlayKataAction(UKataAction* Asset, const FKataContext& Context, UKataActionInstance*& OutInstance)
{
    return StartResolved(Asset ? Asset->Resolve(this) : nullptr, BuildContext(Context), OutInstance);
}

EKataStartResult UKataActionComponent::PlayKataActionTransition(UKataAction* Asset, const FKataContext& Context,
    UKataActionInstance* ExpectedInstance, UKataActionInstance*& OutInstance)
{
    return StartResolved(Asset ? Asset->Resolve(this) : nullptr, BuildContext(Context), OutInstance, ExpectedInstance);
}

EKataStartResult UKataActionComponent::PlayKataActionOnSelf(UKataAction* Asset, AActor* TargetActor, UKataActionInstance*& OutInstance)
{
    FKataContext Context;
    Context.TargetActor = TargetActor;
    return PlayKataAction(Asset, Context, OutInstance);
}

EKataStartResult UKataActionComponent::CanPlayKataAction(UKataAction* Asset, const FKataContext& Context) const
{
    // 해석 비용 측정(#47)용 스코프다. 이 스코프는 해석을 포함하고, StartResolved 스코프는 호출 전에 끝난 해석을 포함하지 않는다.
    TRACE_CPUPROFILER_EVENT_SCOPE(UKataActionComponent::CanPlayKataAction);
    // 판정만 하고 결과를 버리므로 태스크·명령을 복제하지 않는 판정 전용 해석을 쓴다. 판정 결과는 Resolve와 같다.
    return CanStartResolved(Asset ? Asset->ResolveForStartCheck(GetTransientPackage()) : nullptr, BuildContext(Context));
}

EKataStartResult UKataActionComponent::StartResolved(UKataResolvedAction* Resolved, const FKataContext& ResolvedContext,
    UKataActionInstance*& OutInstance, UKataActionInstance* ExpectedInstance)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UKataActionComponent::StartResolved);
    OutInstance = nullptr;
    const EKataStartResult CheckResult = CanStartResolved(Resolved, ResolvedContext, ExpectedInstance);
    if (CheckResult != EKataStartResult::Started)
    {
#if KATA_WITH_ACTION_TRACE
        TraceStartResult(Resolved, ResolvedContext, ExpectedInstance, CheckResult, nullptr);
#endif
        return CheckResult;
    }

    UKataActionInstance* Instance = NewObject<UKataActionInstance>(this);
    const EKataStartResult InitResult = Instance->InitializeInstance(Resolved, ResolvedContext);
    if (InitResult != EKataStartResult::Started)
    {
#if KATA_WITH_ACTION_TRACE
        TraceStartResult(Resolved, ResolvedContext, ExpectedInstance, InitResult, nullptr);
#endif
        return InitResult;
    }

    // 초기화 실패도 기존 액션을 끊지 않도록 교체 직전까지 이전 인스턴스를 유지한다.
    if (IsPlayingKata())
    {
        ActiveInstance->RequestEnd(ExpectedInstance != nullptr ? EKataEndReason::Branched : EKataEndReason::Interrupted);
    }

    ActiveInstance = Instance;
    OutInstance = Instance;
    Instance->OnKataEnded.AddDynamic(this, &UKataActionComponent::HandleInstanceEnded);
    RegisterWithExecutionSubsystem(Instance);

#if KATA_WITH_ACTION_TRACE
    // 시각 0 태스크가 여는 창과 즉시 종료가 시작 기록 뒤에 오도록 StartInstance보다 먼저 남긴다.
    TraceStartResult(Resolved, ResolvedContext, ExpectedInstance, EKataStartResult::Started, Instance);
#endif

    // 시각 0 태스크가 StartInstance 안에서 액션을 끝낼 수 있으므로 시작 알림을 먼저 보낸다.
    OnKataStarted.Broadcast(Instance);
    Instance->StartInstance();

    return EKataStartResult::Started;
}

void UKataActionComponent::StopKata(EKataEndReason Reason)
{
    if (IsValid(ActiveInstance))
    {
        ActiveInstance->RequestEnd(Reason);
    }
}

bool UKataActionComponent::TryCancelKata(FGameplayTag CancelTag, bool bNewPress)
{
    if (!IsPlayingKata() || !ActiveInstance->AcceptsCancel(CancelTag, bNewPress))
    {
        return false;
    }

    ActiveInstance->RequestEnd(EKataEndReason::Cancelled);
    return true;
}

void UKataActionComponent::CancelAbilitiesAndKata(const FGameplayTagContainer& WithTags, const FGameplayTagContainer& WithoutTags,
    UGameplayAbility* IgnoreAbility, EKataEndReason EndReason)
{
    // 빈 컨테이너는 HasAny가 실패하므로 엔진 GE 컴포넌트처럼 nullptr로 넘겨 "필터 없음"을 뜻하게 한다.
    const FGameplayTagContainer* WithFilter = WithTags.IsEmpty() ? nullptr : &WithTags;
    const FGameplayTagContainer* WithoutFilter = WithoutTags.IsEmpty() ? nullptr : &WithoutTags;

#if KATA_WITH_ACTION_TRACE
    {
        // 요청을 먼저 남겨 이어지는 종료 기록이 이 요청의 결과로 읽히게 한다. 고정 필드라 필터는 첫 태그만 남긴다.
        FKataActionTraceEntry Entry;
        Entry.Event = EKataActionTraceEvent::CancelRequested;
        Entry.EndReason = EndReason;
        Entry.Detail = WithFilter == nullptr && WithoutFilter == nullptr ? KataTraceDetailCancelAll : KataTraceDetailCancelFiltered;
        Entry.Tag = WithFilter != nullptr ? WithTags.First() : FGameplayTag();
        if (IsPlayingKata())
        {
            Entry.SetAsset(ActiveInstance->GetKataAction());
            Entry.InstanceId = ActiveInstance->GetUniqueID();
        }
        AddTraceEntry(MoveTemp(Entry));
    }
#endif

    if (UAbilitySystemComponent* AbilitySystem = BuildContext(FKataContext()).ResolveAbilitySystem())
    {
        AbilitySystem->CancelAbilities(WithFilter, WithoutFilter, IgnoreAbility);
    }

    // GA가 재생하던 Kata는 위 취소로 이미 끝났을 수 있으므로 현재 인스턴스를 다시 확인한다.
    if (IsPlayingKata())
    {
        const bool bOwnedByIgnoredAbility = IgnoreAbility != nullptr
            && ActiveInstance->GetContextRef().OwningAbility.Get() == IgnoreAbility;
        const FGameplayTagContainer ActiveTags = GetActiveKataTags();
        const bool bPassesWith = WithFilter == nullptr || ActiveTags.HasAny(*WithFilter);
        const bool bPassesWithout = WithoutFilter == nullptr || !ActiveTags.HasAny(*WithoutFilter);
        if (!bOwnedByIgnoredAbility && bPassesWith && bPassesWithout)
        {
            ActiveInstance->RequestEnd(EndReason);
        }
    }

    if (WithFilter == nullptr && WithoutFilter == nullptr)
    {
        OnCancelAllRequested.Broadcast(EndReason);
    }
}

void UKataActionComponent::HandleInstanceEnded(UKataActionInstance* Instance, EKataEndReason EndReason)
{
    if (Instance == nullptr)
    {
        return;
    }

    Instance->OnKataEnded.RemoveDynamic(this, &UKataActionComponent::HandleInstanceEnded);
    UnregisterFromExecutionSubsystem(Instance);

    if (ActiveInstance == Instance)
    {
        ActiveInstance = nullptr;
    }

#if KATA_WITH_ACTION_TRACE
    {
        FKataActionTraceEntry Entry;
        Entry.Event = EKataActionTraceEvent::ActionEnded;
        Entry.InstanceId = Instance->GetUniqueID();
        Entry.SetAsset(Instance->GetKataAction());
        Entry.EndReason = EndReason;
        AddTraceEntry(MoveTemp(Entry));
    }
#endif

    OnKataEnded.Broadcast(Instance, EndReason);
}

void UKataActionComponent::RegisterWithExecutionSubsystem(UKataActionInstance* Instance)
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

    // EditorPreview처럼 Subsystem이 생성되지 않는 월드에서만 컴포넌트 Tick으로 진행한다.
    SetComponentTickEnabled(!bUsesWorldExecutionSubsystem);
}

void UKataActionComponent::UnregisterFromExecutionSubsystem(UKataActionInstance* Instance)
{
    if (UWorld* World = GetWorld())
    {
        if (UKataExecutionWorldSubsystem* Subsystem = World->GetSubsystem<UKataExecutionWorldSubsystem>())
        {
            Subsystem->UnregisterInstance(Instance);
        }
    }

    // 전이 중 종료가 미뤄진 이전 인스턴스는 새 인스턴스가 등록된 뒤에 끝날 수 있다. 그 경우 현재 인스턴스의 실행 방식을 지우지 않는다.
    if (ActiveInstance == nullptr || ActiveInstance == Instance)
    {
        bUsesWorldExecutionSubsystem = false;
        SetComponentTickEnabled(false);
    }
}

#if KATA_WITH_ACTION_TRACE
void UKataActionComponent::AddTraceEntry(FKataActionTraceEntry Entry)
{
    if (!KataActionTrace::IsEnabled())
    {
        return;
    }

    const UWorld* World = GetWorld();
    Entry.WorldSeconds = World != nullptr ? World->GetTimeSeconds() : 0.0;
    Entry.FrameNumber = GFrameCounter;
    if (TraceBuffer.Add(Entry, KataActionTrace::GetCapacity()) && KataActionTrace::IsLiveLogEnabled())
    {
        UE_LOG(LogKataActionTrace, Log, TEXT("%s"), *KataActionTrace::FormatEntry(Entry, GetOwner()));
    }
}

void UKataActionComponent::TraceStartResult(const UKataResolvedAction* Resolved, const FKataContext& ResolvedContext,
    const UKataActionInstance* ExpectedInstance, EKataStartResult Result, const UKataActionInstance* StartedInstance)
{
    if (!KataActionTrace::IsEnabled())
    {
        return;
    }

    FKataActionTraceEntry Entry;
    Entry.Event = Result == EKataStartResult::Started ? EKataActionTraceEvent::ActionStarted : EKataActionTraceEvent::ActionRejected;
    Entry.Source = ResolvedContext.OwningAbility.IsValid() ? EKataActionTraceSource::Ability
        : ExpectedInstance != nullptr ? EKataActionTraceSource::Transition
        : EKataActionTraceSource::Direct;
    Entry.StartResult = Result;
    Entry.SetAsset(Resolved != nullptr ? Resolved->SourceAction.Get() : nullptr);
    Entry.InstanceId = StartedInstance != nullptr ? StartedInstance->GetUniqueID() : 0;

    // 아래 상세는 CanStartResolved가 같은 사유를 낸 판정을 그대로 다시 따라간다. 판정 순서를 바꾸면 여기도 맞춘다.
    const UAbilitySystemComponent* AbilitySystem = Resolved != nullptr ? ResolvedContext.ResolveAbilitySystem() : nullptr;
    switch (Result)
    {
    case EKataStartResult::ResolveFailed:
        if (Resolved != nullptr)
        {
            for (const FKataDiagnostic& Diagnostic : Resolved->Diagnostics)
            {
                if (Diagnostic.Severity == EKataDiagnosticSeverity::Error)
                {
                    Entry.Detail = Diagnostic.Code;
                    break;
                }
            }
        }
        break;

    case EKataStartResult::MissingRequiredTags:
        if (AbilitySystem != nullptr)
        {
            Entry.Tag = FindFirstTag(Resolved->ActivationRequiredTags,
                [AbilitySystem](const FGameplayTag& Tag) { return !AbilitySystem->HasMatchingGameplayTag(Tag); });
        }
        break;

    case EKataStartResult::BlockedByTags:
        if (AbilitySystem != nullptr)
        {
            Entry.Tag = FindFirstTag(Resolved->ActivationBlockedTags,
                [AbilitySystem](const FGameplayTag& Tag) { return AbilitySystem->HasMatchingGameplayTag(Tag); });
        }
        break;

    case EKataStartResult::BlockedByAbilityTags:
        if (AbilitySystem != nullptr)
        {
            // KataGas::CheckBlockedAbilityTags처럼 전이에서는 교체될 액션이 건 차단 태그를 뺀다.
            FGameplayTagContainer BlockedTags = AbilitySystem->GetBlockedAbilityTags();
            if (const UKataResolvedAction* TransitionSource = ExpectedInstance != nullptr ? ExpectedInstance->GetResolvedDefinition() : nullptr)
            {
                BlockedTags.RemoveTags(TransitionSource->BlockingPolicy.BlockedAbilityTags);
            }
            Entry.Tag = FindFirstTag(Resolved->KataTags,
                [&BlockedTags](const FGameplayTag& Tag) { return Tag.MatchesAny(BlockedTags); });
        }
        break;

    case EKataStartResult::OnCooldown:
        if (AbilitySystem != nullptr)
        {
            Entry.Tag = FindFirstTag(Resolved->CooldownPolicy.GroupTags,
                [AbilitySystem](const FGameplayTag& Tag) { return AbilitySystem->HasMatchingGameplayTag(Tag); });
        }
        break;

    case EKataStartResult::BlockedByActiveKata:
        if (ExpectedInstance != nullptr && (!IsPlayingKata() || ActiveInstance != ExpectedInstance))
        {
            Entry.Detail = KataTraceDetailTransitionSourceChanged;
        }
        else if (IsPlayingKata())
        {
            const UKataResolvedAction* Active = ActiveInstance->GetResolvedDefinition();
            const FGameplayTag BlockedTag = Active != nullptr && Resolved != nullptr
                ? FindFirstTag(Resolved->KataTags,
                    [Active](const FGameplayTag& Tag) { return Tag.MatchesAny(Active->BlockingPolicy.BlockedKataTags); })
                : FGameplayTag();
            Entry.Tag = BlockedTag;
            Entry.Detail = BlockedTag.IsValid() ? KataTraceDetailBlockedByActiveTags : KataTraceDetailCannotInterrupt;
        }
        break;

    case EKataStartResult::ConditionFailed:
        if (Resolved != nullptr && Resolved->StartCondition != nullptr)
        {
            Entry.SetConditionResult(Resolved->StartCondition->Evaluate(ResolvedContext.ToConditionContext()));
        }
        break;

    default:
        break;
    }

    AddTraceEntry(MoveTemp(Entry));
}
#endif
