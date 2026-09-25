#include "Targeting/Tasks/KataTargetingSortTask_Weighted.h"

#include "Types/TargetingSystemTypes.h"

UKataTargetingSortTask_Weighted::UKataTargetingSortTask_Weighted(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

float UKataTargetingSortTask_Weighted::GetRawScore(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
    return 0.0f;
}

void UKataTargetingSortTask_Weighted::Execute(const FTargetingRequestHandle& TargetingHandle) const
{
    Super::Execute(TargetingHandle);
    SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Executing);

    FTargetingDefaultResultsSet* ResultData = TargetingHandle.IsValid() ? FTargetingDefaultResultsSet::Find(TargetingHandle) : nullptr;
    if (ResultData != nullptr && ResultData->TargetResults.Num() > 0)
    {
        TArray<float> RawScores;
        RawScores.Reserve(ResultData->TargetResults.Num());
        float HighestScore = 0.0f;
        for (const FTargetingDefaultResultData& TargetResult : ResultData->TargetResults)
        {
            const float RawScore = FMath::Max(0.0f, GetRawScore(TargetingHandle, TargetResult));
            RawScores.Add(RawScore);
            HighestScore = FMath::Max(HighestScore, RawScore);
        }

        // 엔진 규칙과 같게 누적 점수가 작을수록 앞선다. 높을수록 좋은 점수는 부호를 뒤집어 더한다.
        const float Direction = bHigherIsBetter ? -1.0f : 1.0f;
        if (HighestScore > UE_SMALL_NUMBER)
        {
            for (int32 Index = 0; Index < ResultData->TargetResults.Num(); ++Index)
            {
                ResultData->TargetResults[Index].Score += (RawScores[Index] / HighestScore) * Weight * Direction;
            }
        }

        auto ByScore = [](const FTargetingDefaultResultData& Lhs, const FTargetingDefaultResultData& Rhs)
        {
            return Lhs.Score < Rhs.Score;
        };
        if (bStableSort)
        {
            ResultData->TargetResults.StableSort(ByScore);
        }
        else
        {
            ResultData->TargetResults.Sort(ByScore);
        }
    }

    SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
}
