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

    const UKataCameraPlacement_Spline* SplinePlacement = Cast<UKataCameraPlacement_Spline>(Placement);
    if (!FMath::IsFinite(PitchMin) || !FMath::IsFinite(PitchMax) || PitchMin > PitchMax ||
        (SplinePlacement != nullptr && PitchMax - PitchMin <= UE_SMALL_NUMBER))
    {
        Context.AddError(LOCTEXT("InvalidPitchRange", "Pitch limits must be finite and ordered. Spline Rail requires PitchMin < PitchMax."));
        Result = EDataValidationResult::Invalid;
    }

    if (SplinePlacement != nullptr)
    {
        if (!SplinePlacement->RailTag.IsValid())
        {
            Context.AddError(LOCTEXT("InvalidRailTag", "Spline Rail requires a valid RailTag."));
            Result = EDataValidationResult::Invalid;
        }
        if (!FMath::IsFinite(SplinePlacement->FallbackDistance) || SplinePlacement->FallbackDistance < 0.0f)
        {
            Context.AddError(LOCTEXT("InvalidFallbackDistance", "FallbackDistance must be finite and non-negative."));
            Result = EDataValidationResult::Invalid;
        }
    }

    return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
