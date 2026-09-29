#include "KataCameraData.h"

#include "KataCameraPlacement.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "KataCameraData"

#if WITH_EDITOR
EDataValidationResult UKataCameraData::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (Placement == nullptr)
    {
        Context.AddError(LOCTEXT("MissingPlacement", "Placement is empty. The camera manager falls back to the engine default camera."));
        Result = EDataValidationResult::Invalid;
    }

    if (PitchMin > PitchMax)
    {
        Context.AddError(LOCTEXT("InvalidPitchRange", "PitchMin must not be greater than PitchMax."));
        Result = EDataValidationResult::Invalid;
    }

    return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
