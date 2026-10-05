#pragma once

#include "CoreMinimal.h"
#include "KataCameraTypes.generated.h"

class AActor;
class APlayerController;
class UKataCameraData;
class UKataCameraRailComponent;
class USceneComponent;

/** 현재 프레임의 레일 선택·평가 결과. 실패하면 Spline 배치는 Boom Arm으로 대체한다. */
enum class EKataCameraRailStatus : uint8
{
    NotRequested,
    Ready,
    InvalidTag,
    Missing,
    Duplicate,
    Closed,
    InvalidLength,
    InvalidPitchRange,
    InvalidSample
};

/**
 * 카메라 데이터 전환의 가중치 곡선.
 *
 * 블렌드 중 카메라 Pitch가 목표를 넘거나 되돌아가지 않도록 단조 증가하는 곡선만 둔다. Back·Elastic·스프링은 지원하지 않는다.
 */
UENUM(BlueprintType)
enum class EKataCameraBlendCurve : uint8
{
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut
};

/** 전환 중 궤도 오프셋을 섞는 방법. */
UENUM(BlueprintType)
enum class EKataCameraOffsetBlend : uint8
{
    /** 카메라 Yaw 공간의 오프셋을 직선으로 섞는다. 캐릭터 기준 직선 경로이며 기본값이다. */
    Linear,
    /** 거리는 직선으로, 방향은 Slerp로 섞는다. 직선 경로가 피벗을 스치는 전환(어깨 전환 등)에서만 쓴다. */
    DirectionSlerp
};

/**
 * 카메라 Feature가 끼는 파이프라인 단계. 값 순서가 실행 순서다.
 *
 * 배치는 Feature 단계가 아니라 카메라 데이터의 Placement가 Rotation과 Framing 사이에서 수행한다.
 * 흔들림·FOV 펀치 같은 효과는 파이프라인이 끝난 뒤 엔진 UCameraModifier가 처리하므로 여기에 두지 않는다.
 */
UENUM(BlueprintType)
enum class EKataCameraStage : uint8
{
    /** 카메라가 바라볼 회전을 정한다. 기본값은 컨트롤 회전이며, 락온 같은 회전 드라이버가 덮어쓴다. */
    Rotation,
    /** 배치된 카메라의 구도를 보정한다. 락온 구도, 오프셋, FOV가 여기에 속한다. */
    Framing,
    /** 최종 위치를 제약한다. 장애물 Shrink가 여기에 속한다. */
    Constraint,
    /** 확정된 포즈를 바꾸지 않고 그에 반응한다. 디더링이 여기에 속한다. */
    Reaction
};

/**
 * 한 프레임의 카메라 파이프라인 단계들이 주고받는 값.
 *
 * 카메라 매니저가 매 프레임 스택에 만들고 파이프라인이 끝나면 버린다. 포인터 멤버는 그 프레임 동안만 유효하므로
 * Feature가 저장하지 않는다.
 */
struct FKataCameraPipelineContext
{
    /** 카메라를 조종하는 플레이어 컨트롤러. */
    APlayerController* PlayerController = nullptr;

    /** 피벗 기준이 되는 뷰 타깃. 파이프라인은 폰일 때만 실행된다. */
    AActor* ViewTarget = nullptr;

    /** 이번 프레임에 적용하는 카메라 데이터. 파이프라인이 실행되는 동안 null이 아니다. */
    const UKataCameraData* CameraData = nullptr;

    float DeltaTime = 0.0f;

    /** Framework가 전달한 락온 지점. 카메라는 엔진 SceneComponent 계약만 사용한다. */
    USceneComponent* LockFocus = nullptr;
    /** 블렌드된 락온 초점. 타겟 변경 중에는 이전 지점에서 새 지점으로 이동하고, 해제 중에는 마지막 위치에 머문다. */
    FVector LockFocusLocation = FVector::ZeroVector;
    bool bLockOnActive = false;
    /** 획득·해제 시 구도 보정의 0~1 가중치. 타겟 변경 중에는 1을 유지한다. */
    float LockOnWeight = 0.0f;
    /** 블렌드된 좌우 정렬. +1이면 타겟이 플레이어 오른쪽, -1이면 왼쪽에 보인다. 전환 중에는 그 사이 값이다. */
    float LockOnSide = 1.0f;

    /** 피벗 래그로 피벗과 카메라를 옮긴 양. Framing 이후 단계가 폰 위치 대신 래그된 기준을 쓸 때 더한다. */
    FVector PivotLagOffset = FVector::ZeroVector;

    /** 거리 곡선으로 구한 Boom Arm 거리 배율. 1이면 그대로다. Framing이 락온 가중치만큼 적용한다. */
    float LockOnDistanceScale = 1.0f;

