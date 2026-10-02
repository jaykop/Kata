#include "KataCameraStateTree.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/StateTreeConditionBlueprintBase.h"
#include "Blueprint/StateTreeEvaluatorBlueprintBase.h"
#include "Blueprint/StateTreeTaskBlueprintBase.h"
#include "GameFramework/Pawn.h"
#include "KataCameraData.h"
#include "KataPlayerCameraManager.h"
#include "StateTreeConditionBase.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

namespace KataCameraStateTree
{
    const FName CameraManagerName(TEXT("CameraManager"));
    const FName PawnName(TEXT("Pawn"));
    const FName AbilitySystemName(TEXT("AbilitySystem"));
}

UKataCameraStateTreeSchema::UKataCameraStateTreeSchema()
{
    // GUID는 에셋에 저장된 바인딩이 컨텍스트 항목을 찾는 키다. 한 번 정한 값을 바꾸지 않는다.
    ContextDataDescs.Append({
        FStateTreeExternalDataDesc(KataCameraStateTree::CameraManagerName, AKataPlayerCameraManager::StaticClass(),
            FGuid(0x6B1E2C41, 0x4D3A4F58, 0x9A0C7E21, 0x3F5B8D17)),
        FStateTreeExternalDataDesc(KataCameraStateTree::PawnName, APawn::StaticClass(),
            FGuid(0x2E8F4A93, 0x71C54B06, 0xB4D29E3A, 0x58C1F0D4)),
        FStateTreeExternalDataDesc(KataCameraStateTree::AbilitySystemName, UAbilitySystemComponent::StaticClass(),
            FGuid(0x9C3D5E72, 0x0A6B4E19, 0x8F1D2C64, 0xB7E30A5F))
    });
}

bool UKataCameraStateTreeSchema::IsStructAllowed(const UScriptStruct* InScriptStruct) const
{
    return InScriptStruct->IsChildOf(FStateTreeConditionCommonBase::StaticStruct())
        || InScriptStruct->IsChildOf(FStateTreeEvaluatorCommonBase::StaticStruct())
        || InScriptStruct->IsChildOf(FStateTreeTaskCommonBase::StaticStruct())
        || InScriptStruct->IsChildOf(FKataCameraStateTreeEvaluatorBase::StaticStruct())
        || InScriptStruct->IsChildOf(FKataCameraStateTreeConditionBase::StaticStruct())
        || InScriptStruct->IsChildOf(FKataCameraStateTreeTaskBase::StaticStruct());
}

bool UKataCameraStateTreeSchema::IsClassAllowed(const UClass* InClass) const
{
    return IsChildOfBlueprintBase(InClass);
}

bool UKataCameraStateTreeSchema::IsExternalItemAllowed(const UStruct& InStruct) const
{
    // 카메라 요청 구조체는 매니저가 외부 데이터로 넘긴다. 그 밖의 외부 데이터는 제공하지 않는다.
    return InStruct.IsChildOf(FKataCameraStateTreeRequest::StaticStruct());
}

void FKataCameraStatusTagWatcher::TreeStart(FStateTreeExecutionContext& Context) const
{
    FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
    InstanceData.Subscriptions.Reset();
    InstanceData.BoundAbilitySystem.Reset();

    UAbilitySystemComponent* AbilitySystem = InstanceData.AbilitySystem;
    if (AbilitySystem == nullptr || !InstanceData.ReselectEventTag.IsValid())
    {
        return;
    }

    // ASC 콜백은 트리 갱신 밖에서 오므로 약한 실행 컨텍스트로 이벤트를 넣는다. 매니저가 다음 갱신에서 이벤트를 처리한다.
    const FStateTreeWeakExecutionContext WeakContext = Context.MakeWeakExecutionContext();
    const FGameplayTag EventTag = InstanceData.ReselectEventTag;
    for (const FGameplayTag& Tag : InstanceData.WatchedTags)
    {
        if (!Tag.IsValid())
        {
            continue;
        }

        const FDelegateHandle Handle = AbilitySystem->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
            .AddLambda([WeakContext, EventTag](const FGameplayTag, int32)
            {
                WeakContext.SendEvent(EventTag);
            });
        InstanceData.Subscriptions.Emplace(Tag, Handle);
    }
    InstanceData.BoundAbilitySystem = AbilitySystem;
}

void FKataCameraStatusTagWatcher::TreeStop(FStateTreeExecutionContext& Context) const
{
    FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
    if (UAbilitySystemComponent* AbilitySystem = InstanceData.BoundAbilitySystem.Get())
    {
        for (const TPair<FGameplayTag, FDelegateHandle>& Subscription : InstanceData.Subscriptions)
        {
            AbilitySystem->RegisterGameplayTagEvent(Subscription.Key, EGameplayTagEventType::NewOrRemoved).Remove(Subscription.Value);
        }
    }
    InstanceData.Subscriptions.Reset();
    InstanceData.BoundAbilitySystem.Reset();
}

bool FKataCameraHasStatusTagCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
    const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
    const UAbilitySystemComponent* AbilitySystem = InstanceData.AbilitySystem;
    bool bHasTag = false;
    if (AbilitySystem != nullptr && InstanceData.Tag.IsValid())
    {
        // 소유 태그 컨테이너는 부모 태그를 함께 판정하므로 자식 포함 판정은 HasTag, 정확한 판정은 HasTagExact를 쓴다.
        const FGameplayTagContainer& OwnedTags = AbilitySystem->GetOwnedGameplayTags();
        bHasTag = bIncludeChildTags ? OwnedTags.HasTag(InstanceData.Tag) : OwnedTags.HasTagExact(InstanceData.Tag);
    }
    return bHasTag != bInvert;
}

FKataCameraApplyDataTask::FKataCameraApplyDataTask()
{
    // 진입 시 요청만 하므로 Tick이 필요 없다. 매니저는 이벤트가 있을 때만 트리를 갱신한다.
    bShouldCallTick = false;
}

bool FKataCameraApplyDataTask::Link(FStateTreeLinker& Linker)
{
    Linker.LinkExternalData(RequestHandle);
    return true;
}

EStateTreeRunStatus FKataCameraApplyDataTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
    if (InstanceData.CameraData == nullptr)
    {
        return EStateTreeRunStatus::Failed;
    }

    FKataCameraStateTreeRequest& Request = Context.GetExternalData(RequestHandle);
    Request.CameraData = InstanceData.CameraData;
    Request.BlendTime = FMath::Max(InstanceData.BlendTime, 0.0f);
    Request.BlendCurve = InstanceData.BlendCurve;
    Request.OffsetBlend = InstanceData.OffsetBlend;
    ++Request.Serial;
    return EStateTreeRunStatus::Running;
}
