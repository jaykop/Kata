#pragma once

#include "AIController.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionTypes.h"
#include "StateTreeTypes.h"
#include "KataAIController.generated.h"

class UStateTreeAIComponent;
class UKataAIData;
class UAIPerceptionComponent;
class UKataAITargetingComponent;
struct FStateTreeReference;

/** Pawn·ASC 준비 뒤 StateTree를 실행하고 빙의 해제 전에 정리한다. */
UCLASS(Blueprintable, meta = (DisplayName = "Kata AI Controller"))
class KATAAI_API AKataAIController : public AAIController
{
    GENERATED_BODY()

public:
    AKataAIController();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual FGenericTeamId GetGenericTeamId() const override;

    /** Pawn의 BeginPlay에서도 호출한다. 준비되지 않았거나 이미 시도했으면 아무것도 하지 않는다. */
    void TryStartKataAI();

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

private:
    /** 트리의 실행과 완료를 구분할 수 있도록 상태 전환을 기록한다. */
    UFUNCTION()
    void HandleStateTreeRunStatusChanged(EStateTreeRunStatus Status);

    UFUNCTION()
    void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    void HandleSelectedTargetChanged(bool bHadTarget, AActor* Target);
    void UnbindTargetEvents();

    /** 프로젝트 모듈의 생성 코드에 의존하지 않고 등록된 공용 이벤트 태그를 조회한다. */
    FGameplayTag TargetAcquiredTag;
    FGameplayTag TargetLostTag;
    FGameplayTag TargetChangedTag;

    TWeakObjectPtr<UKataAITargetingComponent> BoundTargeting;
    FDelegateHandle TargetChangedHandle;

    /** Pawn 컨텍스트가 남아 있는 동안 실행·이동·Focus를 정리한다. */
    void StopKataAI();

    /** 이번 빙의의 공유 설정을 복제해 독립적인 인지 Listener를 등록한다. */
    void ConfigureKataPerception(const UKataAIData& Data);

    /** 기존 Listener와 감지 기록을 제거한다. 다른 Pawn의 설정을 이어받지 않는다. */
    void ClearKataPerception();

    /**
     * AI Data의 슬롯 목록을 Linked Asset 오버라이드로 적용한다. 트리 시작 전에 호출하며, 슬롯이 없으면 이전 빙의의 오버라이드를 지운다.
     * 엔진은 무효 항목이 하나라도 있으면 목록 전체를 무시하므로 태그·트리·스키마가 맞지 않는 항목은 경고 후 제외한다.
     */
    void ApplyLinkedStateTreeSlots(const UKataAIData& Data, const FStateTreeReference& MainTree);

    UPROPERTY(VisibleAnywhere, Category = "Kata|AI")
    TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAIPerceptionComponent> RuntimePerceptionComponent;

    /** 한 번의 빙의에서 시작 실패를 반복 시도하지 않는다. 재빙의하면 초기화한다. */
    bool bStartAttempted = false;
    bool bGameplayReady = false;
};
