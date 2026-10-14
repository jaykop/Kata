#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "HitTrace/KataHitTraceTypes.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_SpawnVFX.generated.h"

class UMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;

/** Spawn VFX 태스크가 정상 완료(Completed)로 끝났을 때 이펙트를 어떻게 할지 정한다. */
UENUM(BlueprintType)
enum class EKataVFXEndPolicy : uint8
{
    /** 이펙트가 스스로 끝날 때까지 둔다. 루프 시스템은 끝나지 않으므로 Stop Emitting으로 처리한다. */
    KeepPlaying,
    /** 새 파티클 방출을 멈추고 이미 나온 파티클은 수명대로 사라지게 한다. */
    StopEmitting,
    /** 남은 파티클까지 즉시 없앤다. */
    DestroyImmediately
};

/**
 * 태스크가 시작될 때 Niagara 이펙트를 생성한다.
 *
 * 맞는 순간의 이펙트는 히트 처리기의 Gameplay Cue가 맡는다. 이 태스크는 칼날 궤적처럼 타임라인 시각에 맞춘 이펙트를 다룬다.
 * 기준 메시는 Hit Trace와 같은 규칙으로 고른다. Avatar의 UKataHitBoxComponent가 Character·Weapon 메시를 제공하고,
 * 컴포넌트가 없으면 Character만 ACharacter의 메시로 대신하며, ACharacter가 아니면 첫 스켈레탈 메시를 찾는다.
 * 정상 완료로 끝나면 Completed Policy를 따른다. 실행 중인 태스크가 중단·취소·Kata 종료로 끝나면 방출을 멈춘다.
 * 이미 정상 완료한 태스크가 남긴 이펙트는 이후 Kata 종료로 다시 정리하지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Spawn VFX", KataTaskCategory = "FX"))
class KATAFRAMEWORK_API UKataTask_SpawnVFX : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_SpawnVFX();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    TObjectPtr<UNiagaraSystem> System;

    /** 켜면 기준 메시의 소켓에 붙여 움직임을 따라간다. 끄면 시작 순간의 소켓 위치에 두고 따라가지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    bool bAttachToMesh = true;

    /** 소켓을 읽을 기준 메시. Weapon은 장착한 장비 메시를 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    EKataHitBoxMeshSource MeshSource = EKataHitBoxMeshSource::Character;

    /**
     * Mesh Source가 Weapon일 때 쓸 장비 슬롯. 비우면 장착한 장비 메시가 하나일 때만 그것을 쓴다.
     * 해당 메시가 없으면 경고 후 이펙트를 생성하지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX",
        meta = (Categories = "Equipment.Slot", EditCondition = "MeshSource == EKataHitBoxMeshSource::Weapon", EditConditionHides))
    FGameplayTag WeaponSlot;

    /** 기준 메시의 소켓. 비우면 메시 원점을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    FName AttachSocket;

    /** 소켓 기준 위치 오프셋. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    FVector LocationOffset = FVector::ZeroVector;

    /** 소켓 기준 회전 오프셋. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    FRotator RotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    FVector Scale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    EKataVFXEndPolicy CompletedPolicy = EKataVFXEndPolicy::KeepPlaying;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;

#if WITH_EDITOR
    /** 루프 시스템에 Keep Playing을 지정하면 경고한다. 런타임은 이 조합을 Stop Emitting으로 처리한다. */
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

/**
 * 이펙트 생성의 실행별 상태.
 * 자신이 만든 Niagara 컴포넌트만 추적하며, 컴포넌트는 재생이 끝나면 스스로 소멸하므로 약한 참조로 보관한다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataTaskInstance_SpawnVFX : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    /** Mesh Source에 맞는 기준 메시를 찾는다. 찾지 못하면 경고를 남기고 nullptr을 반환한다. */
    UMeshComponent* ResolveMesh() const;

    TWeakObjectPtr<UNiagaraComponent> NiagaraComponent;
};
