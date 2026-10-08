#include "Debug/GameplayDebuggerCategory_KataAI.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "Action/KataAction.h"
#include "AIController.h"
#include "Components/StateTreeComponent.h"
#include "Data/KataAIData.h"
#include "GameFramework/Pawn.h"
#include "KataAIPawnInterface.h"
#include "KataGraphComponent.h"
#include "Runtime/KataActionComponent.h"
#include "Runtime/KataActionInstance.h"
#include "StateTree.h"
#include "Targeting/KataAITargetingComponent.h"

FGameplayDebuggerCategory_KataAI::FGameplayDebuggerCategory_KataAI()
{
    // AI 상태는 개체마다 다르므로 디버그 대상을 고른 경우에만 표시한다.
    bShowOnlyWithDebugActor = true;
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_KataAI::MakeInstance()
{
    return MakeShareable(new FGameplayDebuggerCategory_KataAI());
}

void FGameplayDebuggerCategory_KataAI::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
    // 디버그 대상으로 Controller가 선택되는 경우도 있어 빙의한 Pawn으로 정규화한다.
    APawn* Pawn = Cast<APawn>(DebugActor);
    if (Pawn == nullptr)
    {
        if (const AController* Controller = Cast<AController>(DebugActor))
        {
            Pawn = Controller->GetPawn();
        }
    }
    if (Pawn == nullptr)
    {
        AddTextLine(TEXT("{red}Debug actor is not a pawn or controller"));
        return;
    }

    const AAIController* AIController = Pawn->GetController<AAIController>();
    const IKataAIPawnInterface* Settings = Cast<IKataAIPawnInterface>(Pawn);
    const UKataAIData* AIData = Settings != nullptr ? Settings->GetKataAIData() : nullptr;
    AddTextLine(FString::Printf(TEXT("{white}Controller: {yellow}%s  {white}AI Data: {yellow}%s"),
        *GetNameSafe(AIController), *GetNameSafe(AIData)));

    // StateTree 실행 상태와 활성 상태 경로. 슬롯 하위 트리의 상태도 같은 목록에 포함된다.
    const UStateTreeComponent* StateTreeComponent =
        AIController != nullptr ? Cast<UStateTreeComponent>(AIController->GetBrainComponent()) : nullptr;
    if (StateTreeComponent == nullptr)
    {
        AddTextLine(TEXT("{orange}No StateTree brain component"));
    }
    else
    {
        const UStateTree* MainTree = AIData != nullptr ? AIData->StateTree.GetStateTree() : nullptr;
        AddTextLine(FString::Printf(TEXT("{white}State Tree: {yellow}%s  %s"), *GetNameSafe(MainTree),
            *UEnum::GetValueAsString(StateTreeComponent->GetStateTreeRunStatus())));
        TArray<FString> StateNames;
        for (const FName& StateName : StateTreeComponent->GetActiveStateNames())
        {
            StateNames.Add(StateName.ToString());
        }
        AddTextLine(FString::Printf(TEXT("{white}Active: {yellow}%s"),
            StateNames.IsEmpty() ? TEXT("-") : *FString::Join(StateNames, TEXT(" / "))));
    }
    if (AIData != nullptr)
    {
        for (const FKataAIStateTreeSlot& Slot : AIData->LinkedStateTreeSlots)
        {
            AddTextLine(FString::Printf(TEXT("{white}Slot {yellow}%s {white}-> {yellow}%s"),
                *Slot.Slot.ToString(), *GetNameSafe(Slot.StateTree.GetStateTree())));
        }
    }

    // 대상·기억·Home은 타게팅 컴포넌트의 현재 값을 그대로 보인다.
    const UKataAITargetingComponent* Targeting = Pawn->FindComponentByClass<UKataAITargetingComponent>();
    const FVector PawnLocation = Pawn->GetActorLocation();
    if (Targeting == nullptr)
    {
        AddTextLine(TEXT("{orange}No Kata AI targeting component"));
    }
    else
    {
        if (const AActor* Target = Targeting->GetCurrentTarget())
        {
            const FVector TargetLocation = Target->GetActorLocation();
            AddTextLine(FString::Printf(TEXT("{white}Target: {green}%s  {white}dist=%.0f"),
                *GetNameSafe(Target), FVector::Dist(PawnLocation, TargetLocation)));
            AddShape(FGameplayDebuggerShape::MakeSegment(PawnLocation, TargetLocation, 2.0f, FColor::Red, TEXT("Target")));
        }
        else
        {
            AddTextLine(TEXT("{white}Target: {grey}none"));
        }

        if (Targeting->HasLastKnownLocation())
        {
            const FVector LastKnown = Targeting->GetLastKnownLocation();
            AddTextLine(FString::Printf(TEXT("{white}Last Known: {yellow}%s  {white}seen %.1fs ago"),
                *LastKnown.ToCompactString(), Targeting->GetTimeSinceLastSeen()));
            AddShape(FGameplayDebuggerShape::MakePoint(LastKnown, 15.0f, FColor::Yellow, TEXT("Last Known")));
        }

        const FVector Home = Targeting->GetHomeLocation();
        const float HomeDistance = FVector::Dist(PawnLocation, Home);
        const float LeashDistance = AIData != nullptr ? AIData->LeashDistance : 0.0f;
        AddTextLine(LeashDistance > 0.0f
            ? FString::Printf(TEXT("{white}Home dist=%.0f  {white}Leash=%.0f%s"), HomeDistance, LeashDistance,
                HomeDistance > LeashDistance ? TEXT("  {orange}outside") : TEXT(""))
            : FString::Printf(TEXT("{white}Home dist=%.0f  {white}Leash={grey}unlimited"), HomeDistance));
        AddShape(FGameplayDebuggerShape::MakePoint(Home, 15.0f, FColor::Green, TEXT("Home")));
        if (LeashDistance > 0.0f)
        {
            AddShape(FGameplayDebuggerShape::MakeCircle(Home, FVector::UpVector, LeashDistance, FColor::Green));
        }

        const int32 MaxRetries = AIData != nullptr ? AIData->MaxMoveRetries : 0;
        AddTextLine(FString::Printf(TEXT("{white}Move Retries: {yellow}%d / %s  {white}interval=%.2fs"),
            Targeting->GetMoveRetryCount(), MaxRetries > 0 ? *FString::FromInt(MaxRetries) : TEXT("unlimited"),
            AIData != nullptr ? AIData->MoveRetryInterval : 0.0f));
    }

    // 실행 Task가 시작한 Action·Graph는 컴포넌트의 현재 실행으로 확인한다.
    const UKataActionComponent* ActionComponent = Pawn->FindComponentByClass<UKataActionComponent>();
    const UKataActionInstance* ActiveInstance = ActionComponent != nullptr ? ActionComponent->GetActiveInstance() : nullptr;
    const UKataGraphComponent* GraphComponent = Pawn->FindComponentByClass<UKataGraphComponent>();
    AddTextLine(FString::Printf(TEXT("{white}Kata Action: {yellow}%s  {white}Graph: %s"),
        ActiveInstance != nullptr && ActiveInstance->IsRunning() ? *GetNameSafe(ActiveInstance->GetKataAction()) : TEXT("-"),
        GraphComponent != nullptr && GraphComponent->IsRunningGraph() ? TEXT("{green}running") : TEXT("{grey}idle")));
}

#endif // WITH_GAMEPLAY_DEBUGGER
