#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "KataGraphNodeBase.h"
#include "KataEdNode.generated.h"

class UKataEdNodeEdge;
class UKataEdGraph;
class SKataEdNode;
class UKataGraph;

/** 그래프 디버거가 노드에 표시하는 실행 상태. 화면 표시 전용이며 저장하지 않는다. */
enum class EKataGraphDebugHighlight : uint8
{
	None,
	/** 디버그 대상이 이 노드를 실행 중이다. */
	Active,
	/** 디버그 대상이 이 포트가 가리키는 SubGraph 안을 실행 중이다. */
	ActiveInside,
	/** 현재 액션이 끝나면 이 노드로 전이하도록 예약됐다. */
	Pending,
	/** 예약된 대상이 이 포트가 가리키는 SubGraph 안에 있다. */
	PendingInside
};

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

	/** FKataGraphDebugger가 매 프레임 갱신한다. 리플렉션 대상이 아니므로 Undo와 저장에 포함되지 않는다. */
	EKataGraphDebugHighlight DebugHighlight = EKataGraphDebugHighlight::None;

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
