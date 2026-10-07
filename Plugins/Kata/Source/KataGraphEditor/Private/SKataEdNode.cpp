#include "SKataEdNode.h"
#include "KataGraphEditorPrivate.h"
#include "KataGraphColors.h"
#include "SLevelOfDetailBranchNode.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "SCommentBubble.h"
#include "SlateOptMacros.h"
#include "SGraphPin.h"
#include "GraphEditorSettings.h"
#include "KataEdNode.h"
#include "KataEdGraph.h"
#include "KataGraphBase.h"
#include "KataGraph.h"
#include "KataSubGraphNode.h"
#include "KataEmbeddedSubGraphEditor.h"
#include "KataGraphDragConnection.h"

#define LOCTEXT_NAMESPACE "EdNode_KataGraph"

//////////////////////////////////////////////////////////////////////////
class SKataGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SKataGraphPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin)
	{
		this->SetCursor(EMouseCursor::Default);

		bShowLabel = true;

		GraphPinObj = InPin;
		check(GraphPinObj != nullptr);

		const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
		check(Schema);

		SBorder::Construct(SBorder::FArguments()
			.BorderImage(this, &SKataGraphPin::GetPinBorder)
			.BorderBackgroundColor(this, &SKataGraphPin::GetPinColor)
			.OnMouseButtonDown(this, &SKataGraphPin::OnPinMouseDown)
			.Cursor(this, &SKataGraphPin::GetPinCursor)
			.Padding(FMargin(5.0f))
		);
	}

protected:
	virtual FSlateColor GetPinColor() const override
	{
		return KataGraphColors::Pin::Default;
	}

	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override
	{
		return SNew(STextBlock);
	}

	const FSlateBrush* GetPinBorder() const
	{
		return FAppStyle::GetBrush(TEXT("Graph.StateNode.Body"));
	}

	virtual TSharedRef<FDragDropOperation> SpawnPinDragEvent(const TSharedRef<class SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphPin> >& InStartingPins) override
	{
		FKataGraphDragConnection::FDraggedPinTable PinHandles;
		PinHandles.Reserve(InStartingPins.Num());
		// since the graph can be refreshed and pins can be reconstructed/replaced 
		// behind the scenes, the DragDropOperation holds onto FGraphPinHandles 
		// instead of direct widgets/graph-pins
		for (const TSharedRef<SGraphPin>& PinWidget : InStartingPins)
		{
			PinHandles.Add(PinWidget->GetPinObj());
		}

		return FKataGraphDragConnection::New(InGraphPanel, PinHandles);
	}

};


