#include "Input/KataInputConfig.h"

FGameplayTag UKataInputConfig::FindTriggerTag(const FGameplayTag& InputTag) const
{
    for (const FKataInputTriggerMapping& Mapping : TriggerMappings)
    {
        if (Mapping.InputTag == InputTag)
        {
            return Mapping.TriggerTag;
        }
    }
    return FGameplayTag();
}
