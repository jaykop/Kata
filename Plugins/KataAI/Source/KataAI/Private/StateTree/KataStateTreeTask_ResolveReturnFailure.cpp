#include "StateTree/KataStateTreeTask_ResolveReturnFailure.h"

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "KataAILog.h"
#include "StateTreeExecutionContext.h"
#include "Targeting/KataAITargetingComponent.h"

EStateTreeRunStatus FKataStateTreeTask_ResolveReturnFailure::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    APawn* Pawn = Data.Pawn;
    UKataAITargetingComponent* Targeting = IsValid(Pawn) ? Pawn->FindComponentByClass<UKataAITargetingComponent>() : nullptr;
    if (Targeting == nullptr)
    {
        Data.AppliedPolicy = EKataAIReturnFailurePolicy::Stay;
        return EStateTreeRunStatus::Failed;
    }

    // 실패한 MoveTo 요청이 남아 순간이동 뒤 이전 경로로 끌려가지 않도록 먼저 멈춘다.
    if (AAIController* Controller = Cast<AAIController>(Pawn->GetController()))
    {
        Controller->StopMovement();
    }

    Data.AppliedPolicy = EKataAIReturnFailurePolicy::Stay;
    if (Data.Policy == EKataAIReturnFailurePolicy::Teleport)
    {
        const FVector Destination = Data.bUseReturnLocation ? Data.ReturnLocation : Targeting->GetHomeLocation();
        // 충돌 검사를 유지해 다른 액터와 겹치는 위치로는 옮기지 않는다.
        if (Pawn->TeleportTo(Destination, Pawn->GetActorRotation()))
        {
            Data.AppliedPolicy = EKataAIReturnFailurePolicy::Teleport;
        }
        else
        {
            UE_LOG(LogKataAI, Warning, TEXT("AI '%s' could not teleport to %s after return failure; staying at current location."),
                *Pawn->GetName(), *Destination.ToCompactString());
        }
    }

    // 순간이동 성공 시에도 실제 도착 위치를 기록해 충돌 보정으로 달라진 좌표를 반영한다.
    Targeting->SetHomeLocation(Pawn->GetActorLocation());
    Targeting->ClearTargetMemory();
    Targeting->ResetMoveRetryCount();
    return EStateTreeRunStatus::Succeeded;
}
