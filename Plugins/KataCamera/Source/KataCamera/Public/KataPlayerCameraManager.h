#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "KataCameraStateTree.h"
#include "KataCameraTypes.h"
#include "KataLockOnData.h"
#include "StateTreeInstanceData.h"
#include "StateTreeReference.h"
#include "KataPlayerCameraManager.generated.h"

class APawn;
class UAbilitySystemComponent;
class UKataCameraData;
class UKataCameraFeature;
class UKataCameraPlacement_Spline;
class UCurveFloat;
class USceneComponent;
class UKataCameraFeature_LockOnRotation;
class UKataCameraFeature_LockOnFraming;
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
    /** 지정하면 BlendCurve 대신 이 곡선으로 가중치를 구한다. 락온 BlendIn 곡선이 여기에 들어온다. */
    UPROPERTY()
    TObjectPtr<UCurveFloat> BlendCurveAsset;
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
UCLASS(PrioritizeCategories = "Kata|Camera", meta = (DisplayName = "Kata Player Camera Manager"))
class KATACAMERA_API AKataPlayerCameraManager : public APlayerCameraManager
{
    GENERATED_BODY()

public:
    AKataPlayerCameraManager();

    /**
     * 락온 초점을 바꾼다. nullptr이면 해제하며, 지점은 약한 참조로 보관한다. Framework가 폰 변경 시에도 호출한다.
     * 획득·타겟 변경은 BlendIn 시간과 곡선으로, 해제는 같은 시간 동안 기본 Ease In-Out으로 진행 중인 값에서 이어서 블렌드한다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Camera|Lock On")
    void SetLockOnFocus(USceneComponent* Focus, UKataLockOnData* Data = nullptr);

    UFUNCTION(BlueprintPure, Category = "Kata|Camera|Lock On")
    bool HasLockOnFocus() const;

    /** 현재 프레임에 블렌드된 설정. Feature가 읽으며 공유 에셋을 수정하지 않는다. */
    const FKataLockOnFramingSettings& GetLockOnSettings() const { return CurrentLockOnSettings; }

    /** 이번 프레임의 락온 궤도 회전. 락온 중에만 의미가 있으며 Rotation Feature가 컨트롤 회전에 기록한다. */
    const FRotator& GetLockOnViewRotation() const { return LockOnViewRotation; }

    virtual void InitializeFor(APlayerController* PC) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

    /** 가장 최근에 요청되어 블렌드 스택 맨 위에 있는 카메라 데이터. 스택이 비었으면 DefaultCameraData다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Camera")
    UKataCameraData* GetActiveCameraData() const;

    /** 마지막 파이프라인 결과 사본. 디버그 표시용이다. */
    const FKataCameraDebugSnapshot& GetDebugSnapshot() const { return DebugSnapshot; }

    /** 초기화된 Feature를 실행 순서대로 돌려준다. 디버그 표시용이다. */
    const TArray<TObjectPtr<UKataCameraFeature>>& GetOrderedFeatures() const { return OrderedFeatures; }

protected:
    /** 락온 정렬·구도·블렌드 기본값. 지점이 락온 데이터를 쓰지 않거나 비어 있을 때 적용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Camera|Lock On")
    FKataLockOnFramingSettings DefaultLockOnSettings;

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
    /** 현재 지점의 락온 데이터가 있으면 그 설정을, 없으면 기본값을 골라 범위를 정리한다. */
    FKataLockOnFramingSettings ResolveLockOnSettings() const;

    /** 전환 진행도·가중치·초점·설정·좌우 값과 락온 회전을 갱신한다. 레이어 선택 뒤, Rotation 단계 전에 호출한다. */
    void UpdateLockOn(float DeltaTime, APawn* ViewPawn);

    /** 정렬 설정에 맞는 좌우 값(+1 오른쪽, -1 왼쪽)을 고른다. Auto는 View 기준으로 타겟이 있는 쪽이다. */
    float ChooseLockOnSide(const FKataLockOnFramingSettings& Settings, const FRotator& View, const FVector& FocusLocation) const;

    /** 좌우 전환을 시작한다. bInstant면 블렌드 없이 바로 적용한다. */
    void StartLockOnSideTransition(float NewSide, bool bInstant);

    TWeakObjectPtr<USceneComponent> LockOnFocus;
    UPROPERTY(Transient)
    TObjectPtr<UKataLockOnData> LockOnData;
    UPROPERTY(VisibleAnywhere, Instanced, Category = "Kata|Camera|Lock On")
    TObjectPtr<UKataCameraFeature_LockOnRotation> LockOnRotation;
    UPROPERTY(VisibleAnywhere, Instanced, Category = "Kata|Camera|Lock On")
    TObjectPtr<UKataCameraFeature_LockOnFraming> LockOnFraming;

