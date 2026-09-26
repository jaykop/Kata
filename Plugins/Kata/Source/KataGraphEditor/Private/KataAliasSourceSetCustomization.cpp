#include "KataAliasSourceSetCustomization.h"

#include "DetailWidgetRow.h"
#include "EdGraph/EdGraph.h"
#include "GraphEditAction.h"
#include "IDetailChildrenBuilder.h"
#include "KataAliasNode.h"
#include "KataEdNode.h"
#include "KataGraphBase.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "KataAliasSourceSetCustomization"

namespace
{
	/**
	 * 별칭이 속한 편집기 그래프를 찾는다.
	 *
	 * UKataGraphBase::AllNodes는 저장 시점의 RebuildKataGraph에서 채우므로 편집 중에는 뒤처져 있다.
	 * 방금 만든 노드까지 보려면 편집기 그래프를 직접 훑어야 한다.
	 */
	UEdGraph* FindOwningEdGraph(const UKataAliasNode* AliasNode)
	{
		if (AliasNode == nullptr)
		{
			return nullptr;
		}

		if (const UKataGraphBase* OwningGraph = AliasNode->GetGraph())
		{
			if (OwningGraph->EdGraph != nullptr)
			{
				return OwningGraph->EdGraph;
			}
		}

		// 그래프를 다시 만들기 전에는 노드의 Outer가 아직 편집기 노드다.
		return AliasNode->GetTypedOuter<UEdGraph>();
	}
}

FKataAliasSourceNodeBuilder::FKataAliasSourceNodeBuilder(TSharedPtr<IPropertyHandle> InNodesHandle,
	TArray<TWeakObjectPtr<UKataAliasNode>> InEditedAliases)
	: NodesHandle(MoveTemp(InNodesHandle))
	, EditedAliases(MoveTemp(InEditedAliases))
{
}

FKataAliasSourceNodeBuilder::~FKataAliasSourceNodeBuilder()
{
	if (UEdGraph* Graph = ObservedGraph.Get())
	{
		Graph->RemoveOnGraphChangedHandler(GraphChangedHandle);
	}
}

void FKataAliasSourceNodeBuilder::SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren)
{
	OnRegenerateChildren = InOnRegenerateChildren;
}

void FKataAliasSourceNodeBuilder::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	UEdGraph* EdGraph = EditedAliases.Num() > 0 ? FindOwningEdGraph(EditedAliases[0].Get()) : nullptr;
	if (EdGraph == nullptr)
	{
		ChildrenBuilder.AddCustomRow(LOCTEXT("NoGraphFilter", "Source Nodes"))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoGraph", "Graph not found."))
			.Font(IPropertyTypeCustomizationUtils::GetRegularFont())
		];
		return;
	}

	// 생성자에서는 AsShared를 쓸 수 없어 목록을 처음 만들 때 알림을 건다.
	if (!ObservedGraph.IsValid())
	{
		ObservedGraph = EdGraph;
		GraphChangedHandle = EdGraph->AddOnGraphChangedHandler(
			FOnGraphChanged::FDelegate::CreateSP(this, &FKataAliasSourceNodeBuilder::OnGraphChanged));
	}

	ChildrenBuilder.AddCustomRow(LOCTEXT("SourceNodesFilter", "Source Nodes"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SourceNodesLabel", "Source Nodes"))
			.Font(IPropertyTypeCustomizationUtils::GetBoldFont())
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Text(this, &FKataAliasSourceNodeBuilder::GetSummaryText)
			.Font(IPropertyTypeCustomizationUtils::GetRegularFont())
		];

	int32 CandidateCount = 0;
	for (UEdGraphNode* EdGraphNode : EdGraph->Nodes)
	{
		const UKataEdNode* KataEdNode = Cast<UKataEdNode>(EdGraphNode);
		if (KataEdNode == nullptr)
		{
			continue;
		}

		UKataNode* Candidate = Cast<UKataNode>(KataEdNode->KataNode);
		// 머무를 수 있는 노드만 출발지가 된다. 경유 노드와 다른 별칭은 현재 노드가 될 수 없다.
		if (Candidate == nullptr || !Candidate->IsExecutableState())
		{
			continue;
		}

		const FText NodeTitle = KataEdNode->GetNodeTitle(ENodeTitleType::ListView);
		++CandidateCount;

		ChildrenBuilder.AddCustomRow(NodeTitle)
			.NameContent()
			[
				SNew(STextBlock)
				.Text(NodeTitle)
				.Font(IPropertyTypeCustomizationUtils::GetRegularFont())
			]
			.ValueContent()
			[
				SNew(SCheckBox)
				.IsChecked(this, &FKataAliasSourceNodeBuilder::GetNodeCheckState,
					TWeakObjectPtr<UKataNode>(Candidate))
				.OnCheckStateChanged(this, &FKataAliasSourceNodeBuilder::OnNodeCheckStateChanged,
					TWeakObjectPtr<UKataNode>(Candidate))
			];
	}

	if (CandidateCount == 0)
	{
		ChildrenBuilder.AddCustomRow(LOCTEXT("NoCandidateFilter", "Source Nodes"))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoCandidate", "This graph has no node that can be a source."))
			.Font(IPropertyTypeCustomizationUtils::GetRegularFont())
		];
	}
}

