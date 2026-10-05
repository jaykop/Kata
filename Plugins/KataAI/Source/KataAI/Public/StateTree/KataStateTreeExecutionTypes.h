#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataRuntimeTypes.h"
#include "StateTreeTaskBase.h"
#include "KataStateTreeExecutionTypes.generated.h"

class APawn;
class UKataActionInstance;
class UKataGraphInstance;

/** StateTree 실행 요청과 종료 결과다. 시작·종료 사유의 유효 여부는 별도 출력으로 제공한다. */
UENUM(BlueprintType)
enum class EKataStateTreeExecutionResult : uint8
{
    None,
    Running,
    Completed,
    Busy,
    InvalidSetup,
    NoEligibleEntry,
    StartRejected,
    Interrupted
};

/** 시작·종료 진단과 실행 참조다. 필요한 경우 Details를 펼쳐 출력에 접근한다. */
USTRUCT()
struct KATAAI_API FKataStateTreeExecutionDetails
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Output")
    bool bHasStartResult = false;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    EKataStartResult StartResult = EKataStartResult::InvalidDefinition;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    bool bHasEndReason = false;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    EKataEndReason EndReason = EKataEndReason::Completed;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    TObjectPtr<UKataActionInstance> ActionInstance = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    TObjectPtr<UKataGraphInstance> GraphInstance = nullptr;

};

/** 진입 입력과 캐릭터별 실행 기록이다. 실행 중 바인딩 복사로 기록을 덮어쓰지 않는다. */
USTRUCT()
struct KATAAI_API FKataStateTreeExecutionData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Context")
    TObjectPtr<APawn> Pawn = nullptr;

    UPROPERTY(EditAnywhere, Category = "Input")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    EKataStateTreeExecutionResult Result = EKataStateTreeExecutionResult::None;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    FKataStateTreeExecutionDetails Details;

    UPROPERTY(Transient)
    TWeakObjectPtr<APawn> ExecutionPawn;
};
