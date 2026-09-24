#pragma once

#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "UObject/Object.h"
#include "KataResolvedAction.generated.h"

class UKataAction;
class UKataCommand;
class UKataCondition;
class UKataTask;

/**
 * 부모 정의와 자식 변경분을 합친 읽기 전용 실행 데이터.
 * 편집용 데이터와 분리되며 실행 상태를 보관하지 않는다. 태스크 사본은 이 객체가 소유한다.
 */
UCLASS(BlueprintType)
class KATARUNTIME_API UKataResolvedAction : public UObject
{
    GENERATED_BODY()

public:
    /** 실행 원본 에셋. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Action")
    TObjectPtr<UKataAction> SourceAction;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Tag")
    FGameplayTagContainer KataTags;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Tag")
    FGameplayTagContainer ActivationRequiredTags;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Tag")
    FGameplayTagContainer ActivationBlockedTags;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Tag")
    FGameplayTagContainer ActiveGrantedTags;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Tag")
    FKataBlockingPolicy BlockingPolicy;

    /** 정의의 조건 객체를 복제한 실행용 사본. 평가는 부작용이 없다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Condition")
    TObjectPtr<UKataCondition> StartCondition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Cooldown")
    FKataCooldownPolicy CooldownPolicy;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Loop")
    FKataLoopPolicy LoopPolicy;

    /** 시작 시 실행할 명령 사본. 선언 순서를 유지하며 빈 항목은 제외한다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Command")
    TArray<TObjectPtr<UKataCommand>> PreCommands;

    /** 종료 시 실행할 명령 사본과 종료 사유 필터. 선언 순서를 유지하며 빈 항목은 제외한다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Command")
    TArray<FKataPostCommandEntry> PostCommands;

    /**
     * 실행 순서로 정렬된 태스크 사본.
     * Disable·Remove 처리된 항목은 포함하지 않는다.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Timeline")
    TArray<TObjectPtr<UKataTask>> Tasks;

    /** 해석 중 수집한 진단. Error가 하나라도 있으면 실행을 거절한다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Diagnostics")
    TArray<FKataDiagnostic> Diagnostics;

    /** Error 진단이 있는지. */
    UFUNCTION(BlueprintPure, Category = "Kata|Action")
    bool HasErrors() const;

    /** 가장 늦은 태스크 종료 시각. 태스크가 없으면 0이다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Action")
    float GetTimelineDuration() const;

    const UKataTask* FindTask(const FKataTaskId& TaskId) const;

    int32 FindTaskIndex(const FKataTaskId& TaskId) const;

    void AddDiagnostic(EKataDiagnosticSeverity Severity, FName Code, const FKataTaskId& TaskId, FString Detail,
        FString TaskLabel = FString(), bool bIncompleteAuthoring = false);

    /** 수집한 진단을 LogKata로 한 번 출력한다. */
    void LogDiagnostics() const;
};
