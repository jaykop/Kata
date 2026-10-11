#include "KataDebugMenuSubsystem.h"

#include "Attributes/KataCombatSettings.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "FunctionLibraries/KataFL_CheatTarget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "KataDebugMenuInputProcessor.h"
#include "KataDebugPanel.h"
#include "ProjectKataDebugLog.h"
#include "SlateIM.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Targeting/KataTargetingComponent.h"
#include "UObject/UObjectHash.h"
#include "Widgets/SViewport.h"

namespace KataDebugMenu
{
    constexpr float MenuWidth = 440.f;
    constexpr float MenuHeight = 680.f;
    constexpr float TargetListHeight = 160.f;

    /** 대상 목록을 다시 모으는 간격(초). 거리순 정렬이 매 프레임 흔들리지 않게 한다. */
    constexpr double TargetRefreshInterval = 0.5;

    UKataDebugMenuSubsystem* FindMenu(UWorld* World)
    {
        APlayerController* PlayerController = World != nullptr ? World->GetFirstPlayerController() : nullptr;
        ULocalPlayer* LocalPlayer = PlayerController != nullptr ? PlayerController->GetLocalPlayer() : nullptr;
        return LocalPlayer != nullptr ? LocalPlayer->GetSubsystem<UKataDebugMenuSubsystem>() : nullptr;
    }

    FAutoConsoleCommandWithWorld ToggleCommand(
        TEXT("Kata.DebugMenu"),
        TEXT("Opens or closes the Kata in-game debug menu. The Home key does the same while the game viewport has focus."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
        {
            if (UKataDebugMenuSubsystem* Menu = FindMenu(World))
            {
                Menu->ToggleMenu();
            }
        }));
}

void UKataDebugMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    RootName = FName(*FString::Printf(TEXT("KataDebugMenu_%u"), GetLocalPlayer()->GetUniqueID()));

    // 패널은 클래스만 추가하면 목록에 오르도록 파생 클래스를 찾아 만든다. 서브시스템이 Outer라 플레이어 수명을 따른다.
    TArray<UClass*> PanelClasses;
    GetDerivedClasses(UKataDebugPanel::StaticClass(), PanelClasses, true);
    for (UClass* PanelClass : PanelClasses)
    {
        if (PanelClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
        {
            continue;
        }
        Panels.Add(NewObject<UKataDebugPanel>(this, PanelClass));
    }
    // TArray::Sort는 포인터 원소를 역참조해 비교 함수에 넘긴다.
    Panels.Sort([](const UKataDebugPanel& A, const UKataDebugPanel& B)
    {
        const int32 OrderA = A.GetSortOrder();
        const int32 OrderB = B.GetSortOrder();
        return OrderA != OrderB ? OrderA < OrderB : A.GetDisplayName() < B.GetDisplayName();
    });

    if (FSlateApplication::IsInitialized())
    {
        PreTickHandle = FSlateApplication::Get().OnPreTick().AddUObject(this, &UKataDebugMenuSubsystem::HandleSlatePreTick);
        InputProcessor = MakeShared<FKataDebugMenuInputProcessor>(this);
        FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
    }
}

void UKataDebugMenuSubsystem::Deinitialize()
{
    SetMenuOpen(false);

    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().OnPreTick().Remove(PreTickHandle);
        if (InputProcessor.IsValid())
        {
            FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
        }
    }
    PreTickHandle.Reset();
    InputProcessor.Reset();
    Panels.Reset();

    Super::Deinitialize();
}

void UKataDebugMenuSubsystem::ToggleMenu()
{
    SetMenuOpen(!bMenuOpen);
}

void UKataDebugMenuSubsystem::SetMenuOpen(bool bOpen)
{
    if (bMenuOpen == bOpen)
    {
        return;
    }

    bMenuOpen = bOpen;
    ApplyOpenState(bOpen);
    bRefreshTargetList = true;

    if (UKataDebugPanel* Panel = GetActivePanel())
    {
        if (bOpen)
        {
            Panel->OnShown();
        }
        else
        {
            Panel->OnHidden();
        }
    }
}

bool UKataDebugMenuSubsystem::IsViewportFocused() const
{
    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    const UGameViewportClient* ViewportClient = LocalPlayer != nullptr ? LocalPlayer->ViewportClient.Get() : nullptr;
    const TSharedPtr<SViewport> ViewportWidget = ViewportClient != nullptr ? ViewportClient->GetGameViewportWidget() : nullptr;
    // 메뉴 위젯은 게임 뷰포트 안의 레이어에 붙으므로, 메뉴를 클릭한 뒤에도 뷰포트의 하위 포커스로 잡힌다.
    return ViewportWidget.IsValid() && ViewportWidget->HasAnyUserFocusOrFocusedDescendants();
}

