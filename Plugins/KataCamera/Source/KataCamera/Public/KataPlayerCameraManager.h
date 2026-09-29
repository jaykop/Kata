#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "KataCameraTypes.h"
#include "KataPlayerCameraManager.generated.h"

class UKataCameraData;
class UKataCameraFeature;

/**
 * Kata 카메라 파이프라인을 실행하는 플레이어 카메라 매니저.
 *
 * 매 프레임 회전 결정 → Rotation Feature → 카메라 데이터의 배치 → Framing·Constraint·Reaction Feature 순으로 포즈를 만든다.
 * 흔들림 같은 효과는 그 뒤에 엔진 UCameraModifier가 적용한다.
 * 카메라 상태는 플레이어에 속하므로 빙의나 리스폰으로 폰이 바뀌어도 유지되고, 다음 갱신부터 새 폰을 피벗으로 쓴다.
 *
 * 뷰 타깃이 폰이 아니거나 적용할 카메라 데이터 또는 그 Placement가 없으면 엔진 기본 계산을 쓴다.
 * 카메라 액터를 보는 경우와 디버그 카메라 스타일은 엔진이 먼저 처리하므로 파이프라인을 거치지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Player Camera Manager"))
class KATACAMERA_API AKataPlayerCameraManager : public APlayerCameraManager
{
    GENERATED_BODY()

public:
    virtual void InitializeFor(APlayerController* PC) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** 이번 프레임에 적용하는 카메라 데이터. 상태별 선택이 생기기 전까지는 DefaultCameraData다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Camera")
    UKataCameraData* GetActiveCameraData() const;

    /** 마지막 파이프라인 결과 사본. 디버그 표시용이다. */
    const FKataCameraDebugSnapshot& GetDebugSnapshot() const { return DebugSnapshot; }

    /** 초기화된 Feature를 실행 순서대로 돌려준다. 디버그 표시용이다. */
    const TArray<TObjectPtr<UKataCameraFeature>>& GetOrderedFeatures() const { return OrderedFeatures; }

protected:
    virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime) override;

    /** 카메라 상태를 고르는 기능이 없을 때 적용하는 카메라 데이터. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Camera")
    TObjectPtr<UKataCameraData> DefaultCameraData;

    /** 이 매니저가 소유하는 Feature. 플레이어마다 인스턴스가 따로 생긴다. */
    UPROPERTY(EditAnywhere, Instanced, Category = "Kata|Camera")
    TArray<TObjectPtr<UKataCameraFeature>> Features;

private:
    void RunFeatures(EKataCameraStage Stage, FKataCameraPipelineContext& Context);

    /** 카메라 데이터의 피치 제한을 시점 입력 제한에 반영한다. 데이터가 없으면 클래스 기본값으로 되돌린다. */
    void ApplyPitchLimits(const UKataCameraData* CameraData);

    /** Features 중 유효한 것을 단계, 우선순위 순으로 정렬한 실행 목록. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UKataCameraFeature>> OrderedFeatures;

    /** 피치 제한을 마지막으로 반영한 데이터. 데이터가 바뀔 때만 다시 반영한다. */
    TWeakObjectPtr<const UKataCameraData> PitchLimitSource;

    /** Placement가 비었다는 경고를 이미 남긴 데이터. 같은 경고를 매 프레임 반복하지 않는다. */
    TWeakObjectPtr<const UKataCameraData> MissingPlacementWarned;

    bool bPitchLimitsApplied = false;

    FKataCameraDebugSnapshot DebugSnapshot;
};
