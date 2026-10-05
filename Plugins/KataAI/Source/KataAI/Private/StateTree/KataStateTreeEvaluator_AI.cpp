#include "StateTree/KataStateTreeEvaluator_AI.h"

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"
#include "Targeting/KataAITargetingComponent.h"

namespace
{
    void ClearOutputs(FKataStateTreeEvaluator_AIInstanceData& Data)
    {
        Data.TargetActor = nullptr;
        Data.bHasVisibleTarget = false;
        Data.bHasLastKnownLocation = false;
        Data.LastKnownLocation = FVector::ZeroVector;
        Data.HomeLocation = FVector::ZeroVector;
        Data.TimeSinceLastSeen = 0.0f;
    }
}

void FKataStateTreeEvaluator_AI::TreeStart(FStateTreeExecutionContext& Context) const
{
    Tick(Context, 0.0f);
}

void FKataStateTreeEvaluator_AI::TreeStop(FStateTreeExecutionContext& Context) const
{
    ClearOutputs(Context.GetInstanceData(*this));
}

void FKataStateTreeEvaluator_AI::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    const APawn* Pawn = IsValid(Data.AIController) ? Data.AIController->GetPawn() : nullptr;
    const UKataAITargetingComponent* Targeting = Pawn != nullptr
        ? Pawn->FindComponentByClass<UKataAITargetingComponent>() : nullptr;
    if (Targeting == nullptr)
    {
        ClearOutputs(Data);
        return;
    }
    Data.TargetActor = Targeting->GetCurrentTarget();
    Data.bHasVisibleTarget = IsValid(Data.TargetActor);
    Data.bHasLastKnownLocation = Targeting->HasLastKnownLocation();
    Data.LastKnownLocation = Data.bHasLastKnownLocation ? Targeting->GetLastKnownLocation() : FVector::ZeroVector;
    Data.HomeLocation = Targeting->GetHomeLocation();
    Data.TimeSinceLastSeen = Targeting->GetTimeSinceLastSeen();
}
