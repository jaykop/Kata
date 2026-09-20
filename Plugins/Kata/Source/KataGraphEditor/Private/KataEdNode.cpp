#include "KataEdNode.h"
#include "KataEdGraph.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "EdNode_KataGraph"

UKataEdNode::UKataEdNode()
{
	bCanRenameNode = true;
}

UKataEdNode::~UKataEdNode()
{

}

void UKataEdNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, "MultipleNodes", FName(), TEXT("In"));
	CreatePin(EGPD_Output, "MultipleNodes", FName(), TEXT("Out"));
}

UKataEdGraph* UKataEdNode::GetKataEdGraph()
{
	return Cast<UKataEdGraph>(GetGraph());
}

FText UKataEdNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (KataNode == nullptr)
	{
		return Super::GetNodeTitle(TitleType);
	}
	else
	{
		return KataNode->GetNodeTitle();
	}
}

void UKataEdNode::PrepareForCopying()
{
	KataNode->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UKataEdNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin != nullptr)
	{
		if (GetSchema()->TryCreateConnection(FromPin, GetInputPin()))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
}

void UKataEdNode::SetKataNode(UKataGraphNodeBase* InNode)
{
	KataNode = InNode;
}

FLinearColor UKataEdNode::GetBackgroundColor() const
{
	return KataNode == nullptr ? FLinearColor::Black : KataNode->GetBackgroundColor();
}

UEdGraphPin* UKataEdNode::GetInputPin() const
{
	return Pins[0];
}

UEdGraphPin* UKataEdNode::GetOutputPin() const
{
	return Pins[1];
}

void UKataEdNode::PostEditUndo()
{
	UEdGraphNode::PostEditUndo();
}

#undef LOCTEXT_NAMESPACE
