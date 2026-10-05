#include "ActionGroup/KataActionGroup.h"

#include "Action/KataAction.h"
#include "KataGraph.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool FKataActionGroupEntry::HasValidAsset() const
{
    switch (Type)
    {
    case EKataActionGroupEntryType::Action:
        return IsValid(Action.Get());
    case EKataActionGroupEntryType::Graph:
        return IsValid(Graph.Get());
    default:
        return false;
    }
}

bool FKataActionGroupEntry::HasValidPayload() const
{
    return !Payload.IsValid() || Payload.GetScriptStruct()->IsChildOf(FKataActionGroupPayload::StaticStruct());
}

#if WITH_EDITOR
EDataValidationResult UKataActionGroup::IsDataValid(FDataValidationContext& Context) const
{
    bool bInvalid = Super::IsDataValid(Context) == EDataValidationResult::Invalid;
    bool bHasPositiveWeight = false;
    if (Entries.IsEmpty())
    {
        Context.AddError(FText::FromString(TEXT("Action Group must contain at least one entry.")));
        bInvalid = true;
    }
    for (int32 Index = 0; Index < Entries.Num(); ++Index)
    {
        const FKataActionGroupEntry& Entry = Entries[Index];
        if (!Entry.HasValidAsset())
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Entry %d has an invalid type or missing active asset."), Index)));
            bInvalid = true;
        }
        if (!FMath::IsFinite(Entry.Weight) || Entry.Weight < 0.0f)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Entry %d Weight must be finite and non-negative."), Index)));
            bInvalid = true;
        }
        else if (Entry.Weight > 0.0f)
        {
            bHasPositiveWeight = true;
        }
        if (!Entry.HasValidPayload())
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Entry %d Payload must derive from KataActionGroupPayload."), Index)));
            bInvalid = true;
        }
        if ((Entry.Type == EKataActionGroupEntryType::Action && Entry.Graph != nullptr)
            || (Entry.Type == EKataActionGroupEntryType::Graph && Entry.Action != nullptr))
        {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT("Entry %d retains an inactive asset reference; only Type's active asset is used."), Index)));
        }
    }
    if (!Entries.IsEmpty() && !bHasPositiveWeight)
    {
        Context.AddError(FText::FromString(TEXT("Action Group must contain at least one positive Weight.")));
        bInvalid = true;
    }
    return bInvalid ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
