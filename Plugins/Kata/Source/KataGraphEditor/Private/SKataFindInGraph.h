#pragma once

#include "CoreMinimal.h"
#include "FindInGraph.h"
#include "UObject/WeakObjectPtr.h"

class UEdGraph;

/** 검색 결과 항목. 클릭하면 그래프 화면을 해당 노드로 옮긴다. */
class FKataFindInGraphResult : public FFindInGraphResult
{
public:
	explicit FKataFindInGraphResult(const FCreateParams& InCreateParams)
		: FFindInGraphResult(InCreateParams)
	{
	}

	virtual void JumpToNode(TWeakPtr<FAssetEditorToolkit> AssetEditorToolkit, const UEdGraphNode* InNode) const override;
};

/**
 * Kata 그래프 전용 노드 검색 패널.
 *
 * 노드 제목과 주석, 핀은 기반 클래스가 검사한다. 여기서는 Kata 고유 정보를 더해
 * 액션 에셋 이름과 경로, 노드 종류, 전이의 트리거 태그와 창 태그로도 찾을 수 있게 한다.
 * 전이의 제목은 트리거 태그를 그대로 쓰지만 직접 지은 제목으로 덮을 수 있어,
 * 태그는 제목과 별개로 검사한다.
 */
class SKataFindInGraph : public SFindInGraph
{
public:
	/** 검색 대상 그래프를 직접 받는다. 편집기 종류에 의존하지 않기 위한 것이다. */
	void Construct(const FArguments& InArgs, TSharedPtr<FAssetEditorToolkit> InAssetEditorToolkit, UEdGraph* InGraph);

protected:
	virtual TSharedPtr<FFindInGraphResult> MakeSearchResult(const FFindInGraphResult::FCreateParams& InParams) override;
	virtual const UEdGraph* GetGraph() override;
	virtual bool MatchTokensInNode(const UEdGraphNode* Node, const TArray<FString>& Tokens) override;

private:
	TWeakObjectPtr<UEdGraph> SearchedGraph;
};
