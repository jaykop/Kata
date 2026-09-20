#include "KataEdNodeEdge.h"
#include "KataGraphEdgeBase.h"
#include "KataEdNode.h"

#define LOCTEXT_NAMESPACE "EdNode_KataGraphEdge"

UKataEdNodeEdge::UKataEdNodeEdge()
{
	bCanRenameNode = true;
}

void UKataEdNodeEdge::SetEdge(UKataGraphEdgeBase* Edge)
{
	KataEdge = Edge;
}

void UKataEdNodeEdge::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, TEXT("Edge"), FName(), TEXT("In"));
	Inputs->bHidden = true;
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, TEXT("Edge"), FName(), TEXT("Out"));
	Outputs->bHidden = true;
}

FText UKataEdNodeEdge::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (KataEdge)
	{
		return KataEdge->GetNodeTitle();
	}
	return FText();
}

void UKataEdNodeEdge::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (Pin->LinkedTo.Num() == 0)
	{
		// Commit suicide; transitions must always have an input and output connection
		Modify();

		// Our parent graph will have our graph in SubGraphs so needs to be modified to record that.
		if (UEdGraph* ParentGraph = GetGraph())
		{
			ParentGraph->Modify();
		}

		DestroyNode();
	}
}

void UKataEdNodeEdge::PrepareForCopying()
{
	KataEdge->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UKataEdNodeEdge::CreateConnections(UKataEdNode* Start, UKataEdNode* End)
{
	Pins[0]->Modify();
	Pins[0]->LinkedTo.Empty();

	Start->GetOutputPin()->Modify();
	Pins[0]->MakeLinkTo(Start->GetOutputPin());

	// This to next
	Pins[1]->Modify();
	Pins[1]->LinkedTo.Empty();

	End->GetInputPin()->Modify();
	Pins[1]->MakeLinkTo(End->GetInputPin());
}

UKataEdNode* UKataEdNodeEdge::GetStartNode()
{
	if (Pins[0]->LinkedTo.Num() > 0)
	{
		return Cast<UKataEdNode>(Pins[0]->LinkedTo[0]->GetOwningNode());
	}
	else
	{
		return nullptr;
	}
}

UKataEdNode* UKataEdNodeEdge::GetEndNode()
{
	if (Pins[1]->LinkedTo.Num() > 0)
	{
		return Cast<UKataEdNode>(Pins[1]->LinkedTo[0]->GetOwningNode());
	}
	else
	{
		return nullptr;
	}
}

#undef LOCTEXT_NAMESPACE

