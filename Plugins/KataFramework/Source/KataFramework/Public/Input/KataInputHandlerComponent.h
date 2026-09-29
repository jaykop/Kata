#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataInputHandlerComponent.generated.h"

class AController;
class APawn;
class APlayerController;
class UInputComponent;
class UKataGraph;
class UKataInputConfig;
struct FInputActionValue;

/**
 * 폰의 플레이어 입력을 처리하는 컴포넌트.
 *
 * Input Config에 따라 Enhanced Input을 바인딩하고, 로컬 플레이어 컨트롤러에 빙의되는 동안 기본 IMC를 유지한다.
 * 빙의 대상이 다른 폰으로 바뀌면 이전 폰의 컴포넌트가 IMC를 제거하고 새 폰의 컴포넌트가 추가한다.
 * 폰이 아닌 액터에 붙이면 아무 일도 하지 않는다.
 *
 * Input Config의 Input Bindings로 입력을 Input 태그로 바꾸고, Trigger Mappings로 찾은 Trigger 태그를
 * 폰의 UKataGraphComponent에 보낸다. 그래프는 콤보가 끝나면 종료되므로 실행 중이 아니면 Graph를 새로 시작한 뒤 보낸다.
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

    /**
     * 입력 구성을 바꾼다.
     *
     * 빙의 전에만 바꿀 수 있다. 캐릭터 데이터 테이블 행을 적용할 때처럼 생성 직후 빙의 전에 호출한다.
     * 폰에 컨트롤러가 있으면 이미 추가한 IMC와 바인딩이 새 구성과 어긋나므로 경고를 남기고 무시한다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Input")
    void SetInputConfig(UKataInputConfig* NewInputConfig);

    /** 트리거를 보낼 그래프. 지정하지 않았으면 null이며 이때 입력을 그래프로 보내지 않는다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Input")
    UKataGraph* GetGraph() const { return Graph; }

    /**
     * 트리거를 보낼 그래프를 바꾼다.
     *
     * 입력 구성과 같이 빙의 전에만 바꿀 수 있으며, 폰에 컨트롤러가 있으면 경고를 남기고 무시한다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Input")
    void SetGraph(UKataGraph* NewGraph);

    /**
     * Trigger 태그를 폰의 그래프에 보낸다.
     *
     * 그래프가 실행 중이 아니면 Graph를 대상 없이 새로 시작한 뒤 보낸다. 대상은 액션의 Resolve Target Command가 정한다.
     * 새로 시작한 그래프가 이 트리거로 진입하지 못하면 Cancelled로 멈춰, 입력을 기다리는 그래프가 남지 않게 한다.
     * Graph나 폰의 UKataGraphComponent가 없으면 아무것도 하지 않는다.
     *
     * @return 그래프가 트리거를 받아 전이했거나 전이를 예약했으면 true.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Input")
    bool SendGraphTrigger(UPARAM(meta = (Categories = "Trigger")) FGameplayTag TriggerTag);

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
    /** Input Bindings에 등록한 입력이 발생했을 때 호출된다. Trigger Mappings에 있으면 그래프로 보낸다. */
    void HandleInputTag(FGameplayTag InputTag);

    UFUNCTION()
    void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

    APawn* GetPawn() const;
    bool IsPossessed() const;
    void AddDefaultMappingContexts(const APlayerController* PlayerController) const;
    void RemoveDefaultMappingContexts(const APlayerController* PlayerController) const;

    /**
     * 기본 IMC와 InputAction을 담은 입력 구성.
     * 빙의 중에 바꾸면 추가한 IMC와 제거할 IMC가 달라지므로 런타임에 바꾸지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Input", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UKataInputConfig> InputConfig;

    /** 입력 트리거로 구동할 콤보 그래프. 이후 캐릭터 정의가 Input Config와 함께 채운다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Input", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UKataGraph> Graph;
};