void UKataDebugMenuSubsystem::HandleSlatePreTick(float DeltaTime)
{
    if (!bMenuOpen || !SlateIM::CanUpdateSlateIM())
    {
        return;
    }

    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    APlayerController* PlayerController = GetPlayerController();
    UWorld* World = PlayerController != nullptr ? PlayerController->GetWorld() : nullptr;
    if (LocalPlayer == nullptr || LocalPlayer->ViewportClient == nullptr || World == nullptr)
    {
        return;
    }

    FKataDebugPanelContext Context;
    Context.World = World;
    Context.PlayerController = PlayerController;
    Context.SelectedActor = SelectedActor.Get();

    SlateIM::FViewportParams Params;
    Params.Layout.Anchors = FAnchors(0.f, 0.f);
    Params.Layout.Offset = FVector2f(24.f, 24.f);
    Params.Layout.Size = FVector2f(KataDebugMenu::MenuWidth, KataDebugMenu::MenuHeight);

    if (SlateIM::BeginViewportRoot(RootName, LocalPlayer, Params))
    {
        SlateIM::Fill();
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::VAlign(VAlign_Fill);
        SlateIM::BeginBorder(FAppStyle::GetBrush("ToolPanel.GroupBorder"));
        SlateIM::Fill();
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::BeginScrollBox();
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::BeginVerticalStack();

        DrawHeader();
        DrawPanelList();
        if (UKataDebugPanel* Panel = GetActivePanel())
        {
            if (Panel->UsesTargetSelection())
            {
                DrawTargetSelection(Context);
                Context.SelectedActor = SelectedActor.Get();
            }
            SlateIM::Padding(FMargin(4.f, 8.f, 4.f, 2.f));
            SlateIM::HAlign(HAlign_Fill);
            SlateIM::BeginVerticalStack();
            Panel->Draw(Context);
            SlateIM::EndVerticalStack();
        }
        else
        {
            SlateIM::Padding(FMargin(4.f));
            SlateIM::Text(TEXT("No debug panels."));
        }

        SlateIM::EndVerticalStack();
        SlateIM::EndScrollBox();
        SlateIM::EndBorder();
    }
    SlateIM::EndRoot();
}

void UKataDebugMenuSubsystem::DrawHeader()
{
    SlateIM::Padding(FMargin(4.f, 4.f));
    SlateIM::HAlign(HAlign_Fill);
    SlateIM::BeginHorizontalStack();
    SlateIM::VAlign(VAlign_Center);
    SlateIM::Fill();
    SlateIM::Text(TEXT("Kata Debug Menu"), FStyleColors::AccentBlue);

    SlateIM::VAlign(VAlign_Center);
    bool bPause = bPauseWhileOpen;
    if (SlateIM::CheckBox(TEXT("Pause Game"), bPause))
    {
        bPauseWhileOpen = bPause;
        ApplyPause(bPause);
    }

    SlateIM::Padding(FMargin(8.f, 0.f, 0.f, 0.f));
    if (SlateIM::Button(TEXT("Close")))
    {
        SetMenuOpen(false);
    }
    SlateIM::EndHorizontalStack();
}

void UKataDebugMenuSubsystem::DrawPanelList()
{
    if (Panels.Num() <= 1)
    {
        return;
    }

    SlateIM::Padding(FMargin(4.f, 2.f));
    SlateIM::BeginHorizontalWrap();
    for (int32 Index = 0; Index < Panels.Num(); ++Index)
    {
        SlateIM::Padding(FMargin(0.f, 0.f, 4.f, 4.f));
        if (SlateIM::Button(Panels[Index]->GetDisplayName(), { .bEnabled = Index != ActivePanelIndex }))
        {
            if (UKataDebugPanel* Previous = GetActivePanel())
            {
                Previous->OnHidden();
            }
            ActivePanelIndex = Index;
            Panels[Index]->OnShown();
        }
    }
    SlateIM::EndHorizontalWrap();
}

void UKataDebugMenuSubsystem::DrawTargetSelection(const FKataDebugPanelContext& Context)
{
    APlayerController* PlayerController = Context.PlayerController;
    const double Now = FPlatformTime::Seconds();
    bool bForceListRefresh = false;
    if (bRefreshTargetList || Now >= NextTargetRefreshTime)
    {
        const TArray<FString> PreviousNames = TargetCandidateNames;
        RefreshTargetCandidates(*PlayerController);
        NextTargetRefreshTime = Now + KataDebugMenu::TargetRefreshInterval;
        bForceListRefresh = bRefreshTargetList || PreviousNames != TargetCandidateNames;
        bRefreshTargetList = false;
    }

    AActor* Selected = SelectedActor.Get();
    SlateIM::Padding(FMargin(4.f, 8.f, 4.f, 2.f));
    SlateIM::Text(TEXT("Target"), FStyleColors::AccentBlue);
    SlateIM::Padding(FMargin(4.f, 2.f));
    SlateIM::Text(Selected != nullptr ? Selected->GetActorNameOrLabel() : FString(TEXT("(none)")));

    SlateIM::Padding(FMargin(4.f, 2.f));
    SlateIM::BeginHorizontalStack();
    if (SlateIM::Button(TEXT("Self")))
    {
        SelectedActor = PlayerController->GetPawn();
        bRefreshTargetList = true;
    }
    SlateIM::Padding(FMargin(4.f, 0.f, 0.f, 0.f));
    if (SlateIM::Button(TEXT("Lock-On Target")))
    {
        const APawn* PlayerPawn = PlayerController->GetPawn();
        const UKataTargetingComponent* Targeting = PlayerPawn != nullptr ? PlayerPawn->FindComponentByClass<UKataTargetingComponent>() : nullptr;
        SelectedActor = Targeting != nullptr ? Targeting->GetCurrentTarget() : nullptr;
        bRefreshTargetList = true;
    }
    SlateIM::Padding(FMargin(4.f, 0.f, 0.f, 0.f));
    if (SlateIM::Button(TEXT("Clear")))
    {
        SelectedActor.Reset();
        bRefreshTargetList = true;
    }
    SlateIM::EndHorizontalStack();

    SlateIM::Padding(FMargin(4.f, 2.f));
    SlateIM::MinHeight(KataDebugMenu::TargetListHeight);
    SlateIM::MaxHeight(KataDebugMenu::TargetListHeight);
    SlateIM::HAlign(HAlign_Fill);
    if (SlateIM::SelectionList(TargetCandidateNames, TargetListIndex, { .bForceRefresh = bForceListRefresh })
        && TargetCandidates.IsValidIndex(TargetListIndex))
    {
        SelectedActor = TargetCandidates[TargetListIndex];
    }
}

