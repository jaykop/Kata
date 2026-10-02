#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataCameraTypes.h"
#include "StateTreeConditionBase.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeExecutionTypes.h"
#include "StateTreeSchema.h"
#include "StateTreeTaskBase.h"
#include "StateTreeTypes.h"
#include "KataCameraStateTree.generated.h"

class AKataPlayerCameraManager;
class APawn;
class UAbilitySystemComponent;
class UKataCameraData;

/** 카메라 StateTree 스키마가 제공하는 컨텍스트 데이터 이름. */
namespace KataCameraStateTree
{
    KATACAMERA_API extern const FName CameraManagerName;
    KATACAMERA_API extern const FName PawnName;
    KATACAMERA_API extern const FName AbilitySystemName;
}

/**
 * 카메라 StateTree가 카메라 매니저에 전달하는 카메라 데이터 요청.
 *
 * 매니저가 외부 데이터로 제공하고 Apply Camera Data 태스크가 상태 진입 시 채운다.
 * Serial이 바뀌면 매니저가 새 블렌드 레이어를 올린다.
 */
USTRUCT()
struct KATACAMERA_API FKataCameraStateTreeRequest
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UKataCameraData> CameraData;

    float BlendTime = 0.0f;
    EKataCameraBlendCurve BlendCurve = EKataCameraBlendCurve::EaseInOut;
    EKataCameraOffsetBlend OffsetBlend = EKataCameraOffsetBlend::Linear;

    /** 요청할 때마다 늘어난다. 0은 아직 요청이 없다는 뜻이다. */
    uint32 Serial = 0;
};

/**
 * 플레이어 카메라 StateTree의 스키마.
 *
 * "어떤 Status일 때 어떤 카메라 데이터를 적용하는가"만 다룬다. 컨텍스트로 카메라 매니저, 뷰 타깃 폰, 폰의 ASC를 제공한다.
 * 트리는 카메라 매니저가 직접 실행하며 폰에 ASC가 없으면 시작하지 않는다.
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, meta = (DisplayName = "Kata Camera"))
class KATACAMERA_API UKataCameraStateTreeSchema : public UStateTreeSchema
{
    GENERATED_BODY()

public:
    UKataCameraStateTreeSchema();

protected:
    virtual bool IsStructAllowed(const UScriptStruct* InScriptStruct) const override;
    virtual bool IsClassAllowed(const UClass* InClass) const override;
    virtual bool IsExternalItemAllowed(const UStruct& InStruct) const override;
    virtual TConstArrayView<FStateTreeExternalDataDesc> GetContextDataDescs() const override { return ContextDataDescs; }

private:
    UPROPERTY()
    TArray<FStateTreeExternalDataDesc> ContextDataDescs;
};

/** 카메라 스키마에서만 쓰는 Evaluator의 기반 구조체. */
USTRUCT(meta = (Hidden))
struct KATACAMERA_API FKataCameraStateTreeEvaluatorBase : public FStateTreeEvaluatorBase
{
    GENERATED_BODY()
};

/** 카메라 스키마에서만 쓰는 Condition의 기반 구조체. */
USTRUCT(meta = (Hidden))
struct KATACAMERA_API FKataCameraStateTreeConditionBase : public FStateTreeConditionBase
{
    GENERATED_BODY()
};

/** 카메라 스키마에서만 쓰는 Task의 기반 구조체. */
USTRUCT(meta = (Hidden))
struct KATACAMERA_API FKataCameraStateTreeTaskBase : public FStateTreeTaskBase
{
    GENERATED_BODY()
};

USTRUCT()
struct KATACAMERA_API FKataCameraStatusTagWatcherInstanceData
{
    GENERATED_BODY()

    /** 스키마가 제공하는 폰의 ASC. 자동으로 바인딩된다. */
    UPROPERTY(EditAnywhere, Category = "Context")
    TObjectPtr<UAbilitySystemComponent> AbilitySystem;

    /**
     * 추가·제거를 감시할 Status 태그. 카메라 상태를 가르는 태그만 넣고 락온 태그는 넣지 않는다.
     * 부모 태그를 넣으면 자식 태그 변화에도 반응하므로 정확한 태그를 넣는다.
     */
    UPROPERTY(EditAnywhere, Category = "Parameter")
    TArray<FGameplayTag> WatchedTags;