    /** 락온 Framing이 화면 위치에 맞춘 조준점. 피벗과 락온 초점을 잇는 선 위의 LookAtAlpha 지점이다. 락온 가중치가 0이면 쓰지 않는다. */
    FVector LockOnAimPoint = FVector::ZeroVector;

    /** 카메라가 궤도를 도는 기준점. 정상 Spline은 컴포넌트 원점, Boom Arm은 폰 위치와 배치의 PivotOffset, Spline 대체 배치는 폰 위치를 쓴다. */
    FVector PivotLocation = FVector::ZeroVector;

    /** Rotation 단계의 결과. 배치가 이 회전을 기준으로 카메라 위치를 정한다. */
    FRotator ViewRotation = FRotator::ZeroRotator;

    /** 배치 이후 단계가 읽고 고치는 카메라 위치. */
    FVector CameraLocation = FVector::ZeroVector;

    /** 배치 이후 단계가 읽고 고치는 카메라 회전. */
    FRotator CameraRotation = FRotator::ZeroRotator;

    /** 수평 시야각(도). */
    float FieldOfView = 90.0f;

    /** 매니저가 선택한 레일. Ready일 때만 사용하며 이 프레임 밖으로 저장하지 않는다. */
    const UKataCameraRailComponent* Rail = nullptr;
    EKataCameraRailStatus RailStatus = EKataCameraRailStatus::NotRequested;

    /** Spline 평가에 사용한 정규화된 Pitch와 Spline 길이상의 거리. */
    float RailAlpha = 0.0f;
    float RailDistance = 0.0f;

    /** Placement 결과의 피벗 기준 위치와 조준점 오프셋. 카메라 Yaw 공간이며 이후 Feature 보정은 포함하지 않는다. */
    FVector OrbitOffset = FVector::ZeroVector;
    FVector AimOffset = FVector::ZeroVector;
};

/** 디버그 표시용 블렌드 레이어 정보. */
struct FKataCameraDebugLayer
{
    FString CameraDataName;
    float Weight = 0.0f;
    float RemainingTime = 0.0f;
    /** 스택 상한을 넘어 바닥 레이어 둘을 고정 결과로 합친 레이어인지. */
    bool bFrozen = false;
};

/**
 * 마지막으로 실행한 파이프라인의 결과 사본. GameplayDebugger 카테고리가 읽는다.
 *
 * 값만 담아 UObject 수명과 무관하게 보관할 수 있다.
 */
struct FKataCameraDebugSnapshot
{
    /** 마지막 갱신에서 Kata 파이프라인이 실행되었는지. false이면 엔진 기본 계산을 사용한 것이다. */
    bool bPipelineActive = false;

    FString CameraDataName;
    FString PlacementName;
    FVector PivotLocation = FVector::ZeroVector;
    FRotator ViewRotation = FRotator::ZeroRotator;
    FVector CameraLocation = FVector::ZeroVector;
    FRotator CameraRotation = FRotator::ZeroRotator;
    float FieldOfView = 0.0f;

    FString RailTag;
    FString RailDiagnostic;
    bool bUsingSpline = false;
    float RailAlpha = 0.0f;
    float RailDistance = 0.0f;
    float RailLength = 0.0f;
    FVector OrbitOffset = FVector::ZeroVector;
    FVector AimOffset = FVector::ZeroVector;

    /** 아래에서 위 순서의 블렌드 레이어. 맨 마지막이 가장 최근에 요청한 데이터다. */
    TArray<FKataCameraDebugLayer> Layers;

    FString StateTreeName;
    bool bStateTreeRunning = false;

    /** 락온 상태. 해제 블렌드 중에는 bLockOnActive가 false이고 가중치가 남는다. 데이터 이름이 비면 매니저 기본값을 쓴다. */
    bool bLockOnActive = false;
    float LockOnWeight = 0.0f;
    float LockOnSide = 1.0f;
    FString LockOnDataName;

    /** 피벗 래그로 피벗과 카메라를 옮긴 양. */
    FVector PivotLagOffset = FVector::ZeroVector;

    /** 락온 조준선. 피벗(래그 적용 후, Shrink 전)에서 블렌드된 초점까지의 선과 그 위의 조준점이다. 락온 가중치가 0이면 표시하지 않는다. */
    FVector LockOnLineStart = FVector::ZeroVector;
    FVector LockOnLineEnd = FVector::ZeroVector;
    FVector LockOnAimPoint = FVector::ZeroVector;
    float LockOnLookAtAlpha = 1.0f;

    /** 거리 곡선으로 구한 Pitch 오프셋(도)과 Boom Arm 거리 배율. */
    float LockOnPitchOffset = 0.0f;
    float LockOnDistanceScale = 1.0f;

#if WITH_GAMEPLAY_DEBUGGER
    /** 카메라 Yaw 공간의 곡선 사본. 화면의 XZ 투영에 쓰며 월드 좌표는 포함하지 않는다. */
    TArray<FVector> RailSamples;
#endif
};