void UKataDebugMenuSubsystem::RefreshTargetCandidates(APlayerController& PlayerController)
{
    FGameplayTagContainer ExcludedTags;
    const FGameplayTag& DeadStatusTag = UKataCombatSettings::Get()->DeadStatusTag;
    if (DeadStatusTag.IsValid())
    {
        ExcludedTags.AddTag(DeadStatusTag);
    }

    TArray<AActor*> Actors = KataFL::ResolveCheatTargets(&PlayerController, EKataCheatTarget::All, ExcludedTags);
    const APawn* PlayerPawn = PlayerController.GetPawn();
    const FVector Origin = PlayerPawn != nullptr ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;
    Actors.Sort([&Origin](const AActor& A, const AActor& B)
    {
        return FVector::DistSquared(Origin, A.GetActorLocation()) < FVector::DistSquared(Origin, B.GetActorLocation());
    });

    TargetCandidates.Reset();
    TargetCandidateNames.Reset();
    TargetListIndex = INDEX_NONE;
    const AActor* Selected = SelectedActor.Get();
    for (AActor* Actor : Actors)
    {
        // 거리는 미터 단위 정수로 보여 목록이 매 갱신마다 바뀌지 않게 한다.
        const int32 Meters = FMath::RoundToInt(FVector::Dist(Origin, Actor->GetActorLocation()) / 100.0);
        if (Actor == Selected)
        {
            TargetListIndex = TargetCandidates.Num();
        }
        TargetCandidates.Add(Actor);
        TargetCandidateNames.Add(FString::Printf(TEXT("%s  (%d m)"), *Actor->GetActorNameOrLabel(), Meters));
    }
}

void UKataDebugMenuSubsystem::ApplyOpenState(bool bOpen)
{
    APlayerController* PlayerController = GetPlayerController();
    if (!IsValid(PlayerController))
    {
        bPausedByMenu = false;
        return;
    }

    if (bOpen)
    {
        bSavedShowMouseCursor = PlayerController->bShowMouseCursor;
        PlayerController->SetShowMouseCursor(true);

        // 메뉴를 쓰는 동안에도 캐릭터 조작은 받도록 Game and UI로 둔다. 커서가 뷰포트 밖으로 나가도 잠그지 않는다.
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PlayerController->SetInputMode(InputMode);

        if (bPauseWhileOpen)
        {
            ApplyPause(true);
        }
    }
    else
    {
        ApplyPause(false);
        // 열기 전 입력 모드는 엔진에서 읽을 수 없으므로 게임 입력 모드로 되돌린다.
        PlayerController->SetInputMode(FInputModeGameOnly());
        PlayerController->SetShowMouseCursor(bSavedShowMouseCursor);
    }
}

void UKataDebugMenuSubsystem::ApplyPause(bool bPause)
{
    APlayerController* PlayerController = GetPlayerController();
    if (!IsValid(PlayerController))
    {
        bPausedByMenu = false;
        return;
    }

    if (bPause)
    {
        if (bMenuOpen && !bPausedByMenu && !PlayerController->IsPaused())
        {
            bPausedByMenu = PlayerController->SetPause(true);
        }
    }
    else if (bPausedByMenu)
    {
        PlayerController->SetPause(false);
        bPausedByMenu = false;
    }
}

APlayerController* UKataDebugMenuSubsystem::GetPlayerController() const
{
    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    return LocalPlayer != nullptr ? LocalPlayer->GetPlayerController(LocalPlayer->GetWorld()) : nullptr;
}

UKataDebugPanel* UKataDebugMenuSubsystem::GetActivePanel() const
{
    return Panels.IsValidIndex(ActivePanelIndex) ? Panels[ActivePanelIndex].Get() : nullptr;
}
