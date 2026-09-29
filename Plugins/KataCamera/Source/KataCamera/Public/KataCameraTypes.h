#pragma once

#include "CoreMinimal.h"
#include "KataCameraTypes.generated.h"

class AActor;
class APlayerController;
class UKataCameraData;

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

    /** 카메라가 궤도를 도는 기준점. 뷰 타깃 위치에 카메라 데이터의 PivotOffset을 더한 값이다. */
    FVector PivotLocation = FVector::ZeroVector;

    /** Rotation 단계의 결과. 배치가 이 회전을 기준으로 카메라 위치를 정한다. */
    FRotator ViewRotation = FRotator::ZeroRotator;

    /** 배치 이후 단계가 읽고 고치는 카메라 위치. */
    FVector CameraLocation = FVector::ZeroVector;

    /** 배치 이후 단계가 읽고 고치는 카메라 회전. */
    FRotator CameraRotation = FRotator::ZeroRotator;

    /** 수평 시야각(도). */
    float FieldOfView = 90.0f;
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
};
