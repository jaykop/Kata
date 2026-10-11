#include "KataDebugPanel_Cheats.h"

#include "Cheats/KataAICheatExtension.h"
#include "Cheats/KataFrameworkCheatExtension.h"
#include "FunctionLibraries/KataFL_CheatTarget.h"
#include "GameFramework/CheatManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "SlateIM.h"
#include "Styling/StyleColors.h"

namespace KataDebugPanelCheats
{
    void Section(const TCHAR* Label)
    {
        SlateIM::Padding(FMargin(0.f, 8.f, 0.f, 2.f));
        SlateIM::Text(Label, FStyleColors::AccentBlue);
    }

    /** 체크박스 하나를 그리고 바뀌었으면 Setter를 부른다. */
    template<typename SetterType>
    void Toggle(const TCHAR* Label, bool bCurrent, SetterType&& Setter)
    {
        SlateIM::Padding(FMargin(0.f, 1.f));
        bool bValue = bCurrent;
        if (SlateIM::CheckBox(Label, bValue))
        {
            Setter(bValue);
        }
    }

    /** 가로로 이어지는 버튼 사이 간격을 맞춘다. 첫 버튼에는 간격을 두지 않는다. */
    bool RowButton(const TCHAR* Label, bool bEnabled, bool bFirst = false)
    {
        SlateIM::Padding(FMargin(bFirst ? 0.f : 4.f, 0.f, 0.f, 0.f));
        return SlateIM::Button(Label, { .bEnabled = bEnabled });
    }
}

void UKataDebugPanel_Cheats::Draw(const FKataDebugPanelContext& Context)
{
    using namespace KataDebugPanelCheats;

    APlayerController* PlayerController = Context.PlayerController;
    UCheatManager* CheatManager = PlayerController != nullptr ? PlayerController->CheatManager.Get() : nullptr;
    UKataFrameworkCheatExtension* FrameworkCheats = CheatManager != nullptr ? CheatManager->FindCheatManagerExtension<UKataFrameworkCheatExtension>() : nullptr;
    UKataAICheatExtension* AICheats = CheatManager != nullptr ? CheatManager->FindCheatManagerExtension<UKataAICheatExtension>() : nullptr;
    if (FrameworkCheats == nullptr || AICheats == nullptr)
    {
        SlateIM::Text(TEXT("Cheats are unavailable. The player controller has no Cheat Manager with Kata cheat extensions."));
        return;
    }

    Section(TEXT("Player"));
    Toggle(TEXT("Invincible (KataGod)"), FrameworkCheats->IsGodEnabled(),
        [FrameworkCheats](bool bEnabled) { FrameworkCheats->SetGodEnabled(bEnabled); });
    Toggle(TEXT("Ignore Damage (KataNoDamage)"), FrameworkCheats->IsNoDamageEnabled(),
        [FrameworkCheats](bool bEnabled) { FrameworkCheats->SetNoDamageEnabled(bEnabled); });
    Toggle(TEXT("Infinite Stamina (KataInfiniteStamina)"), FrameworkCheats->IsInfiniteStaminaEnabled(),
        [FrameworkCheats](bool bEnabled) { FrameworkCheats->SetInfiniteStaminaEnabled(bEnabled); });
    Toggle(TEXT("Hidden From AI (KataHideFromAI)"), AICheats->IsHiddenFromAI(),
        [AICheats](bool bEnabled) { AICheats->SetHiddenFromAI(bEnabled); });

    APawn* PlayerPawn = PlayerController->GetPawn();
    SlateIM::Padding(FMargin(0.f, 4.f, 0.f, 0.f));
    SlateIM::BeginHorizontalStack();
    if (RowButton(TEXT("Kill Self"), PlayerPawn != nullptr, true))
    {
        LastResult = FrameworkCheats->KillActor(PlayerPawn) ? TEXT("Killed the player.") : TEXT("The player could not be killed.");
    }
    if (RowButton(TEXT("Respawn"), true))
    {
        LastResult = FrameworkCheats->RespawnPlayer() ? TEXT("Respawn requested.") : TEXT("Respawn needs a Kata Game Mode.");
    }
    SlateIM::EndHorizontalStack();

    AActor* Target = Context.SelectedActor;
    Section(TEXT("Selected Target"));
    SlateIM::BeginHorizontalStack();
    if (RowButton(TEXT("Kill"), Target != nullptr, true))
    {
        LastResult = FrameworkCheats->KillActor(Target) ? TEXT("Killed the target.") : TEXT("The target has no death component or is already dead.");
    }
    if (RowButton(TEXT("Groggy"), Target != nullptr))
    {
        LastResult = FrameworkCheats->MakeActorGroggy(Target) ? TEXT("Sent groggy to the target.") : TEXT("The target has no Stance set or Max Groggy.");
    }
    const bool bTargetPaused = UKataAICheatExtension::IsAIPaused(Target);
    if (RowButton(bTargetPaused ? TEXT("Resume AI") : TEXT("Pause AI"), Target != nullptr))
    {
        const int32 Changed = UKataAICheatExtension::SetAIPaused({ Target }, !bTargetPaused);
        LastResult = Changed > 0 ? (bTargetPaused ? TEXT("Resumed the target AI.") : TEXT("Paused the target AI.")) : TEXT("The target has no AI.");
    }
    SlateIM::EndHorizontalStack();

    Section(TEXT("All AI"));
    SlateIM::BeginHorizontalStack();
    const bool bPauseAll = RowButton(TEXT("Pause All"), true, true);
    const bool bResumeAll = RowButton(TEXT("Resume All"), true);
    if (bPauseAll || bResumeAll)
    {
        // KataAI 명령과 같이 시체를 빼지 않는다. 시체의 AI를 멈춰도 해가 없다.
        const TArray<AActor*> Actors = KataFL::ResolveCheatTargets(PlayerController, EKataCheatTarget::All, FGameplayTagContainer());
        const int32 Changed = UKataAICheatExtension::SetAIPaused(Actors, bPauseAll);
        LastResult = FString::Printf(TEXT("%s %d AI."), bPauseAll ? TEXT("Paused") : TEXT("Resumed"), Changed);
    }
    SlateIM::EndHorizontalStack();

    if (!LastResult.IsEmpty())
    {
        SlateIM::Padding(FMargin(0.f, 8.f, 0.f, 0.f));
        SlateIM::Text(LastResult);
    }
}
