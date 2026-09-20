#pragma once

#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "Runtime/KataTaskInstance.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"
#include "KataTask.generated.h"

/**
 * 공유 가능한 태스크 설정. 정의가 소유하며 실행 중 변경하지 않는다.
 * 시간, 대상 기록, 외부 핸들 같은 실행 상태는 UKataTaskInstance에 둔다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories)
class KATARUNTIME_API UKataTask : public UObject
{
    GENERATED_BODY()

public:
    /**
     * 부모와 자식 정의가 같은 항목을 식별하는 ID.
     * 편집기에서 자동으로 생성하며 사람이 임의로 바꾸지 않는다.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timeline", meta = (DisplayName = "Task Id"))
    FKataTaskId TaskId;

    /** 타임라인과 진단에 표시할 항목 이름. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline")
    FName TaskName;

    /** 타임라인 시작 시각(초). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline", meta = (ClampMin = "0.0", Units = "s"))
    float StartTime = 0.0f;

    /**
     * 켜면 지속 시간과 무관하게 시작한 프레임에서 Tick을 한 번만 받고 끝난다.
     * Duration이 0인 순간 태스크는 Tick을 한 번도 받지 않으므로 이와 다르다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline")
    bool bSingleFrame = false;

    /** 지속 시간(초). 0이면 시작 즉시 끝나는 순간 태스크다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline", meta = (ClampMin = "0.0", Units = "s", EditCondition = "!bSingleFrame", EditConditionHides))
    float Duration = 0.0f;

    /** 해석된 타임라인에 포함할지 여부. 자식 정의는 Disable 오버라이드로도 끌 수 있다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline")
    bool bEnabled = true;

    /** 같은 시각에 시작하는 태스크의 종류별 선후 관계를 정하는 실행 단계. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ordering")
    EKataTaskPhase Phase = EKataTaskPhase::Gameplay;

    /** 같은 시각, 같은 단계에서만 쓰는 보조 순서 값. 의존성을 대신하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ordering")
    int32 OrderHint = 0;

    /** 요구하는 엔진 업데이트 시점. 미지원 값은 해석 단계에서 오류로 보고한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ordering")
    EKataTaskUpdateHook UpdateHook = EKataTaskUpdateHook::KataTick;

    /** 명시적인 선후 관계. 대상은 같은 Kata의 해석된 타임라인 안에 있어야 한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ordering")
    TArray<FKataTaskDependency> Dependencies;

    /** 이 설정을 실행할 인스턴스 클래스. 파생 태스크가 자신의 상태 클래스를 반환한다. */
    UFUNCTION(BlueprintNativeEvent, Category = "Kata|Timeline")
    TSubclassOf<UKataTaskInstance> GetTaskInstanceClass() const;
    virtual TSubclassOf<UKataTaskInstance> GetTaskInstanceClass_Implementation() const;

    /** Duration이 0에 가까운 순간 태스크인지. 한 프레임 태스크는 포함하지 않는다. */
    bool IsInstant() const;

    float GetEndTime() const;

    /** 타임라인과 진단에 사용할 표시 이름. TaskName이 없으면 클래스 이름을 쓴다. */
    FString GetDisplayName() const;

    /** 에디터와 런타임이 공유하는 설정 검사. NAME_None이면 유효한 설정이다. */
    virtual FName GetConfigurationError() const;

    /**
     * 설정 오류 코드를 사람이 읽을 설명으로 바꾼다.
     * 파생 태스크는 자신이 반환하는 코드만 처리하고 나머지는 Super를 호출한다.
     */
    virtual FString DescribeConfigurationError(FName ErrorCode) const;

    virtual void PostInitProperties() override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

    /** 편집 중 ID가 비어 있으면 새로 발급한다. 이미 있는 ID는 유지한다. */
    void EnsureTaskId();
#endif
};