ECheckBoxState FKataAliasSourceNodeBuilder::GetNodeCheckState(TWeakObjectPtr<UKataNode> Node) const
{
	UKataNode* TargetNode = Node.Get();
	if (TargetNode == nullptr)
	{
		return ECheckBoxState::Unchecked;
	}

	bool bAnyChecked = false;
	bool bAnyUnchecked = false;
	for (const TWeakObjectPtr<UKataAliasNode>& Alias : EditedAliases)
	{
		if (!Alias.IsValid())
		{
			continue;
		}
		if (Alias->SourceNodes.Nodes.Contains(TargetNode))
		{
			bAnyChecked = true;
		}
		else
		{
			bAnyUnchecked = true;
		}
	}

	// 여러 별칭을 함께 편집할 때 값이 갈리면 중간 상태로 보여 준다.
	if (bAnyChecked && bAnyUnchecked)
	{
		return ECheckBoxState::Undetermined;
	}
	return bAnyChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FKataAliasSourceNodeBuilder::OnNodeCheckStateChanged(ECheckBoxState NewState, TWeakObjectPtr<UKataNode> Node)
{
	UKataNode* TargetNode = Node.Get();
	if (TargetNode == nullptr)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetAliasSource", "Set Kata Alias Source"));
	if (NodesHandle.IsValid())
	{
		NodesHandle->NotifyPreChange();
	}

	const bool bShouldContain = NewState == ECheckBoxState::Checked;
	for (const TWeakObjectPtr<UKataAliasNode>& Alias : EditedAliases)
	{
		if (!Alias.IsValid())
		{
			continue;
		}
		Alias->Modify();
		if (bShouldContain)
		{
			Alias->SourceNodes.Nodes.AddUnique(TargetNode);
		}
		else
		{
			Alias->SourceNodes.Nodes.Remove(TargetNode);
		}
	}

	if (NodesHandle.IsValid())
	{
		NodesHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
		NodesHandle->NotifyFinishedChangingProperties();
	}
}

void FKataAliasSourceNodeBuilder::OnGraphChanged(const FEdGraphEditAction& EditAction)
{
	// 후보 목록이 달라지는 변경만 본다. 선택만 바뀐 경우까지 다시 만들면 낭비다.
	// GRAPHACTION_Default는 값이 0이라 비트 검사로 잡히지 않으므로 따로 비교한다.
	const bool bNodeSetChanged = EditAction.Action == GRAPHACTION_Default
		|| (EditAction.Action & (GRAPHACTION_AddNode | GRAPHACTION_RemoveNode)) != 0;
	if (!bNodeSetChanged)
	{
		return;
	}

	// 이 목록만 다시 만든다. 디테일 패널 전체를 다시 그리지 않으므로 펼쳐 둔 상태가 유지된다.
	OnRegenerateChildren.ExecuteIfBound();
}

FText FKataAliasSourceNodeBuilder::GetSummaryText() const
{
	if (EditedAliases.Num() == 0 || !EditedAliases[0].IsValid())
	{
		return FText::GetEmpty();
	}
	return FText::Format(LOCTEXT("SelectedCount", "{0} selected"),
		FText::AsNumber(EditedAliases[0]->SourceNodes.Nodes.Num()));
}

TSharedRef<IPropertyTypeCustomization> FKataAliasSourceSetCustomization::MakeInstance()
{
	return MakeShared<FKataAliasSourceSetCustomization>();
}

void FKataAliasSourceSetCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	NodesHandle = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FKataAliasSourceSet, Nodes));

	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);
	for (UObject* OuterObject : OuterObjects)
	{
		if (UKataAliasNode* AliasNode = Cast<UKataAliasNode>(OuterObject))
		{
			EditedAliases.Add(AliasNode);
		}
	}

	// HeaderRow를 채우지 않는다. 내용이 없으면 이 속성의 행이 숨고 목록이 카테고리 바로 아래로 붙는다.
}

void FKataAliasSourceSetCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	ChildBuilder.AddCustomBuilder(MakeShared<FKataAliasSourceNodeBuilder>(NodesHandle, EditedAliases));
}

#undef LOCTEXT_NAMESPACE
