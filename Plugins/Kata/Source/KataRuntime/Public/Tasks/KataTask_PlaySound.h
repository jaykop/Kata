#pragma once

#include "Action/KataTask.h"
#include "AudioParameter.h"
#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_PlaySound.generated.h"

class UAudioComponent;
class USceneComponent;
class USoundAttenuation;
class USoundBase;

/**
 * 태스크가 시작될 때 사운드를 재생한다. Sound Wave, Sound Cue, MetaSound Source를 모두 받는다.
 *
 * 맞는 순간의 소리는 이 태스크가 아니라 히트 처리기의 Gameplay Cue가 맡는다.
 * 이 태스크는 휘두르는 소리처럼 액션 타임라인의 시각에 맞춰 내는 소리를 다룬다.
 *
 * 정상 완료로 끝나면 Completed Policy를 따른다. 실행 중인 태스크가 중단·취소·Kata 종료로 끝나면 Fade Out Time 동안 줄이며 멈춘다.
 * 이미 정상 완료한 태스크가 남긴 소리는 이후 Kata 종료로 다시 정리하지 않는다.
 * 월드가 오디오 재생을 허용하지 않으면 재생하지 않는다. 액션 편집기 프리뷰는 Seek하는 동안 이 허용을 끈다.
 * 프리뷰 월드에서는 UI 사운드로 재생하고 공간화를 끈다. 감쇠 설정은 별도로 해제하지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Play Sound", KataTaskCategory = "FX"))
class KATARUNTIME_API UKataTask_PlaySound : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_PlaySound();

    /** 재생할 사운드. MetaSound Source도 USoundBase이므로 같은 경로로 재생한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
    TObjectPtr<USoundBase> Sound;

    /**
     * 켜면 Avatar의 스켈레탈 메시에 붙여 움직임을 따라간다.
     * 끄면 시작 순간의 Attach Socket 위치에 두고 따라가지 않는다. 메시가 없으면 Avatar의 루트 컴포넌트를 기준으로 한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
    bool bAttachToMesh = true;

    /** 기준으로 삼을 메시 소켓. 비우면 메시 원점을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
    FName AttachSocket;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.0"))
    float VolumeMultiplier = 1.0f;

    /** 피치 배율. 0보다 커야 한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.01"))
    float PitchMultiplier = 1.0f;

    /** 비우면 사운드 에셋의 감쇠 설정을 그대로 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
    TObjectPtr<USoundAttenuation> AttenuationOverride;

    /**
     * 재생을 시작하기 전에 사운드에 넘길 입력 값. MetaSound Source의 입력 이름과 타입이 맞아야 반영된다.
     * 이 태스크는 입력 이름과 타입의 유효성을 직접 검사하지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
    TArray<FAudioParameter> AudioParameters;

    /** 정상 완료로 끝났을 때 재생 중인 소리를 둘지 멈출지. Stop이면 Fade Out Time 동안 줄인다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
    EKataEffectEndPolicy CompletedPolicy = EKataEffectEndPolicy::KeepPlaying;

    /** 소리를 멈출 때 줄이는 시간(초). 0이면 즉시 멈춘다. 중단으로 끝날 때도 이 값을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.0", Units = "s"))
    float FadeOutTime = 0.2f;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;

#if WITH_EDITOR
    /** 루프 사운드에 Keep Playing을 지정하면 경고한다. 런타임은 이 조합을 Stop으로 처리한다. */
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

/**
 * 사운드 재생의 실행별 상태.
 * 자신이 만든 오디오 컴포넌트만 추적하며, 컴포넌트는 재생이 끝나면 스스로 소멸하므로 약한 참조로 보관한다.
 */
UCLASS()
class KATARUNTIME_API UKataTaskInstance_PlaySound : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    /** Avatar의 스켈레탈 메시를 찾고, 없으면 루트 컴포넌트를 반환한다. */
    USceneComponent* ResolveAttachParent() const;

    TWeakObjectPtr<UAudioComponent> AudioComponent;
};