//////////////////////////////////////////////////////////////////////////
void SKataEdNode::Construct(const FArguments& InArgs, UKataEdNode* InNode)
{
	GraphNode = InNode;
	UpdateGraphNode();
	InNode->SEdNode = this;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SKataEdNode::UpdateGraphNode()
{
	const FMargin NodePadding = FMargin(5);
	const FMargin NamePadding = FMargin(2);

	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	const FSlateBrush *NodeTypeIcon = GetNameIcon();

	FLinearColor TitleShadowColor(0.6f, 0.6f, 0.6f);
	TSharedPtr<SErrorText> ErrorText;
	TSharedPtr<SVerticalBox> NodeBody;
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
			.Padding(0.0f)
			.BorderBackgroundColor(this, &SKataEdNode::GetBorderBackgroundColor)
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SVerticalBox)

					// Input Pin Area
					+ SVerticalBox::Slot()
					.FillHeight(1)
					[
						SAssignNew(LeftNodeBox, SVerticalBox)
					]

					// Output Pin Area	
					+ SVerticalBox::Slot()
					.FillHeight(1)
					[
						SAssignNew(RightNodeBox, SVerticalBox)
					]
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(8.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("Graph.StateNode.ColorSpill"))
					.BorderBackgroundColor(TitleShadowColor)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.Padding(6.0f)
					[
						SAssignNew(NodeBody, SVerticalBox)
									
						// Title
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)

							// Error message
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SAssignNew(ErrorText, SErrorText)
								.BackgroundColor(this, &SKataEdNode::GetErrorColor)
								.ToolTipText(this, &SKataEdNode::GetErrorMsgToolTip)
							]

							// Icon
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
								.Image(NodeTypeIcon)
							]
										
							// Node Title
							+ SHorizontalBox::Slot()
							.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SAssignNew(InlineEditableText, SInlineEditableTextBlock)
									.Style(FAppStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
									.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
									.OnVerifyTextChanged(this, &SKataEdNode::OnVerifyNameTextChanged)
									.OnTextCommitted(this, &SKataEdNode::OnNameTextCommited)
									.IsReadOnly(this, &SKataEdNode::IsNameReadOnly)
									.IsSelected(this, &SKataEdNode::IsSelectedExclusively)
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									NodeTitle.ToSharedRef()
								]
							]
						]					
					]
				]
			]
		];

	// Create comment bubble
	TSharedPtr<SCommentBubble> CommentBubble;
	const FSlateColor CommentColor = GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor;

	SAssignNew(CommentBubble, SCommentBubble)
		.GraphNode(GraphNode)
		.Text(this, &SGraphNode::GetNodeComment)
		.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
		.ColorAndOpacity(CommentColor)
		.AllowPinning(true)
		.EnableTitleBarBubble(true)
		.EnableBubbleCtrls(true)
		.GraphLOD(this, &SGraphNode::GetCurrentLOD)
		.IsGraphNodeHovered(this, &SGraphNode::IsHovered);

	GetOrAddSlot(ENodeZone::TopCenter)
		.SlotOffset2f(TAttribute<FVector2f>(CommentBubble.Get(), &SCommentBubble::GetOffset2f))
		.SlotSize2f(TAttribute<FVector2f>(CommentBubble.Get(), &SCommentBubble::GetSize2f))
		.AllowScaling(TAttribute<bool>(CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
		.VAlign(VAlign_Top)
		[
			CommentBubble.ToSharedRef()
		];

	ErrorReporting = ErrorText;
	ErrorReporting->SetError(ErrorMsg);
	CreatePinWidgets();
}

void SKataEdNode::CreatePinWidgets()
{
	UKataEdNode* StateNode = CastChecked<UKataEdNode>(GraphNode);

	for (int32 PinIdx = 0; PinIdx < StateNode->Pins.Num(); PinIdx++)
	{
		UEdGraphPin* MyPin = StateNode->Pins[PinIdx];
		if (!MyPin->bHidden)
		{
			TSharedPtr<SGraphPin> NewPin = SNew(SKataGraphPin, MyPin);

			AddPin(NewPin.ToSharedRef());
		}
	}
}

void SKataEdNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
	if (bAdvancedParameter)
	{
		PinToAdd->SetVisibility( TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced) );
	}

	TSharedPtr<SVerticalBox> PinBox;
	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		PinBox = LeftNodeBox;
		InputPins.Add(PinToAdd);
	}
	else // Direction == EEdGraphPinDirection::EGPD_Output
	{
		PinBox = RightNodeBox;
		OutputPins.Add(PinToAdd);
	}

	if (PinBox)
	{
		PinBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			//.Padding(6.0f, 0.0f)
			[
				PinToAdd
			];
	}
}