    /** 블렌드된 현재 설정. 해제 후에도 곡선 참조가 남으므로 GC가 추적하게 한다. */
    UPROPERTY(Transient)
    FKataLockOnFramingSettings CurrentLockOnSettings;

    /** 전환을 시작한 순간의 값. 진행 중인 블렌드 중간에서 다음 전환을 시작해도 끊기지 않게 한다. */
    UPROPERTY(Transient)
    FKataLockOnFramingSettings TransitionFromSettings;
    UPROPERTY(Transient)
    TObjectPtr<UCurveFloat> TransitionCurve;
    FVector TransitionFromFocus = FVector::ZeroVector;
    FRotator TransitionFromRotation = FRotator::ZeroRotator;
    float TransitionFromWeight = 0.0f;
    float TransitionDuration = 0.0f;
    float TransitionElapsed = 0.0f;
    bool bTransitionActive = false;

    FVector CurrentFocusLocation = FVector::ZeroVector;
    FRotator LockOnViewRotation = FRotator::ZeroRotator;
    float LockOnWeight = 0.0f;

    /** 좌우 값은 타겟 변경 없이도 Auto 판정으로 바뀌므로 전환을 따로 관리한다. 시간과 곡선은 현재 설정의 BlendIn을 쓴다. */
    float LockOnSide = 1.0f;

    /** 거리 곡선으로 구한 값. 타겟 변경 중에는 이전·새 설정의 값을 같은 진행도로 섞고, 해제 중에는 마지막 값을 유지한다. */
    float LockOnPitchOffset = 0.0f;
    float LockOnDistanceScale = 1.0f;
    float SideFrom = 1.0f;
    float SideTarget = 1.0f;
    float SideElapsed = 0.0f;
    bool bSideTransitionActive = false;

    /** 초점이 바뀌어 락온 데이터의 CameraData 교체 여부를 다시 판단해야 한다. */
    bool bLockOnCameraSelectionDirty = false;

    /** 블렌드가 끝난 뒤 락온 회전 감쇠의 각속도(도/초). */
    double LockOnYawRate = 0.0;
    double LockOnPitchRate = 0.0;

    /**
     * 폰 위치를 임계 감쇠로 따라가는 래그 기준점을 갱신하고 PivotLagOffset을 구한다. 락온 회전 전에 호출한다.
     * 피벗 자체를 따라가면 Yaw 공간 피벗 오프셋이 시점 회전에 따라 원을 그리는 움직임까지 늦어져 락온 회전과 되먹임이 생긴다.
     * 폰이 바뀌거나 파이프라인이 끊기면 다시 붙인다.
     */
    void UpdatePivotLag(APawn* ViewPawn, float DeltaTime);

    /** 블렌드된 피벗과 카메라를 PivotLagOffset만큼 함께 옮긴다. 시선 방향은 바뀌지 않는다. */
    void ApplyPivotLag(FKataCameraPipelineContext& Context) const;

    TWeakObjectPtr<APawn> PivotLagPawn;
    FVector LaggedAnchor = FVector::ZeroVector;
    FVector PivotLagRate = FVector::ZeroVector;
    /** 래그 기준점과 실제 폰 위치의 차이. 락온 회전과 Framing이 폰 대신 래그된 기준을 쓸 때 더한다. */
    FVector PivotLagOffset = FVector::ZeroVector;
    bool bPivotLagValid = false;
    /** 직전 프레임에 배치가 정한 피벗의 폰 기준 높이. 락온 방향을 카메라가 실제로 도는 피벗 높이에서 재는 데 쓴다. */
    float PivotHeightFromPawn = 0.0f;

    static constexpr int32 MaxBlendLayers = 4;

    void RunFeatures(EKataCameraStage Stage, FKataCameraPipelineContext& Context);

    /** 섞인 피치 제한을 시점 입력 제한에 반영한다. 적용할 데이터가 없으면 클래스 기본값으로 되돌린다. */
    void ApplyPitchLimits(const FKataCameraLayerPose* Pose);

    /** 폰·ASC가 바뀌면 트리를 다시 시작하고, 대기 중인 이벤트가 있을 때만 갱신한다. */
    void UpdateStateTree(APawn* ViewPawn, float DeltaTime);
    void StopStateTree();
    bool SetupStateTreeContext(FStateTreeExecutionContext& Context, APawn* Pawn, UAbilitySystemComponent* AbilitySystem);

    /** 새 레이어를 맨 위에 올린다. 맨 위와 같은 데이터면 무시하고, 스택이 비었으면 블렌드 없이 적용한다. CurveAsset이 있으면 BlendCurve보다 우선한다. */
    void PushBlendLayer(UKataCameraData* CameraData, float BlendTime, EKataCameraBlendCurve BlendCurve, EKataCameraOffsetBlend OffsetBlend,
        UCurveFloat* CurveAsset = nullptr);

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
