#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "InputTriggers.h"
#include "KataInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/** 캐릭터가 빙의될 때 추가할 Input Mapping Context와 그 우선순위. */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataInputMappingContextEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> MappingContext;

    /** 값이 클수록 먼저 입력을 받는다. 같은 키를 쓰는 낮은 우선순위의 매핑은 InputAction의 Consume Input 설정에 따라 가려진다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    int32 Priority = 0;
};

/**
 * InputAction의 특정 이벤트를 Input 태그로 바꾸는 규칙.
 *
 * Input 태그는 플레이어가 무엇을 눌렀는지를 나타낸다. 그래프에 무엇을 보낼지는 Trigger Mappings가 정한다.
 */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataInputTagBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> InputAction;

    /** 이 이벤트가 발생할 때 Input 태그를 낸다. 누름·홀드·뗌 같은 입력 형태는 InputAction과 IMC의 Trigger로 정한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    ETriggerEvent TriggerEvent = ETriggerEvent::Started;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (Categories = "Input"))
    FGameplayTag InputTag;
};

/** Input 태그를 그래프에 보낼 Trigger 태그로 바꾸는 규칙. */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataInputTriggerMapping
{
    GENERATED_BODY()

    /** 정확히 일치하는 Input 태그만 대응한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (Categories = "Input"))
    FGameplayTag InputTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (Categories = "Trigger"))
    FGameplayTag TriggerTag;
};

/**
 * InputAction을 현재 Kata 액션의 캔슬 요청으로 바꾸는 규칙.
 *
 * 입력이 시작될 때는 새로 누른 요청, 입력이 이어지는 동안에는 누르고 있는 요청을 보낸다.
 * 액션의 Cancel Window 태스크가 같은 태그의 창을 열고 있으면 액션이 Cancelled로 끝난다.
 */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataInputCancelBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> InputAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (Categories = "Window.Cancel"))
    FGameplayTag CancelTag;
};

/**
 * 플레이어 캐릭터의 입력 구성을 담는 데이터 에셋.
 *
 * UKataInputHandlerComponent가 참조하며, 폰이 빙의될 때 기본 IMC를 추가하고 InputAction을 바인딩한다.
 * 여러 캐릭터가 같은 에셋을 공유하므로 실행 중 상태를 저장하지 않는다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata Input Config"))
class KATAFRAMEWORK_API UKataInputConfig : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** 캐릭터가 플레이어 컨트롤러에 빙의될 때 추가하고, 빙의가 풀릴 때 제거하는 IMC 목록. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TArray<FKataInputMappingContextEntry> DefaultMappingContexts;

    /** 이동 입력. Axis2D 값의 X는 오른쪽, Y는 앞쪽으로 해석한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Native")
    TObjectPtr<UInputAction> MoveAction;

    /** 시점 입력. Axis2D 값의 X는 Yaw, Y는 Pitch에 더한다. 상하 반전은 IMC의 Modifier로 정한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Native")
    TObjectPtr<UInputAction> LookAction;

    /**
     * 점프 입력. 누르면 ACharacter::Jump, 떼면 StopJumping을 호출한다.
     * Kata 액션이 실행 중이면 점프하지 않는다. 액션 중 점프는 Cancel Bindings로 액션을 먼저 캔슬해야 나간다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Native")
    TObjectPtr<UInputAction> JumpAction;

    /** InputAction 이벤트를 Input 태그로 바꾸는 규칙 목록. InputAction이나 Input 태그가 빈 항목은 바인딩하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Tags")
    TArray<FKataInputTagBinding> InputBindings;

    /**
     * Input 태그를 그래프에 보낼 Trigger 태그로 바꾸는 규칙 목록.
     * 목록에 없는 Input 태그는 그래프로 보내지 않는다. 같은 Input 태그가 여러 번 있으면 앞의 항목을 쓴다.
     * Input 태그와 Trigger 태그의 선택 목록을 따로 거르기 위해 TMap 대신 구조체 배열을 쓴다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Tags")
    TArray<FKataInputTriggerMapping> TriggerMappings;

    /**
     * InputAction을 Kata 액션 캔슬 요청으로 바꾸는 규칙 목록. InputAction이나 Cancel 태그가 빈 항목은 바인딩하지 않는다.
     * 캔슬은 같은 InputAction의 다른 처리(이동, 점프, Input 태그)보다 먼저 일어나므로, 액션을 끊은 입력이 이어서 원래 동작을 한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Cancel")
    TArray<FKataInputCancelBinding> CancelBindings;

    /** Input 태그에 대응하는 Trigger 태그를 찾는다. 없으면 빈 태그를 돌려준다. */
    FGameplayTag FindTriggerTag(const FGameplayTag& InputTag) const;
};
