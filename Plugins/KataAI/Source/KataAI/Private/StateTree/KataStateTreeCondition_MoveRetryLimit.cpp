#include "StateTree/KataStateTreeCondition_MoveRetryLimit.h"

#include "GameFramework/Pawn.h"
#include "StateTree/KataStateTreeExecutionUtils.h"
#include "StateTreeExecutionContext.h"
#include "Targeting/KataAITargetingComponent.h"

bool FKataStateTreeCondition_MoveRetryLimit::TestCondition(FStateTreeExecutionContext& Context) const
{
    const FInstanceDataType& Data = Context.GetInstanceData(*this);
    const UKataAITargetingComponent* Targeting = IsValid(Data.Pawn)
        ? Data.Pawn->FindComponentByClass<UKataAITargetingComponent>() : nullptr;
    const bool bReached = Targeting != nullptr && Data.MaxMoveRetries > 0
        && Targeting->GetMoveRetryCount() >= Data.MaxMoveRetries;
    return bReached ^ bInvert;
}

#if WITH_EDITOR
FText FKataStateTreeCondition_MoveRetryLimit::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
    const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
    const FInstanceDataType* Data = InstanceDataView.GetPtr<FInstanceDataType>();
    check(Data);
    // 바인딩이 없을 때는 상수 상한을 보이고, 0은 무제한임을 함께 표시한다.
    const FText LimitText = Data->MaxMoveRetries > 0
        ? FText::AsNumber(Data->MaxMoveRetries)
        : NSLOCTEXT("KataAI", "MoveRetryUnlimited", "0 (Unlimited)");
    const FText Value = KataStateTreeExecution::DescribeInput(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, MaxMoveRetries),
        LimitText, BindingLookup, Formatting);
    return bInvert
        ? FText::Format(NSLOCTEXT("KataAI", "MoveRetryLimitNotReachedDescription", "Move Retry Limit Not Reached ({0})"), Value)
        : FText::Format(NSLOCTEXT("KataAI", "MoveRetryLimitReachedDescription", "Move Retry Limit Reached ({0})"), Value);
}
#endif
