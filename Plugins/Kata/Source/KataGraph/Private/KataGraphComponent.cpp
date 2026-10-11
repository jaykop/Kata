#include "KataGraphComponent.h"

#include "GameFramework/Actor.h"
#include "KataGraph.h"
#include "KataGraphInstance.h"
#include "Runtime/KataActionComponent.h"

UKataGraphComponent::UKataGraphComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UKataGraphComponent::BeginPlay()
{
    Super::BeginPlay();

    if (UKataActionComponent* ActionComponent = ResolveActionComponent())
    {
        CancelAllSource = ActionComponent;
        CancelAllHandle = ActionComponent->OnCancelAllRequested.AddUObject(this, &UKataGraphComponent::HandleCancelAllRequested);
    }
}

void UKataGraphComponent::HandleCancelAllRequested(EKataEndReason Reason)
{
    // 액션을 재생하던 그래프는 액션 종료와 함께 이미 끝났다. 남는 것은 첫 액션을 기다리던 그래프다.
    StopGraph(Reason);
}

void UKataGraphComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UKataActionComponent* ActionComponent = CancelAllSource.Get())
    {
        ActionComponent->OnCancelAllRequested.Remove(CancelAllHandle);
    }
    CancelAllSource.Reset();
    CancelAllHandle.Reset();

    if (IsRunningGraph())
    {
        ActiveGraphInstance->RequestEnd(EKataEndReason::OwnerInvalid);
    }
    ActiveGraphInstance = nullptr;
    Super::EndPlay(EndPlayReason);
}

bool UKataGraphComponent::StartGraph(
    UKataGraph* Graph, const FKataContext& Context, UKataGraphInstance*& OutInstance)
{
    OutInstance = nullptr;
    UKataActionComponent* ActionComponent = ResolveActionComponent();
    if (!IsValid(Graph) || !IsValid(ActionComponent))
    {
        return false;
    }

    if (IsRunningGraph())
    {
        ActiveGraphInstance->RequestEnd(EKataEndReason::Interrupted);
    }

    FKataContext ResolvedContext = Context;
    if (!ResolvedContext.OwnerActor.IsValid())
    {
        ResolvedContext.OwnerActor = GetOwner();
    }
    if (!ResolvedContext.AvatarActor.IsValid())
    {
        ResolvedContext.AvatarActor = ResolvedContext.OwnerActor;
    }

    UKataGraphInstance* Instance = NewObject<UKataGraphInstance>(this);
    if (!Instance->InitializeInstance(Graph, ActionComponent, ResolvedContext))
    {
        return false;
    }

    ActiveGraphInstance = Instance;
    OutInstance = Instance;
    return true;
}

bool UKataGraphComponent::StartGraphOnSelf(
    UKataGraph* Graph, AActor* TargetActor, UKataGraphInstance*& OutInstance)
{
    FKataContext Context;
    Context.OwnerActor = GetOwner();
    Context.AvatarActor = GetOwner();
    Context.TargetActor = TargetActor;
    return StartGraph(Graph, Context, OutInstance);
}

bool UKataGraphComponent::SendTrigger(FGameplayTag TriggerTag)
{
    return IsRunningGraph() && ActiveGraphInstance->SendTrigger(TriggerTag);
}

void UKataGraphComponent::StopGraph(EKataEndReason Reason)
{
    if (IsRunningGraph())
    {
        ActiveGraphInstance->RequestEnd(Reason);
    }
}

bool UKataGraphComponent::IsRunningGraph() const
{
    return IsValid(ActiveGraphInstance) && ActiveGraphInstance->IsRunning();
}

UKataActionComponent* UKataGraphComponent::ResolveActionComponent() const
{
    AActor* Owner = GetOwner();
    return Owner != nullptr ? Owner->FindComponentByClass<UKataActionComponent>() : nullptr;
}
