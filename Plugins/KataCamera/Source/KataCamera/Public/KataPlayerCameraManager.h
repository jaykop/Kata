#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "KataCameraStateTree.h"
#include "KataCameraTypes.h"
#include "StateTreeInstanceData.h"
#include "StateTreeReference.h"
#include "KataPlayerCameraManager.generated.h"

class APawn;
class UAbilitySystemComponent;
class UKataCameraData;
class UKataCameraFeature;
class UKataCameraPlacement_Spline;
struct FStateTreeExecutionContext;

/** 한 블렌드 레이어의 궤도 공간 결과. 섞는 단위이며 위치와 시선은 섞은 뒤 다시 계산한다. */
struct FKataCameraLayerPose
{
    /** 월드 피벗. */
    FVector Pivot = FVector::ZeroVector;
    /** 피벗 기준 카메라 위치. 카메라 Yaw 공간이다. */
    FVector OrbitOffset = FVector::ZeroVector;
    /** 피벗 기준 조준점. 카메라 Yaw 공간이다. */
    FVector AimOffset = FVector::ZeroVector;
    float FieldOfView = 90.0f;
    float PitchMin = -70.0f;
    float PitchMax = 60.0f;
};

/** 레이어별 레일 경고를 같은 사유로 반복하지 않기 위한 마지막 상태. */
struct FKataCameraRailDiagnosticState
{
    TWeakObjectPtr<APawn> Pawn;
    TWeakObjectPtr<const UKataCameraPlacement_Spline> Placement;
    FName Tag;
    EKataCameraRailStatus Status = EKataCameraRailStatus::NotRequested;
};

/**
 * 블렌드 스택의 레이어 하나. 매 프레임 자기 카메라 데이터로 다시 평가한다.
 * 스택 상한을 넘어 바닥 두 레이어를 합친 레이어는 그 순간의 결과를 고정해 쓴다.
 */
USTRUCT()
struct FKataCameraBlendLayer
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UKataCameraData> CameraData;

    float BlendTime = 0.0f;
    float Elapsed = 0.0f;
    EKataCameraBlendCurve BlendCurve = EKataCameraBlendCurve::EaseInOut;
    EKataCameraOffsetBlend OffsetBlend = EKataCameraOffsetBlend::Linear;

    bool bFrozen = false;
    FKataCameraLayerPose FrozenPose;
    /** 고정 레이어의 피벗은 폰을 따라가도록 폰 위치 기준으로 보관한다. */
    FVector FrozenPivotFromPawn = FVector::ZeroVector;

    FKataCameraRailDiagnosticState RailDiagnostic;

    /** 곡선을 적용한 0~1 가중치. 단조 증가한다. */
    float GetWeight() const;
};

/**
 * Kata 카메라 파이프라인을 실행하는 플레이어 카메라 매니저.
 *
 * 매 프레임 회전 결정 → Rotation Feature → 블렌드 레이어별 배치 → 궤도 공간 블렌드 → Framing·Constraint·Reaction Feature 순으로 포즈를 만든다.
 * 흔들림 같은 효과는 그 뒤에 엔진 UCameraModifier가 적용한다.
 * 카메라 상태는 플레이어에 속하므로 빙의나 리스폰으로 폰이 바뀌어도 유지되고, 다음 갱신부터 새 폰을 피벗으로 쓴다.
 *
 * CameraStateTree가 있으면 매니저가 직접 실행해 Status 태그에 맞는 카메라 데이터를 고른다. 없거나 폰에 ASC가 없으면 DefaultCameraData를 쓴다.
 * 뷰 타깃이 폰이 아니거나 평가할 수 있는 카메라 데이터가 없으면 엔진 기본 계산을 쓴다.
 * 카메라 액터를 보는 경우와 디버그 카메라 스타일은 엔진이 먼저 처리하므로 파이프라인을 거치지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Player Camera Manager"))
class KATACAMERA_API AKataPlayerCameraManager : public APlayerCameraManager
{
    GENERATED_BODY()

public:
    virtual void InitializeFor(APlayerController* PC) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** 가장 최근에 요청되어 블렌드 스택 맨 위에 있는 카메라 데이터. 스택이 비었으면 DefaultCameraData다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Camera")
    UKataCameraData* GetActiveCameraData() const;

    /** 마지막 파이프라인 결과 사본. 디버그 표시용이다. */
    const FKataCameraDebugSnapshot& GetDebugSnapshot() const { return DebugSnapshot; }

    /** 초기화된 Feature를 실행 순서대로 돌려준다. 디버그 표시용이다. */
    const TArray<TObjectPtr<UKataCameraFeature>>& GetOrderedFeatures() const { return OrderedFeatures; }

