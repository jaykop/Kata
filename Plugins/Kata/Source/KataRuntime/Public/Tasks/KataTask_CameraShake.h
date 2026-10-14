#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "Runtime/KataTaskInstance.h"
#include "Templates/SubclassOf.h"
#include "KataTask_CameraShake.generated.h"

class APlayerCameraManager;
class UCameraShakeBase;

/** 카메라 흔들림을 적용할 대상. */
UENUM(BlueprintType)
enum class EKataCameraShakeTarget : uint8
{
    /** Avatar를 조종하는 로컬 플레이어의 카메라만 흔든다. AI가 실행하면 아무도 흔들리지 않는다. */
    AvatarCamera,
    /** Avatar 위치를 진원으로 반경 안에 있는 모든 로컬 플레이어의 카메라를 거리에 따라 줄여 흔든다. */
    LocalPlayersInRadius
};

/**
 * 태스크가 시작될 때 카메라를 흔든다.
 *
 * 맞는 순간의 흔들림은 히트 처리기의 Gameplay Cue가 맡는다. 이 태스크는 내려찍기 착지처럼 타임라인 시각에 맞춘 흔들림을 다룬다.
 * 정상 완료로 끝나면 Completed Policy를 따른다. 실행 중인 태스크가 중단·취소·Kata 종료로 끝나면 블렌드 아웃으로 멈춘다.
 * 이미 정상 완료한 태스크가 남긴 흔들림은 이후 Kata 종료로 다시 정리하지 않는다.
 * 액션 편집기 프리뷰 월드에는 PlayerController가 없어 흔들리지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Camera Shake", KataTaskCategory = "FX"))
class KATARUNTIME_API UKataTask_CameraShake : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_CameraShake();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Shake")
    TSubclassOf<UCameraShakeBase> ShakeClass;

    /** 흔들림 세기 배율. 반경 모드에서는 거리 감쇠를 이 값에 곱한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Shake", meta = (ClampMin = "0.0"))
    float Scale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Shake")
    EKataCameraShakeTarget Target = EKataCameraShakeTarget::AvatarCamera;

    /** 이 거리 안의 카메라는 Scale 그대로 흔든다. 거리는 진원에서 카메라 위치까지다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Shake",
        meta = (ClampMin = "0.0", Units = "cm", EditCondition = "Target == EKataCameraShakeTarget::LocalPlayersInRadius", EditConditionHides))
    float InnerRadius = 300.0f;

    /** Inner Radius보다 먼 카메라는 이 거리 이상이면 흔들지 않는다. Inner Radius 이상이어야 한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Shake",
        meta = (ClampMin = "0.0", Units = "cm", EditCondition = "Target == EKataCameraShakeTarget::LocalPlayersInRadius", EditConditionHides))
    float OuterRadius = 1500.0f;

    /** Inner Radius와 Outer Radius 사이의 감쇠 곡선 지수. 1이면 선형이다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Shake",
        meta = (ClampMin = "0.0", EditCondition = "Target == EKataCameraShakeTarget::LocalPlayersInRadius", EditConditionHides))
    float Falloff = 1.0f;

    /** 정상 완료로 끝났을 때 진행 중인 흔들림을 둘지 블렌드 아웃으로 멈출지. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Shake")
    EKataEffectEndPolicy CompletedPolicy = EKataEffectEndPolicy::KeepPlaying;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;

    /** 지속 시간이 무한한 흔들림이면 true. Keep Playing으로는 끝나지 않으므로 Stop으로 처리한다. */
    bool IsInfiniteShake() const;

#if WITH_EDITOR
    /** 무한 흔들림에 Keep Playing을 지정하면 경고한다. */
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

/** 이 태스크가 시작한 흔들림 하나. 카메라 매니저와 셰이크 인스턴스는 다른 곳이 소유하므로 약한 참조로 둔다. */
struct FKataStartedCameraShake
{
    TWeakObjectPtr<APlayerCameraManager> CameraManager;
    TWeakObjectPtr<UCameraShakeBase> Shake;
};

/**
 * 카메라 흔들림의 실행별 상태.
 *
 * 카메라 매니저는 끝난 셰이크 인스턴스를 클래스별 풀에 모았다가 다음 시작에 다시 쓴다.
 * 끝난 인스턴스를 계속 가리키면 다른 곳에서 새로 시작한 흔들림을 멈출 수 있으므로, Tick마다 끝난 항목을 놓는다.
 * Tick 사이에 같은 객체가 종료된 뒤 재사용되면 이전 재생과 새 재생을 구분하지 못한다.
 */
UCLASS()
class KATARUNTIME_API UKataTaskInstance_CameraShake : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskTick_Implementation(float DeltaTime) override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    void StartShakeOn(APlayerCameraManager* CameraManager, float ShakeScale);

    /** 이미 끝나 풀로 돌아갔을 수 있는 항목을 지운다. */
    void ReleaseFinishedShakes();

    TArray<FKataStartedCameraShake> StartedShakes;
};
