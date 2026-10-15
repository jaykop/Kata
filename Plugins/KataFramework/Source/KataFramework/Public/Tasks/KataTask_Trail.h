#pragma once

#include "Action/KataTask.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "HitTrace/KataHitTraceTypes.h"
#include "Runtime/KataTaskInstance.h"
#include "KataTask_Trail.generated.h"

class UMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 태스크 구간 동안 칼날·팔다리처럼 소켓을 따라 놓인 부위가 지나간 자리를 Niagara 리본으로 그린다.
 *
 * 소켓 경로는 Hit Trace와 같은 방법으로 계산한다. UKataHitSubsystem이 포즈 확정 뒤 프레임 사이 포즈를 재샘플링하고
 * 구간 경계 [시작, 종료]에 맞춰 잘라 샘플을 만든다. 그래서 저프레임이나 한 프레임보다 짧은 구간에서도 판정과 같은 호를 따른다.
 * 샘플은 이웃한 소켓 구간마다 입자 하나로 바뀌어 Niagara User 파라미터 배열로 전달된다. 계약은 사용법 문서의 표를 따른다.
 *
 * 기준 메시는 Spawn VFX와 같은 규칙으로 고른다. Character는 몸의 소켓(예: 팔꿈치·손), Weapon은 장비 메시의 소켓을 쓴다.
 * 정상 완료로 끝나면 종료 시각까지 그린 뒤 방출을 멈추고, 중단·취소·Kata 종료로 끝나면 더 그리지 않고 방출을 멈춘다.
 * 남은 입자는 수명대로 사라지며, 그동안에도 Avatar의 시간 배율을 따른다(히트스톱 중에는 함께 멈춘다).
 */
UCLASS(meta = (DisplayName = "Kata Task: Trail", KataTaskCategory = "FX"))
class KATAFRAMEWORK_API UKataTask_Trail : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_Trail();

    /** 궤적을 그릴 Niagara 시스템. 사용법 문서의 KataTrail User 파라미터를 읽어 리본을 만들어야 한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trail")
    TObjectPtr<UNiagaraSystem> System;

    /** 소켓을 읽을 기준 메시. Weapon은 장착한 장비 메시, Character는 캐릭터 본체 메시를 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trail")
    EKataHitBoxMeshSource MeshSource = EKataHitBoxMeshSource::Weapon;

    /**
     * Mesh Source가 Weapon일 때 쓸 장비 슬롯. 비우면 장착한 장비 메시가 하나일 때만 그것을 쓴다.
     * 해당 메시가 없으면 경고 후 궤적을 그리지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trail",
        meta = (Categories = "Equipment.Slot", EditCondition = "MeshSource == EKataHitBoxMeshSource::Weapon", EditConditionHides))
    FGameplayTag WeaponSlot;

    /**
     * 궤적의 폭을 이루는 소켓. 2개 이상을 한쪽 끝부터 순서대로 넣는다.
     * 직선 칼날은 손잡이 쪽과 끝 두 개, 휜 칼날은 휘는 지점마다 소켓을 더한다. 이웃한 두 소켓 사이마다 리본 하나가 생긴다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trail")
    TArray<FName> Sockets;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
    virtual FName GetConfigurationError() const override;
    virtual FString DescribeConfigurationError(FName ErrorCode) const override;

#if WITH_EDITOR
    /** System이 KataTrail 출력 User 파라미터를 갖지 않으면 경고한다. 없는 파라미터는 런타임에 건너뛴다. */
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

/**
 * 궤적의 실행별 상태.
 * Subsystem 등록 핸들만 가진다. 시작할 때 만든 Niagara 컴포넌트는 등록과 함께 넘기며, 샘플링과 방출 정지는 UKataHitSubsystem이 맡는다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataTaskInstance_Trail : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    /** Mesh Source에 맞는 기준 메시를 찾는다. 찾지 못하면 경고를 남기고 nullptr을 반환한다. */
    UMeshComponent* ResolveMesh() const;

    int32 TrailHandle = INDEX_NONE;
};
