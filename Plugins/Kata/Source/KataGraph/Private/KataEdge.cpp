#include "KataEdge.h"

#define LOCTEXT_NAMESPACE "KataEdge"

UKataEdge::UKataEdge()
{
#if WITH_EDITORONLY_DATA
    // 연결선 위에 트리거를 보여줘야 그래프를 읽을 수 있다.
    bShouldDrawTitle = true;
#endif
}

bool UKataEdge::MatchesTrigger(const FGameplayTag& Trigger) const
{
    if (IsAutomatic())
    {
        return false;
    }
    // 엣지 쪽이 상위 태그다. Input.Attack 엣지가 Input.Attack.Light 트리거를 받는다.
    return Trigger.MatchesTag(TriggerTag);
}

#if WITH_EDITOR
FText UKataEdge::GetNodeTitle() const
{
    if (!NodeTitle.IsEmpty())
    {
        return NodeTitle;
    }
    if (IsAutomatic())
    {
        return LOCTEXT("AutoTransition", "Auto");
    }
    return FText::FromName(TriggerTag.GetTagName());
}
#endif

#undef LOCTEXT_NAMESPACE
