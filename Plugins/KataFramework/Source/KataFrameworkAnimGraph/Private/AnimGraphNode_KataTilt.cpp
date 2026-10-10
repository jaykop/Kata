#include "AnimGraphNode_KataTilt.h"

#define LOCTEXT_NAMESPACE "KataFrameworkAnimGraph"

FText UAnimGraphNode_KataTilt::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return GetControllerDescription();
}

FText UAnimGraphNode_KataTilt::GetTooltipText() const
{
    return LOCTEXT("KataTiltTooltip",
        "Adds the target tilt pitch to the spine chain set in the Kata Anim Instance class defaults (Tilt Bone Chain). "
        "Place it after the Slot node and bind Pitch to Tilt Pitch and Alpha to Tilt Alpha.");
}

FText UAnimGraphNode_KataTilt::GetControllerDescription() const
{
    return LOCTEXT("KataTiltDescription", "Kata Tilt");
}

#undef LOCTEXT_NAMESPACE
