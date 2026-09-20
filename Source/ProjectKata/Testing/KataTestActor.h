// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "KataRuntimeTypes.h"
#include "Templates/SubclassOf.h"
#include "KataTestActor.generated.h"

class UAbilitySystemComponent;
class UKataComponent;
class UKataDefinition;
class UKataInstance;

/**
 * Kata 실행을 확인하기 위한 테스트 액터.
 *
 * ASC와 UKataComponent를 함께 들고 ASC의 Actor Info를 초기화한다.
 * 프로젝트 전용 테스트 코드이며 플러그인에 포함하지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Test Actor"))
class PROJECTKATA_API AKataTestActor : public AActor, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AKataTestActor();

    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    /** 실행할 정의 클래스. Details에서 테스트 정의나 Blueprint 정의를 고른다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Test")
    TSubclassOf<UKataDefinition> DefinitionToPlay;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Test")
    bool bPlayOnBeginPlay = true;

    /** BeginPlay 이후 실행까지의 지연. 0이면 즉시 실행한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Test", meta = (ClampMin = "0.0", Units = "s"))
    float PlayDelaySeconds = 0.5f;

    /** BeginPlay에서 ASC에 붙여 둘 태그. 활성화 차단과 쿨다운 판정을 확인할 때 쓴다. */
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
    void DumpResolvedDefinition();

    UKataComponent* GetKataComponent() const { return KataComponent; }

private:
    UFUNCTION()
    void HandleKataStarted(UKataInstance* Instance);

    UFUNCTION()
    void HandleKataEnded(UKataInstance* Instance, EKataEndReason EndReason);

    UPROPERTY(VisibleAnywhere, Category = "Kata|Test")
    TObjectPtr<UAbilitySystemComponent> AbilitySystem;

    UPROPERTY(VisibleAnywhere, Category = "Kata|Test")
    TObjectPtr<UKataComponent> KataComponent;

    FTimerHandle PlayTimerHandle;
};
