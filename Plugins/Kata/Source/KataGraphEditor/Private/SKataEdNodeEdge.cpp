#include "SKataEdNodeEdge.h"
#include "EdGraph/EdGraph.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/SToolTip.h"
#include "SGraphPanel.h"
#include "EdGraphSchema_K2.h"
#include "KataEdNode.h"
#include "KataEdNodeEdge.h"
#include "KataGraphEdgeBase.h"
#include "KataGraphConnectionDrawingPolicy.h"

#define LOCTEXT_NAMESPACE "SKataGraphEdge"

void SKataEdNodeEdge::Construct(const FArguments& InArgs, UKataEdNodeEdge* InNode)
{
	this->GraphNode = InNode;
	this->UpdateGraphNode();
}

bool SKataEdNodeEdge::RequiresSecondPassLayout() const
{
	return true;
}

void SKataEdNodeEdge::PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const
{
	UKataEdNodeEdge* EdgeNode = CastChecked<UKataEdNodeEdge>(GraphNode);

	FGeometry StartGeom;
	FGeometry EndGeom;

	UKataEdNode* Start = EdgeNode->GetStartNode();
	UKataEdNode* End = EdgeNode->GetEndNode();
	if (Start != nullptr && End != nullptr)
	{
		const TSharedRef<SNode>* pFromWidget = NodeToWidgetLookup.Find(Start);
		const TSharedRef<SNode>* pToWidget = NodeToWidgetLookup.Find(End);
		if (pFromWidget != nullptr && pToWidget != nullptr)
		{
			const TSharedRef<SNode>& FromWidget = *pFromWidget;
			const TSharedRef<SNode>& ToWidget = *pToWidget;

			StartGeom = FGeometry(FVector2D(Start->NodePosX, Start->NodePosY), FVector2D::ZeroVector, FromWidget->GetDesiredSize(), 1.0f);
			EndGeom = FGeometry(FVector2D(End->NodePosX, End->NodePosY), FVector2D::ZeroVector, ToWidget->GetDesiredSize(), 1.0f);

			// 같은 노드 쌍을 잇는 엣지를 그래프 저장 순서로 모아 서로 다른 위치에 배치한다.
			TArray<UKataEdNodeEdge*> ParallelEdges;
			if (const UEdGraph* Graph = EdgeNode->GetGraph())
			{
				for (UEdGraphNode* Node : Graph->Nodes)
				{
					UKataEdNodeEdge* OtherEdge = Cast<UKataEdNodeEdge>(Node);
					if (OtherEdge != nullptr && OtherEdge->GetStartNode() == Start && OtherEdge->GetEndNode() == End)
					{
						ParallelEdges.Add(OtherEdge);
					}
				}
			}

			const int32 EdgeIndex = FMath::Max(0, ParallelEdges.IndexOfByKey(EdgeNode));
			PositionBetweenTwoNodesWithOffset(StartGeom, EndGeom, EdgeIndex, FMath::Max(1, ParallelEdges.Num()));
		}
	}
}

void SKataEdNodeEdge::OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
	SGraphNode::OnNameTextCommited(InText, CommitInfo);

	UKataEdNodeEdge* MyNode = CastChecked<UKataEdNodeEdge>(GraphNode);

	if (MyNode != nullptr && MyNode->KataEdge != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("KataGraphEditorRenameEdge", "Kata Graph Editor: Rename Edge"));
		MyNode->Modify();
		MyNode->KataEdge->SetNodeTitle(InText);
		UpdateGraphNode();
	}
}

