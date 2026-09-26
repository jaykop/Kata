// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "KataCharacter.generated.h"

class UAbilitySystemComponent;
class UKataActionComponent;
class UKataGraphComponent;
class UKataHitBoxComponent;
class UKataTargetingComponent;

/**
 * Kata 코어와 위성 플러그인의 컴포넌트를 갖춘 공용 캐릭터.
 *
 * ASC, UKataActionComponent, UKataGraphComponent, UKataTargetingComponent, UKataHitBoxComponent를 소유하며
 * PostInitializeComponents에서 ASC의 Actor Info를 초기화한다.
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
UCLASS(Blueprintable, meta = (DisplayName = "Kata Character"))
class KATAFRAMEWORK_API AKataCharacter : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    AKataCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /** 타게팅 컴포넌트의 서브오브젝트 이름. 파생 클래스가 SetDefaultSubobjectClass로 타입을 바꿀 때 쓴다. */
    static const FName TargetingComponentName;

    //~ Begin AActor Interface
    virtual void PostInitializeComponents() override;
    //~ End AActor Interface

    //~ Begin IAbilitySystemInterface
    /** 소유한 ASC를 돌려준다. 생성자에서 만들기 때문에 수명 동안 항상 유효하다. */
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    //~ End IAbilitySystemInterface

    //~ Begin IGenericTeamAgentInterface
    /**
     * 타게팅 컴포넌트의 팩션에 해당하는 팀 번호를 돌려준다. 팩션이 등록되지 않았으면 NoTeam이다.
     * SetGenericTeamId는 재정의하지 않는다. 팩션은 타게팅 컴포넌트의 Faction 값으로만 바꾼다.
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

private:
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
};
