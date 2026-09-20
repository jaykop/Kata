#include "KataGraphComponent.h"

#include "GameFramework/Actor.h"
#include "KataGraph.h"
#include "KataGraphInstance.h"
#include "Runtime/KataComponent.h"

UKataGraphComponent::UKataGraphComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UKataGraphComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
    UKataComponent* ActionComponent = ResolveKataComponent();
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

UKataComponent* UKataGraphComponent::ResolveKataComponent() const
{
    AActor* Owner = GetOwner();
    return Owner != nullptr ? Owner->FindComponentByClass<UKataComponent>() : nullptr;
}
