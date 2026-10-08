#pragma once

#include "StateTree/KataStateTreeExecutionTypes.h"
#include "StateTreeNodeBase.h"

class UKataAction;
class UKataGraph;
class UKataActionComponent;

namespace KataStateTreeExecution
{
    bool Prepare(FKataStateTreeExecutionData& Data, UKataActionComponent*& OutComponent);
    EStateTreeRunStatus PlayAction(FKataStateTreeExecutionData& Data, UKataAction* Action);
    EStateTreeRunStatus PlayGraph(FKataStateTreeExecutionData& Data, UKataGraph* Graph, FGameplayTag EntryTrigger);
    EStateTreeRunStatus Poll(FKataStateTreeExecutionData& Data);
    void Cleanup(FKataStateTreeExecutionData& Data, EKataEndReason Reason = EKataEndReason::Cancelled);

#if WITH_EDITOR
    /**
     * 노드 설명문에 쓸 입력 값 표시를 만든다. MemberName 프로퍼티가 바인딩되어 있으면 바인딩 원본 이름을,
     * 아니면 ValueText를 반환한다. 에디터 설명문 전용이며 실행 결과에 영향을 주지 않는다.
     */
    FText DescribeInput(const FGuid& ID, FName MemberName, const FText& ValueText,
        const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting);

    /** 에셋 참조의 상수 값을 설명문용 이름으로 바꾼다. 비어 있으면 None이다. */
    FText DescribeAsset(const UObject* Asset);
#endif
}
