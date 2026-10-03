#pragma once

#include "Character/KataCharacter.h"
#include "CoreMinimal.h"
#include "KataPlayerCharacter.generated.h"

class UKataInputHandlerComponent;
class UKataPlayerTargetingComponent;

/**
 * 플레이어가 조작하는 Kata 캐릭터.
 *
 * 공용 AKataCharacter의 타게팅 컴포넌트를 UKataPlayerTargetingComponent로 바꿔 만들어 소프트 타겟과 락온을 제공한다.
 * 플레이어 입력은 UKataInputHandlerComponent가 처리하며, 이 캐릭터는 입력 컴포넌트 준비를 그 컴포넌트에 넘긴다.
 * 카메라는 이 클래스가 다루지 않는다.
 */
UCLASS(Blueprintable, PrioritizeCategories = "Kata", meta = (DisplayName = "Kata Player Character"))
class KATAFRAMEWORK_API AKataPlayerCharacter : public AKataCharacter
{
    GENERATED_BODY()

public:
    AKataPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /** PC용 타게팅 컴포넌트. 파생 클래스가 타입을 PC용이 아닌 것으로 바꾸지 않았다면 수명 동안 항상 유효하다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataPlayerTargetingComponent* GetPlayerTargetingComponent() const;

    /** 플레이어 입력을 처리하는 컴포넌트. 생성자에서 만들기 때문에 수명 동안 항상 유효하다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataInputHandlerComponent* GetInputHandlerComponent() const { return InputHandlerComponent; }

    /** 공통 항목에 더해 FKataPlayerCharacterRow의 입력 설정과 콤보 그래프를 입력 처리 컴포넌트에 적용한다. 비어 있는 항목은 Blueprint 기본값을 유지한다. */
    virtual void ApplyCharacterRow(const FInstancedStruct& RowData) override;

protected:
    //~ Begin APawn Interface
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    //~ End APawn Interface

private:
    UPROPERTY(VisibleAnywhere, Category = "Kata")
    TObjectPtr<UKataInputHandlerComponent> InputHandlerComponent;
};
