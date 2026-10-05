#include "Data/KataAIData.h"

#include "Perception/AISense.h"
#include "Perception/AISenseConfig.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UKataAIData::IsDataValid(FDataValidationContext& Context) const
{
    const EDataValidationResult ParentResult = Super::IsDataValid(Context);
    bool bInvalid = ParentResult == EDataValidationResult::Invalid;
    TSet<const UClass*> Implementations;
    for (const UAISenseConfig* Config : Senses)
    {
        const UClass* Implementation = Config != nullptr ? Config->GetSenseImplementation().Get() : nullptr;
        if (Implementation == nullptr || Implementations.Contains(Implementation))
        {
            Context.AddError(FText::FromString(TEXT("Senses must contain valid, unique sense implementations.")));
            bInvalid = true;
        }
        else
        {
            Implementations.Add(Implementation);
        }
    }
    if (DominantSense != nullptr && !Implementations.Contains(DominantSense.Get()))
    {
        Context.AddError(FText::FromString(TEXT("Dominant Sense must be included in Senses.")));
        bInvalid = true;
    }
    return bInvalid ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
