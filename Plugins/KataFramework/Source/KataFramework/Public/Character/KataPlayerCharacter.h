#pragma once

#include "Character/KataCharacter.h"
#include "CoreMinimal.h"
#include "KataPlayerCharacter.generated.h"

class APlayerController;
class UKataInputConfig;
class UKataPlayerTargetingComponent;
struct FInputActionValue;

/**
 * 플레이어가 조작하는 Kata 캐릭터.
 *
 * 공용 AKataCharacter의 타게팅 컴포넌트를 UKataPlayerTargetingComponent로 바꿔 만들어 소프트 타겟과 락온을 제공한다.
 * Input Config에 따라 Enhanced Input을 바인딩하고, 플레이어 컨트롤러에 빙의되는 동안 기본 IMC를 유지한다.
 * 카메라는 이 클래스가 다루지 않는다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Kata Player Character"))
class KATAFRAMEWORK_API AKataPlayerCharacter : public AKataCharacter
{
    GENERATED_BODY()

public:
    AKataPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /** PC용 타게팅 컴포넌트. 파생 클래스가 타입을 PC용이 아닌 것으로 바꾸지 않았다면 수명 동안 항상 유효하다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataPlayerTargetingComponent* GetPlayerTargetingComponent() const;

    /** 이 캐릭터의 입력 구성. 지정하지 않았으면 null이며 이때 입력을 바인딩하지 않는다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Input")
    UKataInputConfig* GetInputConfig() const { return InputConfig; }

protected:
    //~ Begin APawn Interface
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void NotifyControllerChanged() override;
    //~ End APawn Interface

    /** 이동 입력을 컨트롤 회전의 Yaw 기준 앞·오른쪽 방향 이동으로 바꾼다. */
    virtual void Move(const FInputActionValue& Value);

    /** 시점 입력을 컨트롤 회전의 Yaw·Pitch에 더한다. */
    virtual void Look(const FInputActionValue& Value);

private:
    void AddDefaultMappingContexts(APlayerController* PlayerController) const;
    void RemoveDefaultMappingContexts(APlayerController* PlayerController) const;

    /**
     * 기본 IMC와 InputAction을 담은 입력 구성.
     * 빙의 중에 바꾸면 추가한 IMC와 제거할 IMC가 달라지므로 런타임에 바꾸지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Input", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UKataInputConfig> InputConfig;
};
