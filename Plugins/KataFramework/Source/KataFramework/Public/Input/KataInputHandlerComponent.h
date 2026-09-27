#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "KataInputHandlerComponent.generated.h"

class AController;
class APawn;
class APlayerController;
class UInputComponent;
class UKataInputConfig;
struct FInputActionValue;

/**
 * 폰의 플레이어 입력을 처리하는 컴포넌트.
 *
 * Input Config에 따라 Enhanced Input을 바인딩하고, 로컬 플레이어 컨트롤러에 빙의되는 동안 기본 IMC를 유지한다.
 * 빙의 대상이 다른 폰으로 바뀌면 이전 폰의 컴포넌트가 IMC를 제거하고 새 폰의 컴포넌트가 추가한다.
 * 폰이 아닌 액터에 붙이면 아무 일도 하지 않는다.
 *
 * 엔진이 빙의마다 만드는 UEnhancedInputComponent는 바인딩을 보관할 뿐이며, 이 컴포넌트는 그 위에서
 * 무엇을 바인딩하고 입력을 어떻게 처리할지를 맡는다. 폰은 SetupPlayerInputComponent에서 SetupPlayerInput을 호출해야 한다.
 */
UCLASS(ClassGroup = (Kata), meta = (BlueprintSpawnableComponent, DisplayName = "Kata Input Handler Component"))
class KATAFRAMEWORK_API UKataInputHandlerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKataInputHandlerComponent();

    /**
     * 폰의 PlayerInputComponent에 Input Config의 InputAction을 바인딩한다.
     *
     * 폰의 SetupPlayerInputComponent에서 호출한다. 바인딩은 전달받은 컴포넌트에 속하며,
     * 빙의가 풀리면 엔진이 그 컴포넌트를 파괴하므로 따로 해제하지 않는다.
     * UEnhancedInputComponent가 아니거나 Input Config가 없으면 경고를 남기고 바인딩하지 않는다.
     */
    void SetupPlayerInput(UInputComponent* PlayerInputComponent);

    /** 입력 구성. 지정하지 않았으면 null이다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Input")
    UKataInputConfig* GetInputConfig() const { return InputConfig; }

protected:
    //~ Begin UActorComponent Interface
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    //~ End UActorComponent Interface

    /** 이동 입력을 컨트롤 회전의 Yaw 기준 앞·오른쪽 방향 이동으로 바꾼다. */
    virtual void Move(const FInputActionValue& Value);

    /** 시점 입력을 컨트롤 회전의 Yaw·Pitch에 더한다. */
    virtual void Look(const FInputActionValue& Value);

private:
    UFUNCTION()
    void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

    APawn* GetPawn() const;
    void AddDefaultMappingContexts(const APlayerController* PlayerController) const;
    void RemoveDefaultMappingContexts(const APlayerController* PlayerController) const;

    /**
     * 기본 IMC와 InputAction을 담은 입력 구성.
     * 빙의 중에 바꾸면 추가한 IMC와 제거할 IMC가 달라지므로 런타임에 바꾸지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Input", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UKataInputConfig> InputConfig;
};
