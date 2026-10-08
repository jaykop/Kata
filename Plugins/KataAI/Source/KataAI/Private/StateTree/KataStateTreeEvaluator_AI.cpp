#include "StateTree/KataStateTreeEvaluator_AI.h"

#include "AIController.h"
#include "Data/KataAIData.h"
#include "GameFramework/Pawn.h"
#include "KataAIPawnInterface.h"
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
        Data.MaxMoveRetries = 0;
        Data.MoveRetryInterval = 1.0f;
        Data.LeashDistance = 0.0f;
    }

    void ReadMovementFailureSettings(FKataStateTreeEvaluator_AIInstanceData& Data, const APawn* Pawn)
    {
        const IKataAIPawnInterface* Settings = Cast<IKataAIPawnInterface>(Pawn);
        const UKataAIData* AIData = Settings != nullptr ? Settings->GetKataAIData() : nullptr;
        if (AIData == nullptr)
        {
            return;
        }
        Data.MaxMoveRetries = FMath::Max(AIData->MaxMoveRetries, 0);
        // 잘못 저장된 값이 들어와도 Delay가 0초가 되어 매 프레임 재시도하지 않게 한다.
        Data.MoveRetryInterval = AIData->MoveRetryInterval > 0.0f ? AIData->MoveRetryInterval : 1.0f;
        Data.LeashDistance = FMath::Max(AIData->LeashDistance, 0.0f);
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
    ReadMovementFailureSettings(Data, Pawn);
}
