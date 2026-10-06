#pragma once

#include "CoreMinimal.h"
#include "KataSubGraphPortNode.h"
#include "KataSubGraphNode.generated.h"

/**
 * 내장 그래프의 생성·편집 진입점을 제공하는 노드.
 *
 * 편집기에서 배치하면 부모가 소유한 원본을 함께 만든다. 입력은 내부 진입, 출력은 내부 이탈을 뜻한다.
 * 원본은 부모 그래프가 소유한다. 마지막 저작 참조를 삭제하면 원본도 제거하고 Undo로 함께 복구한다.
 * 노드 복사·붙여넣기는 내부 저작 내용까지 새 원본으로 복제한다. 추가 참조는 Port In/Out으로 만든다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata SubGraph"))
class KATAGRAPH_API UKataSubGraphNode : public UKataSubGraphPortNode
{
    GENERATED_BODY()

public:
    UKataSubGraphNode();

    virtual FText GetDescription_Implementation() const override;
    virtual bool CanBeAliasSource() const override { return true; }

#if WITH_EDITOR
    /** 내장 모드의 유효한 원본 표시 이름을 노드에서 편집할 수 있다. */
    virtual bool IsNameEditable() const override;
#endif
};
