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
    /**
     * 뷰 타깃 위치에서 피벗까지의 오프셋.
     * X·Y는 카메라 Yaw 기준(X 앞, Y 오른쪽)으로 적용해 어깨 너머 오프셋이 화면의 같은 쪽에 머물게 한다. Z는 월드 위쪽이다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pivot", meta = (Units = "Centimeters"))
    FVector PivotOffset = FVector(0.0, 0.0, 60.0);

    /** 수평 시야각. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lens", meta = (ClampMin = "5.0", ClampMax = "170.0", Units = "Degrees"))
    float FieldOfView = 90.0f;

    /** 시점 입력으로 내려볼 수 있는 최소 피치. 카메라 매니저의 ViewPitchMin에 반영한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "-89.9", ClampMax = "89.9", Units = "Degrees"))
    float PitchMin = -70.0f;

    /** 시점 입력으로 올려볼 수 있는 최대 피치. 카메라 매니저의 ViewPitchMax에 반영한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "-89.9", ClampMax = "89.9", Units = "Degrees"))
    float PitchMax = 60.0f;

    /** 카메라를 어디에 둘지 정하는 배치 방식. 비어 있으면 카메라 매니저가 엔진 기본 계산을 쓴다. */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Placement")
    TObjectPtr<UKataCameraPlacement> Placement;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
