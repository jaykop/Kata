#include "Data/KataFL_AIData.h"

#include "Data/KataAIData.h"
#include "KataAILog.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

namespace KataFL
{
    UKataAIData* ComposeAIData(UKataAIData* Base, const FKataAIDataOverride& Override, UObject* Outer)
    {
        if (Override.IsEmpty())
        {
            return Base;
        }
        if (Base == nullptr)
        {
            UE_LOG(LogKataAI, Warning, TEXT("AI Data override ignored: no base AI Data to apply it to (Outer=%s)."), *GetNameSafe(Outer));
            return nullptr;
        }

        UObject* CopyOuter = Outer != nullptr ? Outer : GetTransientPackage();
        const FName CopyName = MakeUniqueObjectName(CopyOuter, UKataAIData::StaticClass(),
            FName(*FString::Printf(TEXT("%s_Override"), *Base->GetName())));
        // Senses는 Instanced이므로 사본이 자기 설정 객체를 소유한다. 이후 Controller는 사본의 설정을 다시 복제해 등록한다.
        UKataAIData* Composed = DuplicateObject<UKataAIData>(Base, CopyOuter, CopyName);
        if (Composed == nullptr)
        {
            UE_LOG(LogKataAI, Warning, TEXT("AI Data override ignored: failed to copy '%s'."), *Base->GetPathName());
            return Base;
        }
        Composed->SetFlags(RF_Transient);

        TSet<FGameplayTag> OverrideTags;
        for (const FKataAIStateTreeSlot& Slot : Override.LinkedStateTreeSlots)
        {
            if (!Slot.Slot.IsValid())
            {
                UE_LOG(LogKataAI, Warning, TEXT("AI Data override for '%s' skipped a slot without a tag."), *Base->GetName());
                continue;
            }
            if (OverrideTags.Contains(Slot.Slot))
            {
                UE_LOG(LogKataAI, Warning, TEXT("AI Data override for '%s' has duplicate slot %s; the later entry is used."),
                    *Base->GetName(), *Slot.Slot.ToString());
            }
            OverrideTags.Add(Slot.Slot);

            // 같은 태그가 기준에 있으면 그 자리를 교체해 기준의 순서를 유지한다.
            FKataAIStateTreeSlot* Existing = Composed->LinkedStateTreeSlots.FindByPredicate(
                [&Slot](const FKataAIStateTreeSlot& Candidate) { return Candidate.Slot == Slot.Slot; });
            if (Existing != nullptr)
            {
                *Existing = Slot;
            }
            else
            {
                Composed->LinkedStateTreeSlots.Add(Slot);
            }
        }

        if (Override.bOverrideLeashDistance)
        {
            Composed->LeashDistance = FMath::Max(Override.LeashDistance, 0.0f);
        }
        return Composed;
    }
}