    /** 감시 태그가 추가·제거될 때 보낼 StateTree 이벤트. 루트로 가는 전이가 이 이벤트를 받아 상태를 다시 고른다. */
    UPROPERTY(EditAnywhere, Category = "Parameter")
    FGameplayTag ReselectEventTag;

    /** 구독을 해제하기 위한 기록. 트리가 멈출 때 비운다. */
    TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;
    TArray<TPair<FGameplayTag, FDelegateHandle>> Subscriptions;
};

/**
 * ASC가 감시 태그를 추가하거나 제거하는 순간에만 재선택 이벤트를 보낸다.
 *
 * 태그 개수가 0→1 또는 1→0으로 바뀔 때(NewOrRemoved)만 반응한다. 이미 붙어 있던 태그로는 이벤트가 오지 않으므로
 * 트리 시작 시의 상태는 루트 선택이 정한다. 트리가 멈추면 구독을 모두 해제한다.
 */
USTRUCT(meta = (DisplayName = "Kata Camera Status Tag Watcher", Category = "Kata Camera"))
struct KATACAMERA_API FKataCameraStatusTagWatcher : public FKataCameraStateTreeEvaluatorBase
{
    GENERATED_BODY()

    using FInstanceDataType = FKataCameraStatusTagWatcherInstanceData;

    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
    virtual void TreeStop(FStateTreeExecutionContext& Context) const override;
};

USTRUCT()
struct KATACAMERA_API FKataCameraHasStatusTagInstanceData
{
    GENERATED_BODY()

    /** 스키마가 제공하는 폰의 ASC. 자동으로 바인딩된다. */
    UPROPERTY(EditAnywhere, Category = "Context")
    TObjectPtr<UAbilitySystemComponent> AbilitySystem;

    UPROPERTY(EditAnywhere, Category = "Parameter")
    FGameplayTag Tag;
};

/** 폰의 ASC가 Status 태그를 가졌는지 판정한다. 상태의 Enter Condition에 쓴다. */
USTRUCT(meta = (DisplayName = "Kata Camera Has Status Tag", Category = "Kata Camera"))
struct KATACAMERA_API FKataCameraHasStatusTagCondition : public FKataCameraStateTreeConditionBase
{
    GENERATED_BODY()

    using FInstanceDataType = FKataCameraHasStatusTagInstanceData;

    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

    /** true이면 Tag의 자식 태그를 가져도 참이다(State.Combat이 State.Combat.Aim을 포함). false이면 정확히 일치해야 한다. */
    UPROPERTY(EditAnywhere, Category = "Condition")
    bool bIncludeChildTags = true;

    UPROPERTY(EditAnywhere, Category = "Condition")
    bool bInvert = false;
};

USTRUCT()
struct KATACAMERA_API FKataCameraApplyDataInstanceData
{
    GENERATED_BODY()

    /** 이 상태에서 적용할 카메라 데이터. */
    UPROPERTY(EditAnywhere, Category = "Parameter")
    TObjectPtr<UKataCameraData> CameraData;

    /** 이전 데이터에서 넘어오는 시간(초). 0이면 즉시 바뀐다. */
    UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", Units = "Seconds"))
    float BlendTime = 0.4f;

    UPROPERTY(EditAnywhere, Category = "Parameter")
    EKataCameraBlendCurve BlendCurve = EKataCameraBlendCurve::EaseInOut;

    UPROPERTY(EditAnywhere, Category = "Parameter")
    EKataCameraOffsetBlend OffsetBlend = EKataCameraOffsetBlend::Linear;
};

/**
 * 상태에 들어갈 때 카메라 데이터 적용을 요청한다. 계산은 하지 않으며 상태가 활성인 동안 Running을 유지한다.
 * 카메라 데이터가 비어 있으면 실패한다.
 */
USTRUCT(meta = (DisplayName = "Kata Camera Apply Data", Category = "Kata Camera"))
struct KATACAMERA_API FKataCameraApplyDataTask : public FKataCameraStateTreeTaskBase
{
    GENERATED_BODY()

    using FInstanceDataType = FKataCameraApplyDataInstanceData;

    FKataCameraApplyDataTask();

    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual bool Link(FStateTreeLinker& Linker) override;
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
    TStateTreeExternalDataHandle<FKataCameraStateTreeRequest> RequestHandle;
};
