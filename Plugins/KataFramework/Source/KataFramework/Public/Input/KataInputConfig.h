#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
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
 * 플레이어 캐릭터의 입력 구성을 담는 데이터 에셋.
 *
 * AKataPlayerCharacter가 참조하며, 빙의될 때 기본 IMC를 추가하고 InputAction을 바인딩한다.
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
};
