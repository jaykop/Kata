// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KataCharacter.generated.h"

class UAbilitySystemComponent;
class UKataComponent;

/**
 * Kata 액션을 실행하는 데 필요한 최소 구성을 갖춘 캐릭터.
 *
 * ASC와 UKataComponent를 소유하며 PostInitializeComponents에서 ASC의 Actor Info를 초기화한다.
 * ACharacter가 제공하는 Mesh에 스켈레탈 메시와 Anim Instance를 지정하면
 * UKataTask_PlayMontage가 별도 준비 없이 동작한다.
 * UKataAction의 Preview Actor Class에 지정할 기본 캐릭터로 사용한다.
 *
 * 싱글플레이 전용이므로 복제를 설정하지 않는다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Kata Character"))
class KATARUNTIME_API AKataCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AKataCharacter();

    //~ Begin AActor Interface
    virtual void PostInitializeComponents() override;
    //~ End AActor Interface

    //~ Begin IAbilitySystemInterface
    /** 소유한 ASC를 돌려준다. 생성자에서 만들기 때문에 수명 동안 항상 유효하다. */
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    //~ End IAbilitySystemInterface

    /** Kata 액션 실행을 담당하는 컴포넌트. 생성자에서 만들기 때문에 수명 동안 항상 유효하다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataComponent* GetKataComponent() const { return KataComponent; }

private:
    UPROPERTY(VisibleAnywhere, Category = "Kata")
    TObjectPtr<UAbilitySystemComponent> AbilitySystem;

    UPROPERTY(VisibleAnywhere, Category = "Kata")
    TObjectPtr<UKataComponent> KataComponent;
};
