#include "Controller/KataAIController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/StateTreeAIComponent.h"
#include "Data/KataAIData.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "KataAILog.h"
#include "KataAIPawnInterface.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISenseConfig.h"
#include "Perception/AISense_Sight.h"
#include "StateTree.h"
#include "Targeting/KataAITargetingComponent.h"
#include "Targeting/KataTargetingComponent.h"

AKataAIController::AKataAIController()
{
    StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("KataStateTree"));
    BrainComponent = StateTreeComponent;
    StateTreeComponent->SetStartLogicAutomatically(false);
    StateTreeComponent->OnStateTreeRunStatusChanged.AddDynamic(this, &AKataAIController::HandleStateTreeRunStatusChanged);
    bStartAILogicOnPossess = false;
    bStopAILogicOnUnposses = false;
}

void AKataAIController::BeginPlay()
{
    Super::BeginPlay();
    bGameplayReady = true;
    TryStartKataAI();
}

void AKataAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    bStartAttempted = false;
    TryStartKataAI();
}

void AKataAIController::TryStartKataAI()
{
    APawn* ControlledPawn = GetPawn();
    UWorld* World = GetWorld();
    if (bStartAttempted || !bGameplayReady || World == nullptr || !World->IsGameWorld()
        || !IsValid(ControlledPawn))
    {
        return;
    }

    const IKataAIPawnInterface* AISettings = Cast<IKataAIPawnInterface>(ControlledPawn);
    if (AISettings != nullptr && !AISettings->IsKataAIReady())
    {
        return;
    }
    const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(ControlledPawn);
    UAbilitySystemComponent* AbilitySystem = AbilityOwner != nullptr ? AbilityOwner->GetAbilitySystemComponent() : nullptr;
    if (AISettings == nullptr || AbilitySystem == nullptr || AbilitySystem->GetAvatarActor() != ControlledPawn)
    {
        UE_LOG(LogKataAI, Warning, TEXT("Cannot start Kata AI for '%s': Pawn settings or initialized ASC is missing."),
            *GetNameSafe(ControlledPawn));
        bStartAttempted = true;
        return;
    }

    bStartAttempted = true;
    UKataAIData* Data = AISettings->GetKataAIData();
    ClearKataPerception();
    if (Data == nullptr)
    {
        UE_LOG(LogKataAI, Log, TEXT("AI start skipped: Controller=%s Pawn=%s AIData=None. NPC row AIData must be applied before BeginPlay."),
            *GetNameSafe(this), *GetNameSafe(ControlledPawn));
        StateTreeComponent->SetStateTree(nullptr);
        return;
    }

    UnbindTargetEvents();
    TargetAcquiredTag = FGameplayTag::RequestGameplayTag(FName(TEXT("StateTree.Event.AI.TargetAcquired")), false);
    TargetLostTag = FGameplayTag::RequestGameplayTag(FName(TEXT("StateTree.Event.AI.TargetLost")), false);
    TargetChangedTag = FGameplayTag::RequestGameplayTag(FName(TEXT("StateTree.Event.AI.TargetChanged")), false);
    if (!TargetAcquiredTag.IsValid() || !TargetLostTag.IsValid() || !TargetChangedTag.IsValid())
    {
        UE_LOG(LogKataAI, Warning, TEXT("AI target events require StateTree.Event.AI.TargetAcquired, StateTree.Event.AI.TargetLost and StateTree.Event.AI.TargetChanged tags."));
    }
    ConfigureKataPerception(*Data);
    if (UKataAITargetingComponent* Targeting = ControlledPawn->FindComponentByClass<UKataAITargetingComponent>())
    {
        BoundTargeting = Targeting;
        TargetChangedHandle = Targeting->OnTargetChanged.AddUObject(this, &AKataAIController::HandleSelectedTargetChanged);
        Targeting->InitializeKataAI(Data, RuntimePerceptionComponent);
    }
    // 파라미터 동기화는 사본에서 수행해 공유 에셋을 실행 중 수정하지 않는다.
    FStateTreeReference TreeReference = Data->StateTree;
    TreeReference.SyncParameters();
    StateTreeComponent->SetStateTreeReference(TreeReference);
    UE_LOG(LogKataAI, Log, TEXT("AI StateTree assigned: Controller=%s Pawn=%s AIData=%s Tree=%s AIOwner=%s Registered=%s."),
        *GetNameSafe(this), *GetNameSafe(ControlledPawn), *Data->GetPathName(),
        *GetPathNameSafe(TreeReference.GetStateTree()), *GetNameSafe(StateTreeComponent->GetAIOwner()),
        StateTreeComponent->IsRegistered() ? TEXT("true") : TEXT("false"));
    if (TreeReference.IsValid())
    {
        StateTreeComponent->StartLogic();
        UE_LOG(LogKataAI, Log, TEXT("AI StateTree start result: Controller=%s Tree=%s Status=%s Running=%s. See LogStateTree for context or asset errors."),
            *GetNameSafe(this), *GetPathNameSafe(TreeReference.GetStateTree()),
            *UEnum::GetValueAsString(StateTreeComponent->GetStateTreeRunStatus()),
            StateTreeComponent->IsRunning() ? TEXT("true") : TEXT("false"));
    }
    else
    {
        UE_LOG(LogKataAI, Log, TEXT("AI StateTree start skipped: AIData=%s has no StateTree reference; Perception remains independent."),
            *Data->GetPathName());
    }
}

void AKataAIController::HandleStateTreeRunStatusChanged(EStateTreeRunStatus Status)
{
    UE_LOG(LogKataAI, Log, TEXT("AI StateTree status changed: Controller=%s Pawn=%s Status=%s."),
        *GetNameSafe(this), *GetNameSafe(GetPawn()), *UEnum::GetValueAsString(Status));
}

