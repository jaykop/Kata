#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "KataPlayerController.generated.h"

class UKataPlayerTargetingComponent;
class UKataTargetPointComponent;
class UKataMainHUD;

/**
 * Kata 플레이어 컨트롤러.
 *
 * 빙의한 폰과 무관한 플레이어 단위 기능을 맡는다. 어떤 폰을 조종하든 유지해야 하는 IMC가 여기에 해당한다.
 * 폰별 입력 바인딩과 기본 IMC는 폰의 UKataInputHandlerComponent가 담당하므로 이 클래스는 다루지 않는다.
 * 카메라는 KataCamera의 AKataPlayerCameraManager를 기본으로 쓴다. Blueprint 파생에서 Player Camera Manager Class를 바꿀 수 있다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Kata Player Controller"))
class KATAFRAMEWORK_API AKataPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AKataPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void SetPawn(APawn* InPawn) override;

    UFUNCTION(BlueprintPure, Category = "Kata|HUD")
    UKataMainHUD* GetMainHUD() const { return MainHUD; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** 로컬 플레이어의 HUD 클래스. 비우면 HUD를 만들지 않는다. */
    UPROPERTY(EditDefaultsOnly, Category = "Kata|HUD")
    TSubclassOf<UKataMainHUD> MainHUDClass;

private:
    void RefreshTargetingBindings();
    void ClearTargetingBindings();

    UFUNCTION()
    void HandleLockTargetChanged(UKataTargetPointComponent* OldPoint, UKataTargetPointComponent* NewPoint);

    TWeakObjectPtr<UKataPlayerTargetingComponent> PlayerTargeting;

    UPROPERTY(Transient)
    TObjectPtr<UKataMainHUD> MainHUD;
};
