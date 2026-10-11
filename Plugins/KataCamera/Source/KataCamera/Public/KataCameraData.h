#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KataCameraData.generated.h"

class UKataCameraPlacement;

/**
 * 한 카메라 상태에서 쓰는 설정. 피벗, 렌즈, 피치 제한과 배치 방식을 담는다.
 *
 * 여러 플레이어가 공유하는 에셋이므로 실행 중 값을 저장하지 않는다. 어떤 데이터를 적용할지는 카메라 매니저가 정한다.
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "Kata Camera Data"))
class KATACAMERA_API UKataCameraData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** 수평 시야각. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lens", meta = (ClampMin = "5.0", ClampMax = "170.0", Units = "Degrees"))
    float FieldOfView = 90.0f;

    /** 최소 입력 Pitch. ViewPitchMin에 반영하며 Spline에서는 시작점(정규화 값 0)에 대응한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "-89.9", ClampMax = "89.9", Units = "Degrees"))
    float PitchMin = -70.0f;

    /** 최대 입력 Pitch. ViewPitchMax에 반영하며 Spline에서는 끝점(정규화 값 1)에 대응한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "-89.9", ClampMax = "89.9", Units = "Degrees"))
    float PitchMax = 60.0f;

    /**
     * 피벗이 뷰 타깃의 수평 이동을 따라가는 지연 시간의 기준(초). 0이면 즉시 따른다.
     * 임계 감쇠 보간으로 루트 모션의 짧은 좌우 흔들림을 완화한다. 목표 위치에 정확히 도달하는 시간을 지정하는 값은 아니다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lag", meta = (ClampMin = "0.0", Units = "Seconds"))
    float PivotLagTimeHorizontal = 0.12f;

    /** 피벗이 뷰 타깃의 수직 이동을 따라가는 지연 시간의 기준(초). 0이면 즉시 따르며, 수평 값보다 짧게 두면 점프·낙하를 더 빠르게 따라간다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lag", meta = (ClampMin = "0.0", Units = "Seconds"))
    float PivotLagTimeVertical = 0.05f;

    /** 래그로 피벗이 실제 위치에서 떨어질 수 있는 최대 거리. 0이면 제한하지 않는다. 빠른 돌진에서 캐릭터가 화면 밖으로 밀리지 않게 한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lag", meta = (ClampMin = "0.0", Units = "Centimeters"))
    float PivotLagMaxDistance = 150.0f;

    /** 카메라를 어디에 둘지 정하는 배치 방식. 비어 있으면 카메라 매니저가 엔진 기본 계산을 쓴다. */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Placement")
    TObjectPtr<UKataCameraPlacement> Placement;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
