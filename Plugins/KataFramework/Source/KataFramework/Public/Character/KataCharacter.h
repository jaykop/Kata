// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "Character/KataGameplayData.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "StructUtils/InstancedStruct.h"
#include "KataCharacter.generated.h"

class UAbilitySystemComponent;
class UKataActionComponent;
class UKataGraphComponent;
class UKataEquipmentComponent;
class UKataHitBoxComponent;
class UKataRootMotionCurveComponent;
class UKataTargetingComponent;
struct FKataCharacterSpawnOwnership;

/**
 * Kata 코어와 위성 플러그인의 컴포넌트를 갖춘 공용 캐릭터.
 *
 * ASC, UKataActionComponent, UKataGraphComponent, UKataTargetingComponent, UKataHitBoxComponent, UKataEquipmentComponent를 소유하며
 * PostInitializeComponents에서 ASC의 Actor Info를 초기화하고, 행이 지정한 UKataGameplayData를 적용한다.
 * AttributeSet은 클래스에 고정하지 않고 행의 Gameplay Data가 추가한다.
 * ACharacter가 제공하는 Mesh에 스켈레탈 메시와 Anim Instance를 지정하면
 * UKataTask_PlayMontage가 별도 준비 없이 동작한다.
 * UKataAction의 Preview Actor Class에 지정할 기본 캐릭터로 사용한다.
 *
 * 타게팅 컴포넌트는 기반 타입으로 만든다. 역할별 캐릭터는 생성자에서
 * ObjectInitializer.SetDefaultSubobjectClass(TargetingComponentName)로 실제 타입을 바꾼다.
 * 팩션 값은 타게팅 컴포넌트가 가지며 이 캐릭터는 IGenericTeamAgentInterface로 그 팀 번호를 전달만 한다.
 *
 * 싱글플레이 전용이므로 복제를 설정하지 않는다.
 */