protected:
    virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime) override;

    /** 카메라 StateTree가 없거나 실행할 수 없을 때, 그리고 트리가 첫 요청을 하기 전에 적용하는 카메라 데이터. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Camera")
    TObjectPtr<UKataCameraData> DefaultCameraData;

    /** Status 태그로 카메라 데이터를 고르는 StateTree. 비어 있으면 DefaultCameraData만 쓴다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Camera", meta = (Schema = "/Script/KataCamera.KataCameraStateTreeSchema"))
    FStateTreeReference CameraStateTree;

    /** 이 매니저가 소유하는 Feature. 플레이어마다 인스턴스가 따로 생긴다. */
    UPROPERTY(EditAnywhere, Instanced, Category = "Kata|Camera")
    TArray<TObjectPtr<UKataCameraFeature>> Features;

private:
    static constexpr int32 MaxBlendLayers = 4;

    void RunFeatures(EKataCameraStage Stage, FKataCameraPipelineContext& Context);

    /** 섞인 피치 제한을 시점 입력 제한에 반영한다. 적용할 데이터가 없으면 클래스 기본값으로 되돌린다. */
    void ApplyPitchLimits(const FKataCameraLayerPose* Pose);

    /** 폰·ASC가 바뀌면 트리를 다시 시작하고, 대기 중인 이벤트가 있을 때만 갱신한다. */
    void UpdateStateTree(APawn* ViewPawn, float DeltaTime);
    void StopStateTree();
    bool SetupStateTreeContext(FStateTreeExecutionContext& Context, APawn* Pawn, UAbilitySystemComponent* AbilitySystem);

    /** 새 레이어를 맨 위에 올린다. 맨 위와 같은 데이터면 무시하고, 스택이 비었으면 블렌드 없이 적용한다. */
    void PushBlendLayer(UKataCameraData* CameraData, float BlendTime, EKataCameraBlendCurve BlendCurve, EKataCameraOffsetBlend OffsetBlend);

    /** 레이어의 Placement를 평가해 궤도 공간 결과를 만든다. 평가할 수 없으면 false다. */
    bool EvaluateLayer(FKataCameraBlendLayer& Layer, APawn* ViewPawn, const FKataCameraPipelineContext& BaseContext,
        FKataCameraPipelineContext& OutContext, FKataCameraLayerPose& OutPose);

    /** 현재 폰에서 정확히 하나인 레일을 찾는다. 실행 중 태그·컴포넌트 변경도 다음 프레임에 반영한다. */
    void ResolveRail(APawn* ViewPawn, const UKataCameraPlacement_Spline* Placement, FKataCameraPipelineContext& Context) const;

    /** 폰·배치·태그·실패 사유가 달라졌을 때만 경고한다. */
    void UpdateRailDiagnostic(APawn* ViewPawn, const UKataCameraPlacement_Spline* Placement,
        const FKataCameraPipelineContext& Context, FKataCameraRailDiagnosticState& State) const;

    /** Features 중 유효한 것을 단계, 우선순위 순으로 정렬한 실행 목록. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UKataCameraFeature>> OrderedFeatures;

    /** 아래에서 위 순서의 블렌드 레이어. 가중치 1에 도달한 레이어보다 아래는 제거한다. */
    UPROPERTY(Transient)
    TArray<FKataCameraBlendLayer> BlendLayers;

    UPROPERTY(Transient)
    FStateTreeInstanceData StateTreeInstanceData;

    /** 트리 태스크가 채우는 요청. 매니저가 외부 데이터로 제공한다. */
    UPROPERTY(Transient)
    FKataCameraStateTreeRequest StateTreeRequest;

    uint32 AppliedRequestSerial = 0;
    bool bStateTreeRunning = false;
    TWeakObjectPtr<APawn> StateTreePawn;
    TWeakObjectPtr<UAbilitySystemComponent> StateTreeAbilitySystem;
    /** 시작에 실패한 폰. 같은 폰으로 매 프레임 다시 시도하지 않는다. */
    TWeakObjectPtr<APawn> StateTreeFailedPawn;

    /** Placement가 비었다는 경고를 이미 남긴 데이터. 같은 경고를 매 프레임 반복하지 않는다. */
    TWeakObjectPtr<const UKataCameraData> MissingPlacementWarned;

    FKataCameraDebugSnapshot DebugSnapshot;
};
