#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "KataGraphNodeBase.h"
#include "KataEdNode.generated.h"

class UKataEdNodeEdge;
class UKataEdGraph;
class SKataEdNode;
class UKataGraph;

UCLASS(MinimalAPI)
class UKataEdNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UKataEdNode();
	virtual ~UKataEdNode();

	UPROPERTY(VisibleAnywhere, Instanced, Category = "KataGraph")
	UKataGraphNodeBase* KataNode;

    /** 텍스트 클립보드에만 담는 내장 저작 사본. 내보내기와 붙여넣기가 끝나면 비운다. */
    UPROPERTY(Instanced)
    TObjectPtr<UKataGraph> ClipboardSubGraph;

    /** 함께 복사한 SubGraph 노드와 포트를 같은 저작 사본으로 연결하기 위한 원본 식별자. */
    UPROPERTY()
    FString ClipboardSubGraphSource;

	void SetKataNode(UKataGraphNodeBase* InNode);
	UKataEdGraph* GetKataEdGraph();

	SKataEdNode* SEdNode;

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PrepareForCopying() override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;

	virtual FLinearColor GetBackgroundColor() const;
	virtual UEdGraphPin* GetInputPin() const;
	virtual UEdGraphPin* GetOutputPin() const;

#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif

};