void AKataAIController::ConfigureKataPerception(const UKataAIData& Data)
{
    if (Data.Senses.IsEmpty())
    {
        return;
    }
    if (GetAIPerceptionComponent() != nullptr)
    {
        UE_LOG(LogKataAI, Warning, TEXT("Cannot apply AI Data senses: Controller already has a Perception component. Remove the Blueprint component."));
        return;
    }

    // Listener를 새로 만들어 재빙의 시 이전 감각이나 대상 기록이 남지 않게 한다.
    RuntimePerceptionComponent = NewObject<UAIPerceptionComponent>(this, NAME_None, RF_Transient);
    TSet<const UClass*> Implementations;
    TSubclassOf<UAISense> FirstSense;
    for (const UAISenseConfig* TemplateConfig : Data.Senses)
    {
        if (TemplateConfig == nullptr)
        {
            UE_LOG(LogKataAI, Warning, TEXT("AI Data '%s' contains an empty sense configuration."), *Data.GetName());
            continue;
        }
        UAISenseConfig* RuntimeConfig = DuplicateObject<UAISenseConfig>(TemplateConfig, RuntimePerceptionComponent);
        const TSubclassOf<UAISense> Implementation = RuntimeConfig->GetSenseImplementation();
        if (Implementation == nullptr || Implementations.Contains(Implementation.Get()))
        {
            UE_LOG(LogKataAI, Warning, TEXT("AI Data '%s' contains an invalid or duplicate sense."), *Data.GetName());
            continue;
        }
        Implementations.Add(Implementation.Get());
        RuntimePerceptionComponent->ConfigureSense(*RuntimeConfig);
        if (FirstSense == nullptr)
        {
            FirstSense = Implementation;
        }
    }
    if (Implementations.IsEmpty())
    {
        ClearKataPerception();
        return;
    }

    TSubclassOf<UAISense> DominantSense = Data.DominantSense;
    if (DominantSense == nullptr || !Implementations.Contains(DominantSense.Get()))
    {
        if (DominantSense != nullptr)
        {
            UE_LOG(LogKataAI, Warning, TEXT("AI Data '%s' has an unconfigured dominant sense; using the first valid sense."), *Data.GetName());
        }
        DominantSense = FirstSense;
    }
    RuntimePerceptionComponent->SetDominantSense(DominantSense);
    SetPerceptionComponent(*RuntimePerceptionComponent);
    RuntimePerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AKataAIController::HandleTargetPerceptionUpdated);
    RuntimePerceptionComponent->RegisterComponent();
}

void AKataAIController::ClearKataPerception()
{
    if (RuntimePerceptionComponent != nullptr)
    {
        RuntimePerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &AKataAIController::HandleTargetPerceptionUpdated);
        RuntimePerceptionComponent->DestroyComponent();
        if (PerceptionComponent == RuntimePerceptionComponent)
        {
            PerceptionComponent = nullptr;
        }
        RuntimePerceptionComponent = nullptr;
    }
}

FGenericTeamId AKataAIController::GetGenericTeamId() const
{
    const APawn* ControlledPawn = GetPawn();
    const UKataTargetingComponent* Targeting = ControlledPawn != nullptr
        ? ControlledPawn->FindComponentByClass<UKataTargetingComponent>() : nullptr;
    return Targeting != nullptr ? Targeting->GetFactionTeamId() : FGenericTeamId::NoTeam;
}

void AKataAIController::UnbindTargetEvents()
{
    if (UKataAITargetingComponent* Targeting = BoundTargeting.Get())
    {
        Targeting->OnTargetChanged.Remove(TargetChangedHandle);
    }
    TargetChangedHandle.Reset();
    BoundTargeting.Reset();
    TargetAcquiredTag = FGameplayTag();
    TargetLostTag = FGameplayTag();
    TargetChangedTag = FGameplayTag();
}

void AKataAIController::HandleSelectedTargetChanged(bool bHadTarget, AActor* Target)
{
    if (!StateTreeComponent->IsRunning())
    {
        return;
    }
    const FGameplayTag Tag = Target == nullptr ? TargetLostTag
        : (bHadTarget ? TargetChangedTag : TargetAcquiredTag);
    if (Tag.IsValid())
    {
        StateTreeComponent->SendStateTreeEvent(Tag);
    }
}

void AKataAIController::StopKataAI()
{
    UnbindTargetEvents();
    StateTreeComponent->StopLogic(TEXT("Kata AI owner detached"));
    if (APawn* ControlledPawn = GetPawn())
    {
        if (UKataAITargetingComponent* Targeting = ControlledPawn->FindComponentByClass<UKataAITargetingComponent>())
        {
            Targeting->ResetKataAI();
        }
    }
    ClearKataPerception();
    StopMovement();
    ClearFocus(EAIFocusPriority::Gameplay);
    StateTreeComponent->SetStateTree(nullptr);
    bStartAttempted = false;
}

void AKataAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>())
    {
        return;
    }
    if (APawn* ControlledPawn = GetPawn())
    {
        if (UKataAITargetingComponent* Targeting = ControlledPawn->FindComponentByClass<UKataAITargetingComponent>())
        {
            Targeting->UpdateSight(Actor, Stimulus.WasSuccessfullySensed(), Stimulus.StimulusLocation);
        }
    }
}

void AKataAIController::OnUnPossess()
{
    // StateTree의 종료 처리가 이전 Pawn을 참조할 수 있는 순서를 지킨다.
    StopKataAI();
    Super::OnUnPossess();
}

void AKataAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bGameplayReady = false;
    StopKataAI();
    Super::EndPlay(EndPlayReason);
}
