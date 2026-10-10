#pragma once

#include "AnimGraphNode_SkeletalControlBase.h"
#include "Animation/AnimNode_KataTilt.h"
#include "CoreMinimal.h"
#include "AnimGraphNode_KataTilt.generated.h"

/**
 * FAnimNode_KataTilt의 AnimGraph 편집기 노드. 노드 메뉴에 "Kata Tilt"로 나타난다.
 * 본 체인은 노드가 아니라 UKataAnimInstance의 Class Defaults에서 정하므로 이 노드에는 본 선택 항목이 없다.
 */
UCLASS(meta = (Keywords = "Kata Tilt Target Pitch Aim"))
class KATAFRAMEWORKANIMGRAPH_API UAnimGraphNode_KataTilt : public UAnimGraphNode_SkeletalControlBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Settings")
    FAnimNode_KataTilt Node;

    //~ Begin UEdGraphNode Interface
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FText GetTooltipText() const override;
    //~ End UEdGraphNode Interface

protected:
    //~ Begin UAnimGraphNode_SkeletalControlBase Interface
    virtual FText GetControllerDescription() const override;
    virtual const FAnimNode_SkeletalControlBase* GetNode() const override { return &Node; }
    //~ End UAnimGraphNode_SkeletalControlBase Interface
};
