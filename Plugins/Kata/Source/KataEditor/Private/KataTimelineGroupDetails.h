#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KataTimelineGroupDetails.generated.h"

/** Timeline Details에서 선택한 그룹을 편집하기 위한 임시 객체. */
UCLASS(Transient)
class UKataTimelineGroupDetails : public UObject
{
    GENERATED_BODY()

public:
    /** 타임라인 그룹 헤더에 표시할 이름. */
    UPROPERTY(EditAnywhere, Category = "Timeline Group", meta = (DisplayName = "Name"))
    FText Title;

    /** 타임라인 그룹 헤더와 구분선에 사용할 색상. */
    UPROPERTY(EditAnywhere, Category = "Timeline Group", meta = (DisplayName = "Display Color"))
    FLinearColor DisplayColor = FLinearColor::White;

    /** 그룹의 용도를 설명하는 편집기 전용 주석. */
    UPROPERTY(EditAnywhere, Category = "Timeline Group", meta = (DisplayName = "Editor Comment", MultiLine = true))
    FText EditorComment;

    /** 현재 그룹에 들어 있는 태스크 수. */
    UPROPERTY(VisibleAnywhere, Category = "Timeline Group", meta = (DisplayName = "Task Count"))
    int32 TaskCount = 0;
};
