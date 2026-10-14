#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KataHUDSubsystem.generated.h"

class APawn;
class UAbilitySystemComponent;
class UKataCombatEventSubsystem;
class UKataDeathComponent;
class UKataVM_EnemyHealth;
class UKataVM_EnemyHealthList;
class UKataVM_PlayerStatus;
struct FOnAttributeChangeData;

/** 표시 중인 적 체력바 하나의 구독과 만료 시각. 대상·ASC·사망 컴포넌트는 약한 참조다. */
USTRUCT()
struct FKataEnemyBarTracking
{
    GENERATED_BODY()

    UPROPERTY(Transient)
    TObjectPtr<UKataVM_EnemyHealth> ViewModel;

    TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
    TWeakObjectPtr<UKataDeathComponent> DeathComponent;
    FDelegateHandle HealthHandle;
    FDelegateHandle MaxHealthHandle;
    FDelegateHandle DeathHandle;

    /** 락온하지 않았을 때 이 실시간(초)이 지나면 지운다. */
    double HideAtTime = 0.0;

    /** 사망 뒤 이 실시간(초)이 지나면 락온 여부와 관계없이 지운다. 0 이하면 해당 없음이다. */
    double DeadHideAtTime = 0.0;
};

/**
 * 로컬 플레이어의 HUD ViewModel을 소유하고 모델을 구독해 값을 채운다.
 * ViewModel은 클래스마다 하나씩 만들어 플레이어 수명 동안 유지하므로, HUD 위젯을 다시 만들어도 같은 인스턴스에 바인딩된다.
 * 관찰할 폰과 락온 대상은 AKataPlayerController가 바뀔 때마다 전달한다.
 *
 * 적 체력바: 관찰 폰 월드의 UKataCombatEventSubsystem 피해 알림을 구독해 Kata HUD 설정의 출처·거리 조건과
 * 관찰 폰에 대한 적대 관계를 만족하는 대상을 목록 ViewModel에 넣는다. 만료 시각은 실시간으로 재므로 히트스톱·시간 배율의 영향을 받지 않는다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataHUDSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * ViewModelClass의 인스턴스를 돌려주고, 없으면 만들어 보관한다.
     * 클래스가 nullptr이면 nullptr을 반환한다. 새로 만들 클래스가 INotifyFieldValueChanged를 구현하지 않거나
     * 추상·폐기·이전 버전 클래스면 nullptr을 반환하고 경고 로그를 남긴다.
     * 반환한 인스턴스는 이 서브시스템이 소유하며, 서브시스템 종료 시 보관 참조를 해제한다.
     */
    UObject* FindOrAddViewModel(const UClass* ViewModelClass);

    /**
     * 플레이어 상태 ViewModel과 적 체력바가 따를 폰을 정한다. 이전 폰의 Attribute 구독을 해제하고 새 폰의 값을 즉시 반영한다.
     * 폰이 nullptr이거나 ASC 또는 UKataAttributeSet_Base가 없으면 값을 0으로 두고 HasStatus를 false로 둔다.
     * 폰의 월드가 바뀌면 표시 중인 적 체력바를 모두 지우고 새 월드의 피해 알림을 구독한다.
     */
    void SetObservedPawn(APawn* Pawn);

    /**
     * 로컬 플레이어의 락온 대상을 알린다. nullptr이면 락온 해제다.
     * 락온 대상은 피해를 입지 않았어도 체력바를 표시하고 락온 중에는 만료시키지 않는다. 해제하면 그때부터 표시 시간을 잰다.
     */
    void SetLockTarget(AActor* Target);

    UKataVM_PlayerStatus* GetPlayerStatus() const { return PlayerStatus; }
    UKataVM_EnemyHealthList* GetEnemyHealthList() const { return EnemyHealthList; }

    // FTickableGameObject
    virtual void Tick(float DeltaTime) override;
    virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;

private:
    void ClearPlayerStatusBindings();
    void HandleAttributeChanged(const FOnAttributeChangeData& Data);

    /** Pawn이 속한 월드의 피해 알림을 구독한다. 이미 같은 월드를 구독 중이면 그대로 둔다. */
    void RefreshCombatEventBinding(APawn* Pawn);
    void ClearCombatEventBinding();
    void HandleDamageApplied(AActor* Target, AActor* DamageInstigator, float Damage);

    /** 대상의 항목을 찾거나 새로 만든다. 조건을 만족하지 못하거나 자리가 없으면 nullptr이다. */
    FKataEnemyBarTracking* FindOrAddEnemyBar(AActor* Target, bool bIgnoreDistance);
    FKataEnemyBarTracking* FindEnemyBar(const AActor* Target);
    void RemoveEnemyBarAt(int32 Index);
    void ClearEnemyBars();
    double GetNow() const;

    /** 클래스별로 하나씩 보관한다. 개수가 적으므로 선형 탐색으로 찾는다. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> ViewModels;

    /** ViewModels에도 들어 있으며, 구독 처리에서 바로 쓰기 위해 따로 가리킨다. */
    UPROPERTY(Transient)
    TObjectPtr<UKataVM_PlayerStatus> PlayerStatus;

    /** ViewModels에도 들어 있다. 항목 ViewModel은 이 목록과 EnemyBars가 함께 가리킨다. */
    UPROPERTY(Transient)
    TObjectPtr<UKataVM_EnemyHealthList> EnemyHealthList;

    UPROPERTY(Transient)
    TArray<FKataEnemyBarTracking> EnemyBars;

    TWeakObjectPtr<APawn> ObservedPawn;
    TWeakObjectPtr<AActor> LockTarget;
    TWeakObjectPtr<UAbilitySystemComponent> ObservedAbilitySystem;

    /** 구독한 Attribute 순서대로 보관한 핸들. 순서는 구현 파일의 Attribute 목록과 같다. */
    TArray<FDelegateHandle> AttributeChangeHandles;

    TWeakObjectPtr<UKataCombatEventSubsystem> CombatEvents;
    FDelegateHandle DamageAppliedHandle;
};
