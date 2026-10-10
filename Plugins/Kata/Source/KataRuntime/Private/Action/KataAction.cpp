#include "Action/KataAction.h"

#include "Action/KataActionTemplate.h"
#include "Action/KataPropertyOverride.h"

void UKataAction::PostLoad()
{
    Super::PostLoad();

    // 삭제된 설정의 경로만 제거한다. 나머지 명시적 오버라이드는 그대로 보존한다.
    OverriddenSettings.Remove(FName(TEXT("LoopPolicy.MaxIterationsPerTick")));
}

#if WITH_EDITOR
void UKataAction::PostEditChangeChainProperty(FPropertyChangedChainEvent& Event)
{
    // 부모 Template이 있을 때만 이 에셋에서 바꾼 고유 설정 경로를 오버라이드로 기록한다.
    if (ParentAction && Event.MemberProperty)
    {
        const FName Name = Event.MemberProperty->GetFName();
        if (Name != GET_MEMBER_NAME_CHECKED(UKataAction, ParentAction)
            && Name != GET_MEMBER_NAME_CHECKED(UKataAction, OverriddenSettings)
            && Name != GET_MEMBER_NAME_CHECKED(UKataAction, TimelineTasks)
            && Name != GET_MEMBER_NAME_CHECKED(UKataAction, TaskOverrides)
            && !Name.ToString().StartsWith(TEXT("Preview"))
            && !Name.ToString().StartsWith(TEXT("bPreview")))
        {
            OverriddenSettings.AddUnique(KataPropertyOverride::GetPropertyPath(Event.MemberProperty, Event.Property));
        }
    }
    Super::PostEditChangeChainProperty(Event);
}
#endif