void SKataEdNodeEdge::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	this->ContentScale.Bind( this, &SGraphNode::GetContentScale );
	this->GetOrAddSlot( ENodeZone::Center )
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Graph.TransitionNode.ColorSpill"))
				.ColorAndOpacity(this, &SKataEdNodeEdge::GetEdgeColor)
			]
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(this, &SKataEdNodeEdge::GetEdgeImage)
				.Visibility(this, &SKataEdNodeEdge::GetEdgeImageVisibility)
			]

			+ SOverlay::Slot()
			.Padding(FMargin(4.0f, 4.0f, 4.0f, 4.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				[
					SAssignNew(InlineEditableText, SInlineEditableTextBlock)
					.ColorAndOpacity(FLinearColor::Black)
					.Visibility(this, &SKataEdNodeEdge::GetEdgeTitleVisbility)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
					.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
					.OnTextCommitted(this, &SKataEdNodeEdge::OnNameTextCommited)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					NodeTitle.ToSharedRef()
				]
				
			]
		];
}

void SKataEdNodeEdge::PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom, int32 NodeIndex, int32 MaxNodes) const
{
	// Get a reasonable seed point (halfway between the boxes)
	const FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D SeedPoint = (StartCenter + EndCenter) * 0.5f;

	// Find the (approximate) closest points between the two boxes
	const FVector2D StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, SeedPoint);
	const FVector2D EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, SeedPoint);

	// Position ourselves halfway along the connecting line between the nodes, elevated away perpendicular to the direction of the line
	const float Height = 30.0f;

	const FVector2D DesiredNodeSize = GetDesiredSize();

	FVector2D DeltaPos(EndAnchorPoint - StartAnchorPoint);

	if (DeltaPos.IsNearlyZero())
	{
		DeltaPos = FVector2D(10.0f, 0.0f);
	}

	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

	const FVector2D NewCenter = StartAnchorPoint + (0.5f * DeltaPos) + (Height * Normal);

	FVector2D DeltaNormal = DeltaPos.GetSafeNormal();
	
	// Calculate node offset in the case of multiple transitions between the same two nodes
	// MultiNodeOffset: the offset where 0 is the centre of the transition, -1 is 1 <size of node>
	// towards the PrevStateNode and +1 is 1 <size of node> towards the NextStateNode.

	const float MutliNodeSpace = 0.2f; // Space between multiple transition nodes (in units of <size of node> )
	const float MultiNodeStep = (1.f + MutliNodeSpace); //Step between node centres (Size of node + size of node spacer)

	const float MultiNodeStart = -((MaxNodes - 1) * MultiNodeStep) / 2.f;
	const float MultiNodeOffset = MultiNodeStart + (NodeIndex * MultiNodeStep);

	// Now we need to adjust the new center by the node size, zoom factor and multi node offset
	const FVector2D NewCorner = NewCenter - (0.5f * DesiredNodeSize) + (DeltaNormal * MultiNodeOffset * DesiredNodeSize.Size());

	GraphNode->NodePosX = NewCorner.X;
	GraphNode->NodePosY = NewCorner.Y;
}

FSlateColor SKataEdNodeEdge::GetEdgeColor() const
{
	UKataEdNodeEdge* EdgeNode = CastChecked<UKataEdNodeEdge>(GraphNode);
	if (EdgeNode != nullptr && EdgeNode->KataEdge != nullptr)
	{
		return EdgeNode->KataEdge->GetEdgeColour();
	}
	return FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
}

const FSlateBrush* SKataEdNodeEdge::GetEdgeImage() const
{
	return FAppStyle::GetBrush("Graph.TransitionNode.Icon");
}

EVisibility SKataEdNodeEdge::GetEdgeImageVisibility() const
{
	UKataEdNodeEdge* EdgeNode = CastChecked<UKataEdNodeEdge>(GraphNode);
	if (EdgeNode && EdgeNode->KataEdge && EdgeNode->KataEdge->bShouldDrawTitle)
			return EVisibility::Hidden;

	return EVisibility::Visible;
}

EVisibility SKataEdNodeEdge::GetEdgeTitleVisbility() const
{
	UKataEdNodeEdge* EdgeNode = CastChecked<UKataEdNodeEdge>(GraphNode);
	if (EdgeNode && EdgeNode->KataEdge && EdgeNode->KataEdge->bShouldDrawTitle)
		return EVisibility::Visible;

	return EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
