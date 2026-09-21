#include "SKataFindInGraph.h"

#include "Action/KataAction.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "GraphEditor.h"
#include "KataActionNode.h"
#include "KataCondition.h"
#include "KataEdNode.h"
#include "KataEdNodeEdge.h"
#include "KataEdge.h"
#include "KataNode.h"

namespace
{
	/** 기반 클래스가 검사 문자열에서 공백을 지우므로 같은 규칙을 따른다. */
	void AppendSearchText(FString& InOutSearchString, const FString& Text)
	{
		if (!Text.IsEmpty())
		{
			InOutSearchString += Text;
		}
	}

	/** 태그 이름을 검사 문자열에 더한다. 비어 있는 태그는 건너뛴다. */
	void AppendTag(FString& InOutSearchString, const FGameplayTag& Tag)
	{
		if (Tag.IsValid())
		{
			AppendSearchText(InOutSearchString, Tag.GetTagName().ToString());
		}
	}

	/** 인스턴스 객체의 클래스 이름을 검사 문자열에 더한다. */
	void AppendClassName(FString& InOutSearchString, const UObject* Object)
	{
		if (Object != nullptr)
		{
			AppendSearchText(InOutSearchString, Object->GetClass()->GetName());
		}
	}

	/** 노드에서 Kata 고유 검사 문자열을 모은다. */
	FString BuildKataSearchString(const UEdGraphNode* Node)
	{
		FString SearchString;

		if (const UKataEdNode* EdNode = Cast<UKataEdNode>(Node))
		{
			// 노드 종류로 찾을 수 있게 한다. Entry, Action 등으로 걸린다.
			AppendClassName(SearchString, EdNode->KataNode);

			if (const UKataNode* KataNode = Cast<UKataNode>(EdNode->KataNode))
			{
				AppendClassName(SearchString, KataNode->EntryCondition);
			}

			if (const UKataActionNode* ActionNode = Cast<UKataActionNode>(EdNode->KataNode))
			{
				if (const UKataAction* Action = ActionNode->Action)
				{
					// 이름은 노드 제목과 겹치지만, 경로는 폴더로 좁혀 찾을 때 쓴다.
					AppendSearchText(SearchString, Action->GetName());
					AppendSearchText(SearchString, Action->GetPathName());
				}
			}
		}
		else if (const UKataEdNodeEdge* EdNodeEdge = Cast<UKataEdNodeEdge>(Node))
		{
			AppendClassName(SearchString, EdNodeEdge->KataEdge);

			if (const UKataEdge* Edge = Cast<UKataEdge>(EdNodeEdge->KataEdge))
			{
				// 제목은 직접 지은 이름으로 덮을 수 있으므로 태그를 따로 검사한다.
				AppendTag(SearchString, Edge->TriggerTag);
				AppendTag(SearchString, Edge->RequiredWindowTag);
				AppendClassName(SearchString, Edge->Condition);
				if (const UEnum* TimingEnum = StaticEnum<EKataTransitionTiming>())
				{
					AppendSearchText(SearchString, TimingEnum->GetNameStringByValue(static_cast<int64>(Edge->Timing)));
				}
			}
		}

		return SearchString.Replace(TEXT(" "), TEXT(""));
	}
}

void FKataFindInGraphResult::JumpToNode(TWeakPtr<FAssetEditorToolkit> AssetEditorToolkit, const UEdGraphNode* InNode) const
{
	if (InNode == nullptr)
	{
		return;
	}

	// 편집기 종류를 캐스팅하지 않고 노드가 속한 그래프에서 화면을 찾는다.
	if (const TSharedPtr<SGraphEditor> GraphEditor = SGraphEditor::FindGraphEditorForGraph(InNode->GetGraph()))
	{
		GraphEditor->JumpToNode(InNode, false, true);
	}
}

void SKataFindInGraph::Construct(const FArguments& InArgs, TSharedPtr<FAssetEditorToolkit> InAssetEditorToolkit, UEdGraph* InGraph)
{
	SearchedGraph = InGraph;

	SFindInGraph::Construct(InArgs, InAssetEditorToolkit);
}

TSharedPtr<FFindInGraphResult> SKataFindInGraph::MakeSearchResult(const FFindInGraphResult::FCreateParams& InParams)
{
	return MakeShared<FKataFindInGraphResult>(InParams);
}

const UEdGraph* SKataFindInGraph::GetGraph()
{
	return SearchedGraph.Get();
}

bool SKataFindInGraph::MatchTokensInNode(const UEdGraphNode* Node, const TArray<FString>& Tokens)
{
	if (Node == nullptr)
	{
		return false;
	}

	const FString SearchString = BuildKataSearchString(Node);
	return !SearchString.IsEmpty() && StringMatchesSearchTokens(Tokens, SearchString);
}