UCLASS(Blueprintable, PrioritizeCategories = "Kata", meta = (DisplayName = "Kata Character"))
class KATAFRAMEWORK_API AKataCharacter : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    AKataCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /** 타게팅 컴포넌트의 서브오브젝트 이름. 파생 클래스가 SetDefaultSubobjectClass로 타입을 바꿀 때 쓴다. */
    static const FName TargetingComponentName;

    //~ Begin AActor Interface
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void PostInitializeComponents() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SpawnDefaultController() override;
    virtual void DetachFromControllerPendingDestroy() override;
    //~ End AActor Interface

    /** Blueprint Construction Script가 끝난 뒤 행을 적용하고 컴포넌트 초기화 전에 생성을 마친다. */
    void FinishSpawningWithCharacterRow(const FTransform& SpawnTransform, const FInstancedStruct& RowData);

    /** 생성 완료 전에 소유 기록을 연결한다. Controller 생성과 종료 시 같은 기록을 사용한다. */
    void SetSpawnOwnership(const TSharedPtr<FKataCharacterSpawnOwnership>& Ownership);

    /**
     * 캐릭터 데이터 테이블 행을 이 캐릭터에 적용한다.
     *
     * UKataCharacterSpawnSubsystem이 OnConstruction에서 Blueprint Construction Script 이후에 호출하므로
     * 컴포넌트 초기화와 빙의보다 먼저 실행된다. 행이 참조하는 에셋은 호출 전에 로드되어 있어야 한다.
     * 이 클래스는 FKataCharacterRow의 메시와 Anim Blueprint를 적용하고, 비어 있는 항목은 Blueprint 기본값을 유지한다.
     * 파생 클래스는 Super를 호출한 뒤 자기 행 타입의 항목을 적용한다.
     *
     * @param RowData FKataCharacterRow 계열 행의 복사본. 다른 타입이면 아무것도 하지 않는다.
     */
    virtual void ApplyCharacterRow(const FInstancedStruct& RowData);

    //~ Begin IAbilitySystemInterface
    /** 소유한 ASC를 돌려준다. 생성자에서 만들기 때문에 수명 동안 항상 유효하다. */
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    //~ End IAbilitySystemInterface

    //~ Begin IGenericTeamAgentInterface
    /**
     * 타게팅 컴포넌트의 팩션에 해당하는 팀 번호를 돌려준다. 팩션이 등록되지 않았으면 NoTeam이다.
     * SetGenericTeamId는 재정의하지 않는다. 팩션은 타게팅 컴포넌트의 Faction 값으로만 바꾼다.
     * 행으로 생성하면 행의 Faction(스포너가 덮어쓴 값 포함)이 BeginPlay 전에 그 값을 채운다.
     */
    virtual FGenericTeamId GetGenericTeamId() const override;
    //~ End IGenericTeamAgentInterface

    /** Kata 액션 실행을 담당하는 컴포넌트. 생성자에서 만들기 때문에 수명 동안 항상 유효하다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataActionComponent* GetActionComponent() const { return ActionComponent; }

    /** 그래프 실행과 트리거 입력을 받는 컴포넌트. 생성자에서 만들기 때문에 수명 동안 항상 유효하다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataGraphComponent* GetGraphComponent() const { return GraphComponent; }

    /** 팩션과 대상 결정을 담당하는 컴포넌트. 실제 타입은 역할별 캐릭터가 정하며 수명 동안 항상 유효하다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataTargetingComponent* GetTargetingComponent() const { return TargetingComponent; }

    /** 공격 판정의 기준 메시와 직전 포즈를 제공하는 컴포넌트. 생성자에서 만들기 때문에 수명 동안 항상 유효하다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataHitBoxComponent* GetHitBoxComponent() const { return HitBoxComponent; }

    /** 장비를 부위 슬롯에 장착·해제하는 컴포넌트. 생성자에서 만들기 때문에 수명 동안 항상 유효하다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

    /** 몽타주 루트 모션을 Kata 루트 모션 커브 값으로 바꾸는 컴포넌트. 생성자에서 만들기 때문에 수명 동안 항상 유효하다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataRootMotionCurveComponent* GetRootMotionCurveComponent() const { return RootMotionCurveComponent; }

private:
    UFUNCTION()
    void HandleSpawnOwnedControllerPawnChanged(APawn* OldPawn, APawn* NewPawn);

    TSharedPtr<FKataCharacterSpawnOwnership> SpawnOwnership;
    /** FinishSpawning 동안만 유지할 행 사본. Construction Script가 기본값을 복원한 뒤 적용한다. */
    UPROPERTY(Transient)
    FInstancedStruct PendingCharacterRow;

    /** 행에서 받은 Gameplay Data. 행 적용(OnConstruction) 뒤 ASC가 준비되는 PostInitializeComponents까지 보관한다. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UKataGameplayData>> PendingGameplayData;

    /** 행에서 받은 Identity 태그. PendingGameplayData와 함께 적용한다. */
    UPROPERTY(Transient)
    FGameplayTagContainer PendingIdentityTags;

    /** Gameplay Data를 적용한 결과. 추가한 AttributeSet, 부여한 Ability·Effect 핸들과 태그를 기록한다. */
    UPROPERTY(Transient)
    FKataGameplayDataHandles GameplayDataHandles;

    UPROPERTY(VisibleAnywhere, Category = "Kata")
    TObjectPtr<UAbilitySystemComponent> AbilitySystem;

    UPROPERTY(VisibleAnywhere, Category = "Kata")
    TObjectPtr<UKataActionComponent> ActionComponent;

    UPROPERTY(VisibleAnywhere, Category = "Kata")
    TObjectPtr<UKataGraphComponent> GraphComponent;

    UPROPERTY(VisibleAnywhere, Category = "Kata")
    TObjectPtr<UKataTargetingComponent> TargetingComponent;

    UPROPERTY(VisibleAnywhere, Category = "Kata")
    TObjectPtr<UKataHitBoxComponent> HitBoxComponent;

    UPROPERTY(VisibleAnywhere, Category = "Kata")
    TObjectPtr<UKataEquipmentComponent> EquipmentComponent;

    UPROPERTY(VisibleAnywhere, Category = "Kata")
    TObjectPtr<UKataRootMotionCurveComponent> RootMotionCurveComponent;
};
