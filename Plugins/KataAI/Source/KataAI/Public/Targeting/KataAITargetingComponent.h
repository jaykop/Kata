#pragma once

#include "CoreMinimal.h"
#include "Targeting/KataTargetingComponent.h"
#include "TimerManager.h"
#include "KataAITargetingComponent.generated.h"

class UKataAIData;
class UAIPerceptionComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FKataAITargetChanged, bool, AActor*);

/** 보이는 액터의 약한 참조와 마지막 성공 감지 위치를 보관한다. */
USTRUCT()
struct FKataAIVisibleActor
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> Actor;

    FVector Location = FVector::ZeroVector;
    float ObservedTime = 0.0f;
};

/** 시각 인지 후보와 Preset으로 액션 대상을 고르는 AI 타게팅 컴포넌트다. */
UCLASS(Blueprintable, ClassGroup = (Kata), meta = (BlueprintSpawnableComponent))
class KATAAI_API UKataAITargetingComponent : public UKataTargetingComponent
{
    GENERATED_BODY()

public:
    /** 첫 인자는 직전 대상 존재 여부다. 파괴된 대상도 상실로 통지한다. */
    FKataAITargetChanged OnTargetChanged;

    /** 복귀 완료 시 기억만 지운다. 현재 보이는 대상은 유지한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|AI")
    void ClearTargetMemory();

    /** 정상 빙의 때 호출한다. 인지·대상을 비우고 새 Pawn의 귀환 위치를 기록한다. */
    void InitializeKataAI(UKataAIData* InData, UAIPerceptionComponent* InPerception);

    /** 타이머·인지·대상 참조를 모두 정리한다. 여러 번 호출해도 안전하다. */
    void ResetKataAI();

    /** 시각 감각의 성공·실패 이벤트만 받는다. 실패 위치는 사용하지 않는다. */
    void UpdateSight(AActor* Actor, bool bVisible, const FVector& ObservedLocation);

    /** Preset Selection 태스크에 현재 보이는 유효 후보를 감지 순서로 전달한다. */
    void GetVisibleActors(TArray<AActor*>& OutActors) const;

    virtual AActor* GetCurrentTarget_Implementation() const override;
    virtual AActor* ResolveActionTarget_Implementation() override;
    virtual bool CanKeepActionTarget_Implementation(AActor* ActionTarget) const override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintPure, Category = "Kata|AI")
    bool HasLastKnownLocation() const;

    UFUNCTION(BlueprintPure, Category = "Kata|AI")
    FVector GetLastKnownLocation() const { return LastKnownLocation; }

    UFUNCTION(BlueprintPure, Category = "Kata|AI")
    FVector GetHomeLocation() const { return HomeLocation; }

    /** 감지한 적이 없으면 0을 반환한다. 시야 상실 후에는 마지막 관측부터 지난 시간이 증가한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|AI")
    float GetTimeSinceLastSeen() const;

private:
    /** Perception의 최신 시각 후보를 반영하고 Preset을 주기적으로 실행한다. */
    void RefreshPerception();
    void SelectCurrentTarget();
    void RememberTarget(AActor* Target);
    bool IsVisibleHostile(AActor* Actor) const;

    UPROPERTY(Transient)
    TObjectPtr<UKataAIData> AIData;

    UPROPERTY(Transient)
    TWeakObjectPtr<UAIPerceptionComponent> Perception;

    UPROPERTY(Transient)
    TArray<FKataAIVisibleActor> VisibleActors;

    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> CurrentTarget;

    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> LastSeenTarget;

    FVector LastKnownLocation = FVector::ZeroVector;
    FVector HomeLocation = FVector::ZeroVector;
    float LastSeenTime = 0.0f;
    FTimerHandle RefreshTimer;
    bool bHadSelectedTarget = false;
};
