#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "GameplayTagContainer.h"
#include "KataCameraRailComponent.generated.h"

/**
 * 플레이어 캐릭터 Blueprint에서 편집하는 카메라 궤도 레일.
 *
 * 컴포넌트 원점이 피벗이다. 카메라 배치는 로컬 곡선에 상대 회전·스케일과 카메라 Yaw를 적용한다.
 * 컴포넌트의 월드 회전으로 곡선을 평가하지 않으므로 캐릭터가 회전해도 궤도 방향은 카메라 입력을 따른다.
 * 현재 뷰 타깃 폰에 같은 RailTag를 가진 컴포넌트가 정확히 하나 있어야 하며, 열린 Spline만 지원한다.
 */
UCLASS(ClassGroup = (Kata), meta = (BlueprintSpawnableComponent, DisplayName = "Kata Camera Rail"))
class KATACAMERA_API UKataCameraRailComponent : public USplineComponent
{
    GENERATED_BODY()

public:
    UKataCameraRailComponent();

    /** CameraData의 Spline 배치가 이 태그와 정확히 일치하는 레일을 선택한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Camera Rail")
    FGameplayTag RailTag;

    /** Spline 로컬 위치를 피벗 기준 카메라 Yaw 공간으로 변환한다. 월드 이동·회전은 포함하지 않는다. */
    FVector GetOrbitOffsetAtDistance(float Distance) const;
};