bool SKataEdNode::IsNameReadOnly() const
{
	UKataEdNode* EdNode_Node = Cast<UKataEdNode>(GraphNode);
	check(EdNode_Node != nullptr);

	// 노드의 Graph는 그래프를 다시 만들 때 채우므로 방금 만든 노드에서는 비어 있을 수 있다.
	// 편집기 그래프의 소유자에서 가져오면 생성 직후에도 올바르다.
	const UKataEdGraph* EdGraph = Cast<UKataEdGraph>(EdNode_Node->GetGraph());
	const UKataGraphBase* KataGraph = EdGraph != nullptr ? EdGraph->GetKataGraph() : nullptr;
	if (KataGraph == nullptr || !KataGraph->bCanRenameNode)
	{
		return true;
	}

	return !EdNode_Node->KataNode->IsNameEditable() || SGraphNode::IsNameReadOnly();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SKataEdNode::OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
    UKataEdNode* EdNode = CastChecked<UKataEdNode>(GraphNode);
    if (UKataSubGraphNode* SubGraphNode = Cast<UKataSubGraphNode>(EdNode->KataNode))
    {
        if (CommitInfo == ETextCommit::OnCleared || !SubGraphNode->IsNameEditable())
        {
            return;
        }
        const FScopedTransaction Transaction(LOCTEXT("RenameSubGraph", "Rename SubGraph"));
        FText Error;
        if (KataEmbeddedSubGraphEditor::Rename(Cast<UKataGraph>(EdNode->GetGraph()->GetOuter()),
            SubGraphNode->GetReferencedSubGraph(), InText, Error))
        {
            EdNode->GetSchema()->ForceVisualizationCacheClear();
            EdNode->GetGraph()->NotifyGraphChanged();
        }
        return;
    }
	SGraphNode::OnNameTextCommited(InText, CommitInfo);

	UKataEdNode* MyNode = CastChecked<UKataEdNode>(GraphNode);

	if (MyNode != nullptr && MyNode->KataNode != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("KataGraphEditorRenameNode", "Kata Graph Editor: Rename Node"));
		MyNode->Modify();
		MyNode->KataNode->Modify();
		// 자동으로 만든 설명과 같은 글자를 커밋했으면 직접 지은 이름으로 치지 않는다.
		// 그대로 저장하면 이후에 참조 에셋을 바꿔도 제목이 그 자리에 굳는다. 노드를 한 번 클릭해
		// 편집 상태로 들어갔다가 그대로 빠져나오기만 해도 커밋이 일어난다.
		const FText AutoDescription = MyNode->KataNode->GetDescription();
		MyNode->KataNode->SetNodeTitle(InText.EqualTo(AutoDescription) ? FText::GetEmpty() : InText);
		UpdateGraphNode();
	}
}

bool SKataEdNode::OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage)
{
    const UKataEdNode* EdNode = CastChecked<UKataEdNode>(GraphNode);
    if (const UKataSubGraphNode* SubGraphNode = Cast<UKataSubGraphNode>(EdNode->KataNode))
    {
        return KataEmbeddedSubGraphEditor::ValidateName(Cast<UKataGraph>(EdNode->GetGraph()->GetOuter()),
            SubGraphNode->GetReferencedSubGraph(), InText, OutErrorMessage);
    }
    return SGraphNode::OnVerifyNameTextChanged(InText, OutErrorMessage);
}

FSlateColor SKataEdNode::GetBorderBackgroundColor() const
{
	UKataEdNode* MyNode = CastChecked<UKataEdNode>(GraphNode);
	// 디버그 강조는 노드 고유 색보다 우선한다. 바깥 테두리만 바뀌고 제목 영역은 그대로 읽힌다.
	switch (MyNode->DebugHighlight)
	{
	case EKataGraphDebugHighlight::Active:
		return KataGraphColors::NodeBorder::ActiveDebugging;
	case EKataGraphDebugHighlight::ActiveInside:
		return KataGraphColors::NodeBorder::InactiveDebugging;
	case EKataGraphDebugHighlight::Pending:
		return KataGraphColors::NodeBorder::PendingDebugging;
	case EKataGraphDebugHighlight::PendingInside:
		return KataGraphColors::NodeBorder::PendingInactiveDebugging;
	default:
		break;
	}
	return MyNode ? MyNode->GetBackgroundColor() : KataGraphColors::NodeBorder::HighlightAbortRange0;
}

FSlateColor SKataEdNode::GetBackgroundColor() const
{
	return KataGraphColors::NodeBody::Default;
}

EVisibility SKataEdNode::GetDragOverMarkerVisibility() const
{
	return EVisibility::Visible;
}

const FSlateBrush* SKataEdNode::GetNameIcon() const
{
	return FAppStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Icon"));
}

#undef LOCTEXT_NAMESPACE
