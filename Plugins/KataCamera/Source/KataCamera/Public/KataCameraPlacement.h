#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "KataCameraPlacement.generated.h"

struct FKataCameraPipelineContext;

/**
 * 카메라 데이터가 정하는 배치 방식의 기반 클래스.
 *
 * 카메라 데이터 에셋의 인스턴스 서브오브젝트로 존재하며 여러 플레이어가 같은 객체를 공유한다.
 * 그래서 Evaluate()는 const이고 실행 상태를 저장하지 않는다. 지연 같은 프레임 간 상태가 필요하면 카메라 매니저 쪽에 둔다.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class KATACAMERA_API UKataCameraPlacement : public UObject
{
    GENERATED_BODY()

public:
    /**
     * PivotLocation·ViewRotation·선택된 Rail을 읽어 카메라 위치·회전·FOV와 궤도 공간 결과를 채운다.
     * Spline 배치는 피벗을 레일 원점으로 바꾸며 실패하면 초기 피벗에서 Boom Arm으로 대체한다.
     * 입력 포인터와 ViewRotation은 바꾸지 않는다. 이후 Feature는 최종 위치·회전·FOV를 보정할 수 있다.
     */
    virtual void Evaluate(FKataCameraPipelineContext& Context) const PURE_VIRTUAL(UKataCameraPlacement::Evaluate, );
};

/**
 * 피벗에서 시선 반대 방향으로 고정 거리만큼 떨어진 곳에 카메라를 둔다.
 *
 * SpringArm과 같은 배치이며 충돌 처리는 하지 않는다. 장애물 회피는 Constraint 단계의 Feature가 맡는다.
 */
UCLASS(meta = (DisplayName = "Boom Arm"))
class KATACAMERA_API UKataCameraPlacement_BoomArm : public UKataCameraPlacement
{
    GENERATED_BODY()

public:
    virtual void Evaluate(FKataCameraPipelineContext& Context) const override;

    /** 피벗에서 시선 반대 방향으로 카메라를 떨어뜨리는 거리(cm). 시점을 돌려도 이 거리를 유지한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boom Arm", meta = (ClampMin = "0.0", Units = "Centimeters"))
    float Distance = 400.0f;

    /**
     * 뷰 타깃 위치에서 피벗까지의 오프셋.
     * X·Y는 카메라 Yaw 기준(X 앞, Y 오른쪽)으로 적용해 어깨 너머 오프셋이 화면의 같은 쪽에 머물게 한다. Z는 월드 위쪽이다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boom Arm", meta = (Units = "Centimeters"))
    FVector PivotOffset = FVector(0.0, 0.0, 60.0);
};

/** 입력 Pitch로 열린 Spline 위의 카메라 위치를 정하고, 레일 원점인 피벗에 AimOffsetCurve 값을 더한 지점을 바라보는 배치. */
UCLASS(meta = (DisplayName = "Spline Rail"))
class KATACAMERA_API UKataCameraPlacement_Spline : public UKataCameraPlacement
{
    GENERATED_BODY()

public:
    virtual void Evaluate(FKataCameraPipelineContext& Context) const override;

    /**
     * 카메라 위치를 정할 UKataCameraRailComponent의 식별 태그.
     * 현재 뷰 타깃 폰에 이 태그와 정확히 일치하는 레일이 하나 있어야 한다. 부모·자식 태그는 매칭하지 않는다.
     * 레일 컴포넌트 원점이 피벗이며, 입력 Pitch의 최솟값과 최댓값은 각각 Spline 시작점과 끝점에 대응한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Rail")
    FGameplayTag RailTag;

    /**
     * 레일을 찾지 못하거나 레일 설정·평가값이 유효하지 않을 때 대신 사용할 Boom Arm 거리(cm).
     * 대체 배치의 피벗은 오프셋 없이 뷰 타깃 위치이며, FOV는 데이터의 FieldOfView를 사용한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Rail", meta = (ClampMin = "0.0", Units = "Centimeters"))
    float FallbackDistance = 400.0f;

    /**
     * 피벗에 더해서 카메라가 바라볼 지점을 정하는 오프셋 커브(cm). 카메라 위치는 Rail이 정한다.
     * 가로축은 입력 Pitch를 정규화한 값으로, 카메라 데이터의 PitchMin에서 0, PitchMax에서 1이다.
     * Rail 위의 위치를 선택하는 값과 같은 값을 사용하므로, 위아래 시점 입력에 맞춰 조준점을 바꿀 수 있다.
     * X·Y는 카메라 Yaw 기준이고 Z는 월드 위쪽이다. 커브가 비어 있으면 오프셋은 0이며 피벗을 바라본다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Rail|Profile")
    FRuntimeVectorCurve AimOffsetCurve;

    /**
     * 위아래 시점 입력에 따라 수평 FOV(도)를 정하는 커브.
     * 가로축은 Rail 위의 위치를 선택하는 값과 같은 정규화된 Pitch로, PitchMin에서 0, PitchMax에서 1이다.
     * 커브가 비어 있으면 카메라 데이터의 FieldOfView를 사용한다. 커브 평가 결과는 5~170도로 제한한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Rail|Profile")
    FRuntimeFloatCurve FieldOfViewCurve;
};
