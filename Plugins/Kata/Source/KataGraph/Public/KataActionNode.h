#pragma once

#include "CoreMinimal.h"
#include "KataNode.h"
#include "KataActionNode.generated.h"

class UKataAction;

/** 액션 하나를 실행하는 노드. 콤보의 각 단계가 이 노드다. */
UCLASS(BlueprintType, meta = (DisplayName = "Kata Action Node"))
class KATAGRAPH_API UKataActionNode : public UKataNode
{
    GENERATED_BODY()

public:
    UKataActionNode();

    /** 이 노드에 진입하면 실행할 액션. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Node",
        meta = (ToolTip = "Kata Action executed when this node becomes active. The graph node title follows the asset name."))
    TObjectPtr<UKataAction> Action;

    virtual FText GetDescription_Implementation() const override;

#if WITH_EDITOR
    /** 액션 에셋 이름을 항상 노드 제목으로 사용해 할당 변경과 리네임을 즉시 반영한다. */
    virtual FText GetNodeTitle() const override;
    virtual bool IsNameEditable() const override { return false; }
#endif
};
