#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataRuntimeTypes.h"
#include "KataGraphComponent.generated.h"

class UKataActionComponent;
class UKataGraph;
class UKataGraphInstance;

/** 액터의 UKataActionComponent에 그래프 실행과 트리거 입력을 연결하는 진입점. */
UCLASS(ClassGroup = (Kata), meta = (BlueprintSpawnableComponent, DisplayName = "Kata Graph Component"))
class KATAGRAPH_API UKataGraphComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKataGraphComponent();
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** 지정한 그래프를 시작한다. 기존 그래프가 실행 중이면 Interrupted로 끝낸다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph", meta = (DisplayName = "Start Kata Graph"))
    bool StartGraph(UKataGraph* Graph, const FKataContext& Context, UKataGraphInstance*& OutInstance);

    /** 소유 액터를 Context로 사용해 지정한 그래프를 시작한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph", meta = (DisplayName = "Start Kata Graph On Self"))
    bool StartGraphOnSelf(UKataGraph* Graph, AActor* TargetActor, UKataGraphInstance*& OutInstance);

    /** 실행 중인 그래프에 이벤트 태그를 전달한다. 버퍼 없이 즉시 한 번만 평가한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph")
    bool SendTrigger(FGameplayTag TriggerTag);

    UFUNCTION(BlueprintCallable, Category = "Kata|Graph")
    void StopGraph(EKataEndReason Reason);

    UFUNCTION(BlueprintPure, Category = "Kata|Graph")
    UKataGraphInstance* GetActiveGraphInstance() const { return ActiveGraphInstance; }

    UFUNCTION(BlueprintPure, Category = "Kata|Graph")
    bool IsRunningGraph() const;

private:
    UKataActionComponent* ResolveActionComponent() const;

    UPROPERTY(Transient)
    TObjectPtr<UKataGraphInstance> ActiveGraphInstance;
};
