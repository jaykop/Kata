#include "Data/KataAIData.h"

#include "Perception/AISense.h"
#include "Perception/AISenseConfig.h"
#include "StateTree.h"
#include "StateTreeSchema.h"
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
    // ClampMin은 에디터 입력만 막으므로 이전에 저장된 값과 코드에서 바꾼 값도 검증한다.
    if (MoveRetryInterval <= 0.0f)
    {
        Context.AddError(FText::FromString(TEXT("Move Retry Interval must be greater than zero to prevent per-frame retries.")));
        bInvalid = true;
    }
    const UStateTreeSchema* MasterSchema = StateTree.GetStateTree() != nullptr ? StateTree.GetStateTree()->GetSchema() : nullptr;
    TSet<FGameplayTag> SlotTags;
    for (const FKataAIStateTreeSlot& Slot : LinkedStateTreeSlots)
    {
        if (!Slot.Slot.IsValid() || SlotTags.Contains(Slot.Slot))
        {
            Context.AddError(FText::FromString(TEXT("Linked StateTree Slots must use valid, unique slot tags.")));
            bInvalid = true;
        }
        else
        {
            SlotTags.Add(Slot.Slot);
        }
        const UStateTree* SlotTree = Slot.StateTree.GetStateTree();
        if (SlotTree == nullptr)
        {
            Context.AddError(FText::Format(FText::FromString(TEXT("Linked StateTree Slot '{0}' has no StateTree.")),
                FText::FromName(Slot.Slot.GetTagName())));
            bInvalid = true;
        }
        else if (MasterSchema != nullptr && (SlotTree->GetSchema() == nullptr || SlotTree->GetSchema()->GetClass() != MasterSchema->GetClass()))
        {
            // 엔진은 스키마가 다른 오버라이드가 하나라도 있으면 목록 전체를 무시한다.
            Context.AddError(FText::Format(FText::FromString(TEXT("Linked StateTree Slot '{0}' must use the same schema as the main StateTree.")),
                FText::FromName(Slot.Slot.GetTagName())));
            bInvalid = true;
        }
    }
    return bInvalid ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
