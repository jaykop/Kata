#include "Cheats/KataAICheatState.h"

#if UE_WITH_CHEAT_MANAGER

#include "GameFramework/Actor.h"
#include "UObject/WeakObjectPtrTemplates.h"

namespace KataAICheat
{
    namespace
    {
        /**
         * 숨긴 액터 목록. PIE 인스턴스마다 액터가 다르므로 월드를 나누지 않고 약한 참조로 함께 둔다.
         * 치트 전용이라 개수가 작아 선형 탐색으로 충분하다.
         */
        TArray<TWeakObjectPtr<const AActor>>& GetHiddenActors()
        {
            static TArray<TWeakObjectPtr<const AActor>> HiddenActors;
            return HiddenActors;
        }
    }

    void SetHiddenFromAI(const AActor* Actor, bool bHidden)
    {
        TArray<TWeakObjectPtr<const AActor>>& HiddenActors = GetHiddenActors();
        HiddenActors.RemoveAll([Actor](const TWeakObjectPtr<const AActor>& Entry)
        {
            return !Entry.IsValid() || Entry.Get() == Actor;
        });
        if (bHidden && Actor != nullptr)
        {
            HiddenActors.Add(Actor);
        }
    }

    bool IsHiddenFromAI(const AActor* Actor)
    {
        if (Actor == nullptr)
        {
            return false;
        }
        return GetHiddenActors().ContainsByPredicate([Actor](const TWeakObjectPtr<const AActor>& Entry)
        {
            return Entry.Get() == Actor;
        });
    }
}

#endif
