// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Action/KataTask.h"
#include "Runtime/KataTaskInstance.h"
#include "KataDebugTask.generated.h"

/**
 * 검증용 디버그 태스크. 시작·종료 경계와 실행 순서를 로그와 화면 메시지로 드러낸다.
 * 게임 기능이 아니라 프로젝트 전용 테스트 코드이며 플러그인에 포함하지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Task: Debug Log"))
class PROJECTKATA_API UKataTask_Debug : public UKataTask
{
    GENERATED_BODY()

public:
    UKataTask_Debug();

    /** 화면 메시지 색. 여러 태스크를 구분할 때 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
    FColor DisplayColor = FColor::Cyan;

    /** 켜면 매 프레임 Tick 로그도 남긴다. 기본은 끔. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bLogTick = false;

    /**
     * 켜면 시작 후 FinishAfterSeconds가 지났을 때 스스로 완료를 알린다.
     * 지속 시간보다 먼저 끝나는 태스크와 완료 의존성을 확인할 때 쓴다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bFinishEarly = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (ClampMin = "0.0", Units = "s", EditCondition = "bFinishEarly", EditConditionHides))
    float FinishAfterSeconds = 0.0f;

    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const override;
};

/** 디버그 태스크의 실행별 상태. 경과 시간만 보관한다. */
UCLASS()
class PROJECTKATA_API UKataTaskInstance_Debug : public UKataTaskInstance
{
    GENERATED_BODY()

protected:
    virtual void OnTaskStarted_Implementation() override;
    virtual void OnTaskTick_Implementation(float DeltaTime) override;
    virtual void OnTaskEnded_Implementation(EKataTaskEndReason Reason) override;

private:
    /** 로그와 화면에 같은 내용을 한 번씩 남긴다. */
    void Report(const TCHAR* Event, const FString& Detail) const;

    float ElapsedSeconds = 0.0f;
};
