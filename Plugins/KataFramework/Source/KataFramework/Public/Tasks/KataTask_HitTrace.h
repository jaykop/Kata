#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "HitTrace/KataHitTraceTypes.h"
#include "KataRuntimeTypes.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_HitTrace.generated.h"

class UKataHitBoxPreset;
class UKataHitHandler;
class UTargetingPreset;

/**
 * 타임라인의 한 구간 동안 공격 판정을 수행하는 태스크.
 *
 * 판정 영역은 HitBoxPreset이 정의하고, 소켓을 읽을 기준 메시는 Avatar의 UKataHitBoxComponent가 제공한다.
 * 컴포넌트가 없으면 Character 기준은 ACharacter의 Mesh로 대신하지만 직전 포즈가 없어 첫 프레임 구간을 쓸지 못한다.
 * 판정은 UKataHitSubsystem이 프레임 끝에 수행하고, 찾은 대상마다 HitHandlers를 제출 순서대로 호출한다.
 * 태스크 구간 동안 같은 대상은 한 번만 맞는다. 이 태스크는 데미지를 정하지 않는다.
 *
 * 정상 완료로 끝나면 종료 시각까지 잘라낸 마지막 판정을 하고, 취소·중단으로 끝나면 판정 없이 정리한다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Hit Trace"))
class KATAFRAMEWORK_API UKataTask_HitTrace : public UKataTask
{
    GENERATED_BODY()

public:
    /** 판정 영역 정의. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Trace")
    TObjectPtr<UKataHitBoxPreset> HitBoxPreset;

    /** 소켓을 읽을 기준 메시. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Trace")
    EKataHitBoxMeshSource MeshSource = EKataHitBoxMeshSource::Character;

    /** 켜면 구간이 시작되는 순간 판정 영역 안에 이미 있는 대상도 맞힌다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Trace")
    bool bCheckOnStart = true;

    /**
     * 대상 필터. Filter 태스크만 담은 Targeting Preset을 지정한다. 비우면 걸러내지 않는다.
     * 판정 후보를 결과 목록에 넣고 Preset의 태스크를 차례로 즉시 실행한 뒤 남은 대상만 맞힌다.
     * Selection·Sort 태스크가 섞여 있으면 판정과 무관한 대상이 추가되거나 순서만 바뀌므로 데이터 검증이 경고한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Trace")
    TObjectPtr<UTargetingPreset> FilterPreset;

    /** 찾은 대상마다 순서대로 호출할 처리기. */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Hit Trace")
    TArray<TObjectPtr<UKataHitHandler>> HitHandlers;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

/**
 * Hit Trace의 실행별 상태.
 * UKataHitSubsystem에 판정 구간을 등록한 핸들만 가지며, 이전 포즈와 맞은 대상 목록은 Subsystem의 등록 항목이 가진다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataTaskInstance_HitTrace : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    int32 HitBoxHandle = INDEX_NONE;
};
