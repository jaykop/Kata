#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "KataLockOnData.generated.h"

class UCurveFloat;
class UKataCameraData;

/** 락온 중 플레이어 기준으로 타겟을 화면의 어느 쪽에 둘지 정한다. */
UENUM(BlueprintType)
enum class EKataLockOnAlignment : uint8
{
    /** 타겟이 항상 플레이어 오른쪽에 보인다. 카메라는 플레이어→타겟 선의 오른쪽으로 비킨다. */
    Right,
    /** 타겟이 항상 플레이어 왼쪽에 보인다. 카메라는 플레이어→타겟 선의 왼쪽으로 비킨다. */
    Left,
    /** 획득·타겟 변경 시 현재 시선 기준으로 타겟이 있는 쪽을 고른다. 락온 중에는 타겟이 반대쪽으로 AutoSwitchAngle을 넘으면 바꾼다. */
    Auto
};

/**
 * 락온 정렬·구도·블렌드 설정. 카메라 매니저의 기본값과 UKataLockOnData가 같은 구조체를 쓴다.
 *
 * 거리와 Pitch 제한은 이 구조체에 두지 않는다. 락온 중 바꾸려면 UKataLockOnData::CameraData로 배치 데이터를 교체한다.
 */
USTRUCT(BlueprintType)
struct KATACAMERA_API FKataLockOnFramingSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alignment")
    EKataLockOnAlignment Alignment = EKataLockOnAlignment::Auto;

    /** 플레이어→타겟 선에서 카메라가 옆으로 비키는 최종 거리. 피벗·레일이 이미 옆으로 비킨 만큼은 보정해 이 거리에 맞춘다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alignment", meta = (ClampMin = "0.0", Units = "Centimeters"))
    float SideOffset = 60.0f;

    /** Auto에서 카메라 Yaw와 플레이어→타겟 Yaw의 차이가 반대쪽으로 이 각도를 넘으면 좌우를 바꾼다. 블렌드 중에는 판정하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alignment",
        meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "Degrees", EditCondition = "Alignment == EKataLockOnAlignment::Auto", EditConditionHides))
    float AutoSwitchAngle = 30.0f;

    /**
     * 조준점을 둘 화면 위치. 왼쪽 위가 (0, 0), 오른쪽 아래가 (1, 1)이다.
     * X는 Right 정렬 기준 값이며 Left 정렬에서는 좌우를 뒤집는다. 카메라 위치는 바꾸지 않고 회전만 조정한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Framing", meta = (ClampMin = "0.05", ClampMax = "0.95"))
    FVector2D TargetScreenPosition = FVector2D(0.5, 0.4);

    /** 조준점 = Lerp(피벗, 락온 지점, LookAtAlpha). 1이면 타겟을, 0.5면 플레이어와 타겟의 중간을 TargetScreenPosition에 둔다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Framing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LookAtAlpha = 1.0f;

    /**
     * 피벗에서 조준점까지 거리의 상한. 0이면 제한하지 않는다. 타겟이 멀어져도 플레이어가 화면 밖으로 밀리지 않게 한다.
     * 실제 조준점 비율이 1보다 작을 때만 쓴다. 거리 곡선이 비율을 정할 수 있어 Look At Alpha 값으로 숨기지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Framing", meta = (ClampMin = "0.0", Units = "Centimeters"))
    float MaxLookDistance = 0.0f;

    /**
     * 블렌드가 끝난 뒤 지점 방향을 따라잡는 데 걸리는 대략의 시간. 0이면 즉시 따른다.
     * 임계 감쇠로 따라가므로 플레이어의 짧은 이동마다 시선이 돌지 않는다. 획득·타겟 변경 블렌드 중에는 블렌드 곡선이 회전을 정한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Framing", meta = (ClampMin = "0.0", Units = "Seconds"))
    float RotationLagTime = 0.15f;

    /**
     * 플레이어와 락온 지점 사이 거리(cm)에 따른 Pitch 오프셋(도). 양수면 카메라가 올라가 내려다본다.
     * 락온 회전의 목표 Pitch에 더하므로 Boom Arm과 Spline 위치가 함께 움직이며, 카메라 데이터의 Pitch 범위로 제한된다. 비어 있으면 0이다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Distance")
    FRuntimeFloatCurve PitchOffsetByDistance;

    /**
     * 플레이어와 락온 지점 사이 거리(cm)에 따른 Boom Arm 거리 배율. 1이면 그대로다. 가까운 대형 타겟을 담으려면 가까운 쪽 값을 키운다.
     * Spline은 레일 모양이 거리를 정하므로 적용하지 않는다. 비어 있으면 1이다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Distance")
    FRuntimeFloatCurve BoomDistanceScaleByDistance;

    /**
     * 플레이어와 락온 지점 사이 거리(cm)에 따른 조준점 비율 t(0~1). 데이터가 있으면 Look At Alpha 대신 쓰고, 비어 있으면 Look At Alpha를 쓴다.
     * 예를 들어 가까우면 1(타겟), 멀면 0.5(중간)로 두어 먼 타겟을 볼 때 플레이어가 화면 아래로 밀리지 않게 한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Distance")
    FRuntimeFloatCurve LookAtAlphaByDistance;

    /** 획득·타겟 변경·좌우 전환에 걸리는 시간. 해제도 같은 시간 동안 기본 Ease In-Out으로 블렌드한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blend", meta = (ClampMin = "0.0", Units = "Seconds"))
    float BlendInDuration = 0.5f;

    /** 0~1 진행도를 0~1 가중치로 바꾸는 곡선. 단조 증가해야 하며 비어 있으면 Ease In-Out이다. 해제에는 쓰지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blend")
    TObjectPtr<UCurveFloat> BlendInCurve;
};

/**
 * 부위별 락온 데이터. 타겟 지점이 사용하도록 설정하면 매니저 기본값 대신 이 설정 전체를 쓴다.
 * 여러 플레이어가 공유하는 에셋이므로 실행 상태는 카메라 매니저에만 둔다.
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "Kata Lock On Data"))
class KATACAMERA_API UKataLockOnData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** 비어 있으면 StateTree가 고른 배치를 유지하고, 지정하면 락온 동안 이 배치 데이터로 교체한다. 블렌드는 BlendIn 설정을 따른다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UKataCameraData> CameraData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lock On", meta = (ShowOnlyInnerProperties))
    FKataLockOnFramingSettings Settings;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;

    /** 블렌드 곡선이 [0, 1]에서 단조 증가하고 0→0, 1→1에 가까운지 표본으로 확인해 경고한다. 매니저 기본값 검증에도 쓴다. */
    static void ValidateBlendCurve(const UCurveFloat* Curve, class FDataValidationContext& Context);
#endif
};
