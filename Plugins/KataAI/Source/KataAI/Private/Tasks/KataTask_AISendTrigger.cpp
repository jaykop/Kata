#include "Tasks/KataTask_AISendTrigger.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Pawn.h"
#include "KataAILog.h"
#include "KataCondition.h"
#include "KataGraphComponent.h"
#include "Targeting/KataTargetingComponent.h"

UKataTask_AISendTrigger::UKataTask_AISendTrigger()
{
    // 전이 가능 구간은 Transition Window가 정하므로 이 태스크는 입력처럼 한 시점에 한 번 보낸다.
    bSingleFrame = true;
    // 창 태스크와 같은 시각에 두어도 창이 먼저 열린 뒤 보내도록 같은 단계의 기본 순서(0)보다 뒤에 둔다.
    OrderHint = 1;
}

TSubclassOf<UKataTaskInstance> UKataTask_AISendTrigger::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_AISendTrigger::StaticClass();
}

FName UKataTask_AISendTrigger::GetConfigurationError() const
{
    if (!TriggerTag.IsValid())
    {
        return TEXT("MissingTriggerTag");
    }
    if (!FMath::IsFinite(Chance) || Chance < 0.0f || Chance > 1.0f)
    {
        return TEXT("InvalidChance");
    }
    return Super::GetConfigurationError();
}

FString UKataTask_AISendTrigger::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("MissingTriggerTag"))
    {
        return TEXT("'Trigger Tag' must be set to the trigger a graph edge expects");
    }
    if (ErrorCode == TEXT("InvalidChance"))
    {
        return TEXT("'Chance' must be between 0 and 1");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

void UKataTaskInstance_AISendTrigger::OnTaskStarted_Implementation()
{
    Super::OnTaskStarted_Implementation();

    bDone = false;
    const UKataTask_AISendTrigger* Definition = Cast<UKataTask_AISendTrigger>(GetTaskDefinition());
    const APawn* Pawn = Cast<APawn>(GetKataContext().GetAvatarActor());
    if (Definition == nullptr || Pawn == nullptr || Pawn->IsPlayerControlled())
    {
        // PC는 입력으로 트리거를 보낸다. 프리뷰 액터처럼 Pawn이 아니면 보낼 그래프도 없다.
        bDone = true;
        return;
    }

    // 매 프레임 굴리면 구간이 길수록 사실상 항상 성공하므로 시작 때 한 번만 굴린다.
    if (Definition->Chance < 1.0f && FMath::FRand() >= Definition->Chance)
    {
        bDone = true;
        return;
    }

    // 순간 태스크는 Tick을 받지 않으므로 시작에서도 한 번 시도한다.
    TrySend();
}

void UKataTaskInstance_AISendTrigger::OnTaskTick_Implementation(float DeltaTime)
{
    Super::OnTaskTick_Implementation(DeltaTime);

    if (!bDone)
    {
        TrySend();
    }
}

void UKataTaskInstance_AISendTrigger::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    bDone = false;
    Super::OnTaskEnded_Implementation(Reason);
}

void UKataTaskInstance_AISendTrigger::TrySend()
{
    const UKataTask_AISendTrigger* Definition = Cast<UKataTask_AISendTrigger>(GetTaskDefinition());
    const FKataContext Context = GetKataContext();
    AActor* Avatar = Context.GetAvatarActor();
    if (Definition == nullptr || Avatar == nullptr)
    {
        bDone = true;
        return;
    }

    UKataGraphComponent* GraphComponent = Avatar->FindComponentByClass<UKataGraphComponent>();
    if (GraphComponent == nullptr || !GraphComponent->IsRunningGraph())
    {
        // 그래프 없이 단독 실행한 액션이다. 이 구간에서 그래프가 새로 시작되지 않으므로 더 시도하지 않는다.
        bDone = true;
        return;
    }

    if (Definition->Condition != nullptr)
    {
        FKataConditionContext ConditionContext = Context.ToConditionContext();
        // 콤보 도중 대상이 바뀌거나 사라진 것을 반영하려고 액션 시작 때의 대상 대신 AI 타게팅의 현재 대상을 쓴다.
        if (const UKataTargetingComponent* Targeting = Avatar->FindComponentByClass<UKataTargetingComponent>())
        {
            AActor* CurrentTarget = Targeting->GetCurrentTarget();
            ConditionContext.TargetActor = CurrentTarget;
            ConditionContext.TargetAbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CurrentTarget);
        }
        if (!Definition->Condition->IsSatisfied(ConditionContext))
        {
            return;
        }
    }

    // 창이 아직 열리지 않았으면 그래프가 받지 않는다. 구간이 창보다 조금 앞서 시작해도 동작하도록 받아들여질 때까지 다시 시도한다.
    if (GraphComponent->SendTrigger(Definition->TriggerTag))
    {
        bDone = true;
        UE_LOG(LogKataAI, Verbose, TEXT("%s sent graph trigger %s."), *GetNameSafe(Avatar), *Definition->TriggerTag.ToString());
    }
}
