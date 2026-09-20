#pragma once

#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "UObject/WeakObjectPtr.h"

class UKataResolvedAction;
class UKataTask;

/** 스케줄러가 다루는 경계의 종류. 같은 시각에서는 종료를 먼저 처리한다. */
enum class EKataBoundaryType : uint8
{
    End = 0,
    Start = 1
};

/** 해석된 태스크 하나에 대한 실행 계획. 실행 상태는 담지 않는다. */
struct KATARUNTIME_API FKataScheduledTask
{
    FKataTaskId TaskId;
    TWeakObjectPtr<UKataTask> Task;

    /** 스케줄러가 정한 실행 순서. 같은 시각의 경계 정렬에 사용한다. */
    int32 ExecutionIndex = 0;

    float StartTime = 0.0f;
    float EndTime = 0.0f;
    bool bInstant = false;

    /** true이면 시간이 아니라 Tick 한 번으로 태스크를 끝낸다. */
    bool bSingleFrame = false;

    /** 완료를 기다려야 하는 선행 태스크의 실행 인덱스. */
    TArray<int32> CompletionPrerequisites;
};

/** 시간축에서 처리할 경계 하나. */
struct KATARUNTIME_API FKataTimelineBoundary
{
    float Time = 0.0f;
    int32 TaskIndex = INDEX_NONE;
    EKataBoundaryType Type = EKataBoundaryType::Start;
};

/**
 * 인스턴스가 소유하는 태스크 스케줄러.
 *
 * 시간 경계, 같은 시각의 실행 순서, 완료 의존성을 구분해 다룬다.
 * 해석 단계에서 이미 순환·시간 모순·미지원 업데이트 시점을 걸러 냈다고 가정한다.
 * 여러 인스턴스 사이의 실행 순서는 월드 실행 Subsystem이 별도로 조정한다.
 */
class KATARUNTIME_API FKataTaskScheduler
{
public:
    /** 해석된 정의로 실행 계획을 만든다. 정의의 태스크 순서를 실행 순서로 사용한다. */
    void Build(const UKataResolvedAction* ResolvedDefinition);

    void Reset();

    const TArray<FKataScheduledTask>& GetTasks() const { return Tasks; }

    int32 Num() const { return Tasks.Num(); }

    float GetTimelineDuration() const { return TimelineDuration; }

    /**
     * 구간에 걸친 경계를 실행 순서대로 모은다.
     *
     * @param FromTime         구간 시작 시각.
     * @param ToTime           구간 종료 시각.
     * @param bIncludeFromTime true면 FromTime의 경계도 포함한다. 시작과 루프 재진입에 사용한다.
     *
     * 큰 DeltaTime에도 구간 안의 모든 경계를 놓치지 않는다.
     */
    void CollectBoundaries(float FromTime, float ToTime, bool bIncludeFromTime, TArray<FKataTimelineBoundary>& OutBoundaries) const;

private:
    TArray<FKataScheduledTask> Tasks;

    /** 시간, 종료 우선, 실행 순서로 미리 정렬한 경계 목록. */
    TArray<FKataTimelineBoundary> Boundaries;

    float TimelineDuration = 0.0f;
};
