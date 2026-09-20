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
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Node")
    TObjectPtr<UKataAction> Action;

    virtual FText GetDescription_Implementation() const override;
};
