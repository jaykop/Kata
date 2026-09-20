// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "KataRuntimeTypes.h"
#include "Testing/KataTestActions.h"
#include "KataTestActor.generated.h"

class UAbilitySystemComponent;
class UKataComponent;
class UKataAction;
class UKataActionInstance;

/**
 * Kata 실행을 확인하기 위한 테스트 액터.
 *
 * ASC와 UKataComponent를 소유하며 ASC의 Actor Info를 초기화한다.
 * 프로젝트 전용 테스트 코드이며 플러그인에 포함하지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Test Actor"))
class PROJECTKATATESTING_API AKataTestActor : public AActor, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AKataTestActor();

    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    /** 실행할 Kata Action 에셋. Built In Action이 None일 때만 사용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Test")
    TObjectPtr<UKataAction> ActionToPlay;

    /** 에셋 대신 코드로 만드는 검증용 액션. None이 아니면 Action To Play보다 우선한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Test")
    EKataTestAction BuiltInAction = EKataTestAction::Basic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Test")
    bool bPlayOnBeginPlay = true;

    /** BeginPlay 이후 실행까지의 지연. 0이면 즉시 실행한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Test", meta = (ClampMin = "0.0", Units = "s"))
    float PlayDelaySeconds = 0.5f;

    /** BeginPlay에서 ASC에 부여할 태그. 활성화 차단과 쿨다운 판정을 확인할 때 사용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Test")
    FGameplayTagContainer StartupLooseTags;

    /** 조건 평가와 태스크에 전달할 대상. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Test")
    TObjectPtr<AActor> TargetActor;

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Kata|Test")
    void PlayTestKata();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Kata|Test")
    void StopTestKata();

    /** 해석 결과와 진단을 로그로 출력한다. 실행하지 않는다. */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Kata|Test")
    void DumpResolvedAction();

    UKataComponent* GetKataComponent() const { return KataComponent; }

private:
    /** 이번 실행에 사용할 액션을 고른다. 코드 하네스는 매번 새로 만들어 최신 값을 반영한다. */
    UKataAction* ResolveActionToPlay();

    UFUNCTION()
    void HandleKataStarted(UKataActionInstance* Instance);

    UFUNCTION()
    void HandleKataEnded(UKataActionInstance* Instance, EKataEndReason EndReason);

    UPROPERTY(VisibleAnywhere, Category = "Kata|Test")
    TObjectPtr<UAbilitySystemComponent> AbilitySystem;

    UPROPERTY(VisibleAnywhere, Category = "Kata|Test")
    TObjectPtr<UKataComponent> KataComponent;

    /** 코드로 만든 액션 트리의 GC 참조. 최하위 액션 참조만 유지해도 ParentAction 체인 전체가 유지된다. */
    UPROPERTY(Transient)
    TObjectPtr<UKataAction> BuiltInActionObject;

    FTimerHandle PlayTimerHandle;
};
