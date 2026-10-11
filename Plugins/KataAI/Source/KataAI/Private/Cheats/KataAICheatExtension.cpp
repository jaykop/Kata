#include "Cheats/KataAICheatExtension.h"

#include "AIController.h"
#include "BrainComponent.h"
#include "Cheats/KataAICheatState.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FunctionLibraries/KataFL_CheatTarget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "KataAILog.h"
#include "Targeting/KataAITargetingComponent.h"

FDelegateHandle UKataAICheatExtension::RegisterForCheatManagers()
{
#if UE_WITH_CHEAT_MANAGER
    return UCheatManager::RegisterForOnCheatManagerCreated(FOnCheatManagerCreated::FDelegate::CreateLambda([](UCheatManager* CheatManager)
    {
        CheatManager->AddCheatManagerExtension(NewObject<UKataAICheatExtension>(CheatManager));
    }));
#else
    return FDelegateHandle();
#endif
}

void UKataAICheatExtension::UnregisterFromCheatManagers(FDelegateHandle Handle)
{
#if UE_WITH_CHEAT_MANAGER
    if (Handle.IsValid())
    {
        UCheatManager::UnregisterFromOnCheatManagerCreated(Handle);
    }
#endif
}

void UKataAICheatExtension::KataHideFromAI(const FString& Enable)
{
#if UE_WITH_CHEAT_MANAGER
    if (!KataFL::ParseCheatToggle(Enable, bHiddenFromAI, bHiddenFromAI))
    {
        Report(FString::Printf(TEXT("KataHideFromAI: unknown value '%s'. Use 0 or 1."), *Enable));
        return;
    }
    SyncHidden(GetPlayerController() != nullptr ? GetPlayerController()->GetPawn() : nullptr, bHiddenFromAI);
    Report(FString::Printf(TEXT("KataHideFromAI %s"), bHiddenFromAI ? TEXT("on") : TEXT("off")));
#endif
}

void UKataAICheatExtension::KataPauseAI(const FString& Target, const FString& Enable)
{
#if UE_WITH_CHEAT_MANAGER
    EKataCheatTarget TargetKind;
    if (!KataFL::ParseCheatTarget(Target, EKataCheatTarget::All, TargetKind))
    {
        Report(FString::Printf(TEXT("KataPauseAI: unknown target '%s'. Use Self, Target, Nearest or All."), *Target));
        return;
    }

    // KataAI는 사망 상태 태그를 알지 못한다. 시체의 AI를 멈춰도 해가 없으므로 제외 태그 없이 모은다.
    TArray<UBrainComponent*> Brains;
    for (AActor* Actor : KataFL::ResolveCheatTargets(GetPlayerController(), TargetKind, FGameplayTagContainer()))
    {
        const APawn* Pawn = Cast<APawn>(Actor);
        const AAIController* Controller = Pawn != nullptr ? Cast<AAIController>(Pawn->GetController()) : nullptr;
        if (UBrainComponent* Brain = Controller != nullptr ? Controller->GetBrainComponent() : nullptr)
        {
            Brains.Add(Brain);
        }
    }
    if (Brains.IsEmpty())
    {
        Report(FString::Printf(TEXT("KataPauseAI %s: no AI found"), KataFL::GetCheatTargetName(TargetKind)));
        return;
    }

    const bool bAllPaused = !Brains.ContainsByPredicate([](const UBrainComponent* Brain)
    {
        return !Brain->IsPaused();
    });
    bool bPause = !bAllPaused;
    if (!KataFL::ParseCheatToggle(Enable, bAllPaused, bPause))
    {
        Report(FString::Printf(TEXT("KataPauseAI: unknown value '%s'. Use 0 or 1."), *Enable));
        return;
    }

    for (UBrainComponent* Brain : Brains)
    {
        if (bPause)
        {
            Brain->PauseLogic(TEXT("Kata cheat"));
            if (AAIController* Controller = Brain->GetAIOwner())
            {
                Controller->StopMovement();
            }
        }
        else
        {
            Brain->ResumeLogic(TEXT("Kata cheat"));
        }
    }
    Report(FString::Printf(TEXT("KataPauseAI %s: %s %d AI"), KataFL::GetCheatTargetName(TargetKind), bPause ? TEXT("paused") : TEXT("resumed"),
        Brains.Num()));
#endif
}

void UKataAICheatExtension::AddedToCheatManager_Implementation()
{
    Super::AddedToCheatManager_Implementation();

#if UE_WITH_CHEAT_MANAGER
    if (APlayerController* PlayerController = GetPlayerController())
    {
        PlayerController->OnPossessedPawnChanged.AddUniqueDynamic(this, &UKataAICheatExtension::HandlePossessedPawnChanged);
    }
#endif
}

void UKataAICheatExtension::RemovedFromCheatManager_Implementation()
{
#if UE_WITH_CHEAT_MANAGER
    if (APlayerController* PlayerController = GetPlayerController())
    {
        PlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &UKataAICheatExtension::HandlePossessedPawnChanged);
        if (bHiddenFromAI)
        {
            SyncHidden(PlayerController->GetPawn(), false);
        }
    }
#endif

    Super::RemovedFromCheatManager_Implementation();
}

void UKataAICheatExtension::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
#if UE_WITH_CHEAT_MANAGER
    // 숨김은 Pawn 단위로 기록하므로 재생성된 새 Pawn에 옮긴다.
    if (bHiddenFromAI)
    {
        SyncHidden(OldPawn, false);
        SyncHidden(NewPawn, true);
    }
#endif
}

void UKataAICheatExtension::SyncHidden(APawn* Pawn, bool bHidden) const
{
#if UE_WITH_CHEAT_MANAGER
    if (Pawn == nullptr)
    {
        return;
    }

    // 엔진 자극원 해제는 이미 인지한 AI의 기억을 지우지 못하고, 시각 감각이 처음 만들어질 때 모든 Pawn을 다시 등록할 수 있다.
    // 그래서 인지 결과를 받는 KataAI 타게팅에서 거른다. 나중에 스폰된 AI도 같은 필터를 거친다.
    KataAICheat::SetHiddenFromAI(Pawn, bHidden);
    if (!bHidden)
    {
        // 다시 보이게 할 때는 각 AI의 주기 갱신이 엔진 Perception의 현재 결과로 대상을 되찾는다.
        return;
    }

    UWorld* World = Pawn->GetWorld();
    if (World == nullptr)
    {
        return;
    }
    for (TActorIterator<APawn> It(World); It; ++It)
    {
        if (UKataAITargetingComponent* Targeting = It->FindComponentByClass<UKataAITargetingComponent>())
        {
            Targeting->ForgetActor(Pawn);
        }
    }
#endif
}

void UKataAICheatExtension::Report(const FString& Message) const
{
#if UE_WITH_CHEAT_MANAGER
    UE_LOG(LogKataAI, Display, TEXT("%s"), *Message);
    if (APlayerController* PlayerController = GetPlayerController())
    {
        PlayerController->ClientMessage(Message);
    }
#endif
}
