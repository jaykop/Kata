#include "KataDebugPanel_Cheats.h"

#include "Cheats/KataAICheatExtension.h"
#include "Cheats/KataFL_CheatTarget.h"
#include "Cheats/KataFrameworkCheatExtension.h"
#include "GameFramework/CheatManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "KataDebugUserSettings.h"
#include "SlateIM.h"

DEFINE_LOG_CATEGORY_STATIC(LogKataDebugCheats, Log, All);

namespace KataDebugPanelCheats
{
    /** 체크박스 하나를 그리고 바뀌었으면 Setter를 부른다. */
    template<typename SetterType>
    void Toggle(const TCHAR* Label, bool bCurrent, SetterType&& Setter)
    {
        SlateIM::Padding(FMargin(4.f, 1.f));
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

    /** 확장의 현재 플레이어 토글을 저장값과 비교하고, 다르면 저장한다. 콘솔에서 바꾼 값도 패널이 보이는 동안 저장된다. */
    void StorePlayerToggles(const UKataFrameworkCheatExtension& FrameworkCheats, const UKataAICheatExtension& AICheats)
    {
        UKataDebugUserSettings* UserSettings = GetMutableDefault<UKataDebugUserSettings>();
        bool bChanged = false;
        auto Store = [&bChanged](bool& bSaved, bool bCurrent)
        {
            bChanged |= bSaved != bCurrent;
            bSaved = bCurrent;
        };
        Store(UserSettings->bPlayerGod, FrameworkCheats.IsGodEnabled());
        Store(UserSettings->bPlayerNoDamage, FrameworkCheats.IsNoDamageEnabled());
        Store(UserSettings->bPlayerInfiniteStamina, FrameworkCheats.IsInfiniteStaminaEnabled());
        Store(UserSettings->bPlayerHiddenFromAI, AICheats.IsHiddenFromAI());
        if (bChanged)
        {
            UserSettings->SaveConfig();
        }
    }
}

void UKataDebugPanel_Cheats::OnPlaySessionStarted(APlayerController* PlayerController)
{
    UCheatManager* CheatManager = PlayerController != nullptr ? PlayerController->CheatManager.Get() : nullptr;
    UKataFrameworkCheatExtension* FrameworkCheats = CheatManager != nullptr ? CheatManager->FindCheatManagerExtension<UKataFrameworkCheatExtension>() : nullptr;
    UKataAICheatExtension* AICheats = CheatManager != nullptr ? CheatManager->FindCheatManagerExtension<UKataAICheatExtension>() : nullptr;
    if (FrameworkCheats == nullptr || AICheats == nullptr)
    {
        return;
    }

    // 아직 Pawn을 빙의하기 전일 수 있다. 확장이 토글을 기억했다가 빙의할 때 Pawn에 건다.
    const UKataDebugUserSettings* UserSettings = GetDefault<UKataDebugUserSettings>();
    FrameworkCheats->SetGodEnabled(UserSettings->bPlayerGod);
    FrameworkCheats->SetNoDamageEnabled(UserSettings->bPlayerNoDamage);
    FrameworkCheats->SetInfiniteStaminaEnabled(UserSettings->bPlayerInfiniteStamina);
    AICheats->SetHiddenFromAI(UserSettings->bPlayerHiddenFromAI);
    if (UserSettings->bPlayerGod || UserSettings->bPlayerNoDamage || UserSettings->bPlayerInfiniteStamina || UserSettings->bPlayerHiddenFromAI)
    {
        UE_LOG(LogKataDebugCheats, Display, TEXT("Kata Debug restored player cheats: God %d, NoDamage %d, InfiniteStamina %d, HideFromAI %d"),
            UserSettings->bPlayerGod, UserSettings->bPlayerNoDamage, UserSettings->bPlayerInfiniteStamina, UserSettings->bPlayerHiddenFromAI);
    }
}

void UKataDebugPanel_Cheats::Draw(const FKataDebugPanelContext& Context)
{
    using namespace KataDebugPanelCheats;

    APlayerController* PlayerController = Context.PlayerController;
    if (PlayerController == nullptr)
    {
        SlateIM::Padding(FMargin(4.f, 2.f));
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::Text(TEXT("Start PIE to use cheats."));
        return;
    }
    UCheatManager* CheatManager = PlayerController != nullptr ? PlayerController->CheatManager.Get() : nullptr;
    UKataFrameworkCheatExtension* FrameworkCheats = CheatManager != nullptr ? CheatManager->FindCheatManagerExtension<UKataFrameworkCheatExtension>() : nullptr;
    UKataAICheatExtension* AICheats = CheatManager != nullptr ? CheatManager->FindCheatManagerExtension<UKataAICheatExtension>() : nullptr;
    if (FrameworkCheats == nullptr || AICheats == nullptr)
    {
        SlateIM::Padding(FMargin(4.f, 2.f));
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::Text(TEXT("Cheats are unavailable. The player controller has no Cheat Manager with Kata cheat extensions."));
        return;
    }

    KataDebugLayout::SectionHeader(TEXT("Player"));
    Toggle(TEXT("Invincible (KataGod)"), FrameworkCheats->IsGodEnabled(),
        [FrameworkCheats](bool bEnabled) { FrameworkCheats->SetGodEnabled(bEnabled); });
    Toggle(TEXT("Ignore Damage (KataNoDamage)"), FrameworkCheats->IsNoDamageEnabled(),
        [FrameworkCheats](bool bEnabled) { FrameworkCheats->SetNoDamageEnabled(bEnabled); });
    Toggle(TEXT("Infinite Stamina (KataInfiniteStamina)"), FrameworkCheats->IsInfiniteStaminaEnabled(),
        [FrameworkCheats](bool bEnabled) { FrameworkCheats->SetInfiniteStaminaEnabled(bEnabled); });
    Toggle(TEXT("Hidden From AI (KataHideFromAI)"), AICheats->IsHiddenFromAI(),
        [AICheats](bool bEnabled) { AICheats->SetHiddenFromAI(bEnabled); });
    StorePlayerToggles(*FrameworkCheats, *AICheats);

    APawn* PlayerPawn = PlayerController->GetPawn();
    SlateIM::Padding(FMargin(4.f, 4.f, 4.f, 0.f));
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

    // 선택한 대상 전부에 적용한다. 결과는 적용된 수로 알린다.
    const TArray<AActor*>& Targets = Context.SelectedActors;
    const bool bHasTargets = !Targets.IsEmpty();
    KataDebugLayout::SectionHeader(TEXT("Selected Targets"));
    SlateIM::Padding(FMargin(4.f, 2.f));
    SlateIM::BeginHorizontalStack();
    if (RowButton(TEXT("Kill"), bHasTargets, true))
    {
        int32 Count = 0;
        for (AActor* Target : Targets)
        {
            Count += FrameworkCheats->KillActor(Target) ? 1 : 0;
        }
        LastResult = FString::Printf(TEXT("Killed %d of %d."), Count, Targets.Num());
    }
    if (RowButton(TEXT("Groggy"), bHasTargets))
    {
        int32 Count = 0;
        for (AActor* Target : Targets)
        {
            Count += FrameworkCheats->MakeActorGroggy(Target) ? 1 : 0;
        }
        LastResult = FString::Printf(TEXT("Sent groggy to %d of %d."), Count, Targets.Num());
    }
    // 하나라도 실행 중이면 모두 정지하고, 모두 정지해 있으면 모두 재개한다. 콘솔 KataPauseAI와 같은 규칙이다.
    const bool bAllPaused = bHasTargets && !Targets.ContainsByPredicate([](const AActor* Target)
    {
        return !UKataAICheatExtension::IsAIPaused(Target);
    });
    if (RowButton(bAllPaused ? TEXT("Resume AI") : TEXT("Pause AI"), bHasTargets))
    {
        const int32 Changed = UKataAICheatExtension::SetAIPaused(Targets, !bAllPaused);
        LastResult = FString::Printf(TEXT("%s %d AI."), bAllPaused ? TEXT("Resumed") : TEXT("Paused"), Changed);
    }

    // 무적·대미지 무시도 Pause AI와 같은 규칙이다. 하나라도 꺼져 있으면 모두 켜고, 모두 켜져 있으면 모두 끈다.
    const bool bAllInvincible = bHasTargets && !Targets.ContainsByPredicate([](const AActor* Target)
    {
        return !UKataFrameworkCheatExtension::IsActorInvincible(Target);
    });
    const bool bAllIgnoringDamage = bHasTargets && !Targets.ContainsByPredicate([](const AActor* Target)
    {
        return !UKataFrameworkCheatExtension::IsActorIgnoringDamage(Target);
    });
    if (RowButton(bAllInvincible ? TEXT("Remove Invincible") : TEXT("Make Invincible"), bHasTargets))
    {
        int32 Count = 0;
        for (AActor* Target : Targets)
        {
            Count += FrameworkCheats->SetActorInvincible(Target, !bAllInvincible) ? 1 : 0;
        }
        LastResult = FString::Printf(TEXT("Invincible %s for %d of %d."), bAllInvincible ? TEXT("off") : TEXT("on"), Count, Targets.Num());
    }
    if (RowButton(bAllIgnoringDamage ? TEXT("Take Damage") : TEXT("Ignore Damage"), bHasTargets))
    {
        int32 Count = 0;
        for (AActor* Target : Targets)
        {
            Count += FrameworkCheats->SetActorIgnoreDamage(Target, !bAllIgnoringDamage) ? 1 : 0;
        }
        LastResult = FString::Printf(TEXT("Ignore Damage %s for %d of %d."), bAllIgnoringDamage ? TEXT("off") : TEXT("on"), Count, Targets.Num());
    }
    SlateIM::EndHorizontalStack();

    KataDebugLayout::SectionHeader(TEXT("All AI"));
    SlateIM::Padding(FMargin(4.f, 2.f));
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

    // 조건부 문구는 패널 마지막에 두어 앞의 위젯 순서를 바꾸지 않는다.
    if (!LastResult.IsEmpty())
    {
        SlateIM::Padding(FMargin(4.f, 8.f, 4.f, 0.f));
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::Text(LastResult);
    }
}
