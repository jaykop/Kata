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

    /** 피벗에서 카메라까지의 거리. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boom Arm", meta = (ClampMin = "0.0", Units = "Centimeters"))
    float Distance = 400.0f;
};

/** 입력 Pitch에 대응하는 열린 Spline 위치에서 피벗과 조준점 오프셋을 바라보는 배치. */
UCLASS(meta = (DisplayName = "Spline Rail"))
class KATACAMERA_API UKataCameraPlacement_Spline : public UKataCameraPlacement
{
    GENERATED_BODY()

public:
    virtual void Evaluate(FKataCameraPipelineContext& Context) const override;

    /** 현재 뷰 타깃 폰에서 이 태그와 정확히 일치하는 UKataCameraRailComponent를 사용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Rail")
    FGameplayTag RailTag;

    /** 레일을 사용할 수 없을 때 Data.PivotOffset을 피벗으로 삼는 Boom Arm의 거리. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Rail", meta = (ClampMin = "0.0", Units = "Centimeters"))
    float FallbackDistance = 400.0f;

    /** 입력은 정규화된 Pitch(0~1), 값은 피벗 기준 조준점(cm)이다. X·Y는 카메라 Yaw 기준, Z는 위쪽이다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Rail|Profile")
    FRuntimeVectorCurve AimOffsetCurve;

    /** 입력은 정규화된 Pitch(0~1), 값은 수평 FOV(도)다. 비어 있으면 Data.FieldOfView를 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Rail|Profile")
    FRuntimeFloatCurve FieldOfViewCurve;
};
