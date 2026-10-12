#include "KataDebugTab.h"

#include "Attributes/KataCombatSettings.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "FunctionLibraries/KataFL_CheatTarget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "HAL/PlatformTime.h"
#include "KataDebugPanel.h"
#include "KataDebugUserSettings.h"
#include "SlateIM.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Targeting/KataTargetingComponent.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"

namespace KataDebugTabPrivate
{
    /** 대상 목록의 고정 높이. 비어 있어도 같은 크기로 보인다. */
    constexpr float TargetListHeight = 160.f;

    /** 대상 목록을 다시 모으는 간격(초). 거리순 정렬이 매 프레임 흔들리지 않게 한다. */
    constexpr double TargetRefreshInterval = 0.5;
}

void KataDebugLayout::BeginSection()
{
    SlateIM::HAlign(HAlign_Fill);
    SlateIM::BeginVerticalStack();
}

void KataDebugLayout::EndSection()
{
    SlateIM::EndVerticalStack();
}

void KataDebugLayout::SectionHeader(const TCHAR* Title)
{
    SlateIM::Padding(FMargin(4.f, 10.f, 4.f, 2.f));
    SlateIM::Text(Title, FStyleColors::AccentBlue);
}

FKataDebugTab::FKataDebugTab()
    : FSlateIMNomadTabBase(TEXT("Kata Debug"), TEXT("Kata.Debug"), TEXT("Open the Kata Debug tab (cheats and runtime spawner)."),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Character"))
{
    PostPIEStartedHandle = FEditorDelegates::PostPIEStarted.AddRaw(this, &FKataDebugTab::HandlePostPIEStarted);
    EndPIEHandle = FEditorDelegates::EndPIE.AddRaw(this, &FKataDebugTab::HandleEndPIE);
}

FKataDebugTab::~FKataDebugTab()
{
    FEditorDelegates::PostPIEStarted.Remove(PostPIEStartedHandle);
    FEditorDelegates::EndPIE.Remove(EndPIEHandle);
    if (GEngine != nullptr && EditorCloseHandle.IsValid())
    {
        GEngine->OnEditorClose().Remove(EditorCloseHandle);
    }
}

void FKataDebugTab::EnableWidget()
{
    EnsurePanels();
    if (!EditorCloseHandle.IsValid() && GEngine != nullptr)
    {
        EditorCloseHandle = GEngine->OnEditorClose().AddRaw(this, &FKataDebugTab::HandleEditorClose);
    }
    const bool bWasEnabled = IsWidgetEnabled();
    FSlateIMNomadTabBase::EnableWidget();
    if (!bWasEnabled)
    {
        if (UKataDebugPanel* Panel = GetActivePanel())
        {
            Panel->OnShown();
        }
    }
}

void FKataDebugTab::DisableWidget()
{
    if (IsWidgetEnabled())
    {
        if (UKataDebugPanel* Panel = GetActivePanel())
        {
            Panel->OnHidden();
        }
    }
    FSlateIMNomadTabBase::DisableWidget();
}

void FKataDebugTab::HandleEditorClose()
{
    if (UKataDebugPanel* Panel = GetActivePanel())
    {
        Panel->OnHidden();
    }
    // 메인 창이 닫히면 안의 탭은 OnTabClosed 없이 사라진다. 그리기가 남아 있으면 기반 클래스가 탭이 없다고 보고
    // 새 창으로 다시 열기 때문에, 탭은 레이아웃에 남겨 두고 그리기 Tick만 멈춘다.
    FSlateIMWidgetBase::DisableWidget();
}

void FKataDebugTab::HandlePostPIEStarted(bool bIsSimulating)
{
    // PlayerController는 PIE 월드의 BeginPlay 전에 만들어지고 CheatManager도 그때 붙으므로 여기서는 준비되어 있다.
    EnsurePanels();
    UWorld* World = GetPlayWorld();
    APlayerController* PlayerController = World != nullptr ? World->GetFirstPlayerController() : nullptr;
    for (UKataDebugPanel* Panel : Panels)
    {
        Panel->OnPlaySessionStarted(PlayerController);
    }
}

void FKataDebugTab::HandleEndPIE(bool bIsSimulating)
{
    for (UKataDebugPanel* Panel : Panels)
    {
        Panel->OnPlaySessionEnded();
    }
    SelectedActors.Reset();
    TargetCandidates.Reset();
    TargetCandidateNames.Reset();
    bRefreshTargetList = true;
}

void FKataDebugTab::TogglePanel(TSubclassOf<UKataDebugPanel> PanelClass)
{
    EnsurePanels();
    const int32 Index = Panels.IndexOfByPredicate([PanelClass](const UKataDebugPanel* Panel)
    {
        return Panel != nullptr && Panel->IsA(PanelClass);
    });
    if (IsWidgetEnabled() && (Index == INDEX_NONE || Index == ActivePanelIndex))
    {
        DisableWidget();
        return;
    }
    if (Index != INDEX_NONE)
    {
        SetActivePanel(Index);
    }
    EnableWidget();
}

void FKataDebugTab::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObjects(Panels);
}

void FKataDebugTab::EnsurePanels()
{
    if (bPanelsCreated)
    {
        return;
    }
    bPanelsCreated = true;

    // 패널은 클래스만 추가하면 목록에 오르도록 파생 클래스를 찾아 만든다.
    TArray<UClass*> PanelClasses;
    GetDerivedClasses(UKataDebugPanel::StaticClass(), PanelClasses, true);
    for (UClass* PanelClass : PanelClasses)
    {
        if (PanelClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
        {
            continue;
        }
        Panels.Add(NewObject<UKataDebugPanel>(GetTransientPackage(), PanelClass, NAME_None, RF_Transient));
    }
    // TArray::Sort는 포인터 원소를 역참조해 비교 함수에 넘긴다.
    Panels.Sort([](const UKataDebugPanel& A, const UKataDebugPanel& B)
    {
        const int32 OrderA = A.GetSortOrder();
        const int32 OrderB = B.GetSortOrder();
        return OrderA != OrderB ? OrderA < OrderB : A.GetDisplayName() < B.GetDisplayName();
    });
    for (const UKataDebugPanel* Panel : Panels)
    {
        PanelNames.Add(Panel->GetDisplayName());
    }

    // 저장된 패널이 없어졌으면 정렬상 첫 패널을 쓴다.
    const FName SavedPanelClass = GetDefault<UKataDebugUserSettings>()->ActivePanelClass;
    const int32 SavedIndex = Panels.IndexOfByPredicate([SavedPanelClass](const UKataDebugPanel* Panel)
    {
        return Panel->GetClass()->GetFName() == SavedPanelClass;
    });
    ActivePanelIndex = SavedIndex != INDEX_NONE ? SavedIndex : 0;
    bRefreshPanelCombo = true;
}

void FKataDebugTab::SetActivePanel(int32 Index)
{
    if (Index == ActivePanelIndex || !Panels.IsValidIndex(Index))
    {
        return;
    }
    const bool bVisible = IsWidgetEnabled();
    if (UKataDebugPanel* Previous = GetActivePanel())
    {
        if (bVisible)
        {
            Previous->OnHidden();
        }
    }
    ActivePanelIndex = Index;
    bRefreshPanelCombo = true;

    UKataDebugUserSettings* UserSettings = GetMutableDefault<UKataDebugUserSettings>();
    UserSettings->ActivePanelClass = Panels[Index]->GetClass()->GetFName();
    UserSettings->SaveConfig();
    if (bVisible)
    {
        Panels[Index]->OnShown();
    }
}

UKataDebugPanel* FKataDebugTab::GetActivePanel() const
{
    return Panels.IsValidIndex(ActivePanelIndex) ? Panels[ActivePanelIndex].Get() : nullptr;
}

void FKataDebugTab::DrawContent(float DeltaTime)
{
    EnsurePanels();

    UWorld* World = GetPlayWorld();
    FKataDebugPanelContext Context;
    Context.World = World;
    Context.PlayerController = World != nullptr ? World->GetFirstPlayerController() : nullptr;
    Context.SelectedActors = GetSelectedActors();

    SlateIM::Fill();
    SlateIM::HAlign(HAlign_Fill);
    SlateIM::VAlign(VAlign_Fill);
    SlateIM::BeginBorder(FAppStyle::GetBrush("ToolPanel.GroupBorder"));
    SlateIM::Fill();
    SlateIM::HAlign(HAlign_Fill);
    SlateIM::BeginScrollBox();
    SlateIM::HAlign(HAlign_Fill);
    SlateIM::BeginVerticalStack();

    DrawPanelList();

    // SlateIM은 슬롯 정렬이 바뀌었는지를 루트 전체의 위젯 일련번호로 비교하지만 슬롯은 컨테이너 안의 순번으로 재사용한다.
    // 대상 목록처럼 앞쪽 위젯 수가 바뀌면 일련번호가 밀려, 같은 순번의 새 위젯이 이전 프레임의 다른 위젯과 정렬값이 같을 때
    // 슬롯 정렬 갱신을 건너뛰고 이전 위젯의 정렬이 남는다. 패널이나 PIE 상태가 바뀌어 내용이 통째로 달라지는 프레임에는
    // 아래 내용을 감싸는 컨테이너 종류를 바꿔 모든 위젯과 슬롯을 새로 만든다.
    const uint32 ContentLayoutKey = HashCombine(GetTypeHash(ActivePanelIndex), GetTypeHash(Context.PlayerController != nullptr));
    if (ContentLayoutKey != LastContentLayoutKey)
    {
        LastContentLayoutKey = ContentLayoutKey;
        bAlternateContentContainer = !bAlternateContentContainer;
    }
    SlateIM::HAlign(HAlign_Fill);
    if (bAlternateContentContainer)
    {
        SlateIM::BeginBorder(FAppStyle::GetNoBrush(), Orient_Vertical, false, FMargin(0.f));
    }
    else
    {
        SlateIM::BeginVerticalStack();
    }

    if (UKataDebugPanel* Panel = GetActivePanel())
    {
        // 대상 선택을 쓰지 않는 패널이나 PIE가 아닐 때도 빈 묶음을 그려 패널 내용의 위젯 순서를 고정한다.
        KataDebugLayout::BeginSection();
        if (Panel->UsesTargetSelection() && Context.PlayerController != nullptr)
        {
            DrawTargetSelection(Context);
            Context.SelectedActors = GetSelectedActors();
        }
        KataDebugLayout::EndSection();

        KataDebugLayout::BeginSection();
        Panel->Draw(Context);
        KataDebugLayout::EndSection();
    }
    else
    {
        SlateIM::Padding(FMargin(4.f));
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::Text(TEXT("No debug panels."));
    }

    if (bAlternateContentContainer)
    {
        SlateIM::EndBorder();
    }
    else
    {
        SlateIM::EndVerticalStack();
    }

    SlateIM::EndVerticalStack();
    SlateIM::EndScrollBox();
    SlateIM::EndBorder();
}

void FKataDebugTab::DrawPanelList()
{
    // 패널은 드롭다운으로 고른다. 패널이 늘어도 한 줄을 넘지 않는다.
    SlateIM::Padding(FMargin(4.f, 4.f, 4.f, 2.f));
    SlateIM::HAlign(HAlign_Fill);
    SlateIM::BeginHorizontalStack();
    SlateIM::VAlign(VAlign_Center);
    SlateIM::AutoSize();
    SlateIM::Text(TEXT("Panel"));
    SlateIM::Padding(FMargin(8.f, 0.f, 0.f, 0.f));
    SlateIM::Fill();
    SlateIM::HAlign(HAlign_Fill);
    SlateIM::VAlign(VAlign_Center);
    int32 SelectedIndex = ActivePanelIndex;
    const bool bForceRefresh = bRefreshPanelCombo;
    bRefreshPanelCombo = false;
    if (SlateIM::ComboBox(PanelNames, SelectedIndex, { .bForceRefresh = bForceRefresh }) && SelectedIndex != ActivePanelIndex)
    {
        SetActivePanel(SelectedIndex);
    }
    SlateIM::EndHorizontalStack();
}

void FKataDebugTab::DrawTargetSelection(const FKataDebugPanelContext& Context)
{
    APlayerController* PlayerController = Context.PlayerController;
    const double Now = FPlatformTime::Seconds();
    if (bRefreshTargetList || Now >= NextTargetRefreshTime)
    {
        RefreshTargetCandidates(*PlayerController);
        NextTargetRefreshTime = Now + KataDebugTabPrivate::TargetRefreshInterval;
        bRefreshTargetList = false;
    }

    const TArray<AActor*> Selected = GetSelectedActors();
    KataDebugLayout::SectionHeader(TEXT("Target"));
    SlateIM::Padding(FMargin(4.f, 2.f));
    if (Selected.IsEmpty())
    {
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::Text(TEXT("(none)"));
    }
    else
    {
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::Text(Selected.Num() == 1 ? Selected[0]->GetActorNameOrLabel()
            : FString::Printf(TEXT("%d selected (%s, ...)"), Selected.Num(), *Selected[0]->GetActorNameOrLabel()));
    }

    SlateIM::Padding(FMargin(4.f, 2.f));
    SlateIM::BeginHorizontalStack();
    if (SlateIM::Button(TEXT("Self")))
    {
        SelectOnly(PlayerController->GetPawn());
    }
    SlateIM::Padding(FMargin(4.f, 0.f, 0.f, 0.f));
    if (SlateIM::Button(TEXT("Lock-On Target")))
    {
        const APawn* PlayerPawn = PlayerController->GetPawn();
        const UKataTargetingComponent* Targeting = PlayerPawn != nullptr ? PlayerPawn->FindComponentByClass<UKataTargetingComponent>() : nullptr;
        SelectOnly(Targeting != nullptr ? Targeting->GetCurrentTarget() : nullptr);
    }
    SlateIM::Padding(FMargin(4.f, 0.f, 0.f, 0.f));
    if (SlateIM::Button(TEXT("All"), { .bEnabled = !TargetCandidates.IsEmpty() }))
    {
        for (const TWeakObjectPtr<AActor>& Candidate : TargetCandidates)
        {
            SetSelected(Candidate.Get(), true);
        }
    }
    SlateIM::Padding(FMargin(4.f, 0.f, 0.f, 0.f));
    if (SlateIM::Button(TEXT("Clear")))
    {
        SelectOnly(nullptr);
    }
    SlateIM::EndHorizontalStack();

    // 목록은 대상 수와 관계없이 항상 같은 높이로 두어 아래 패널이 들썩이지 않게 한다. 넘치면 목록 안에서 스크롤한다.
    // 빈 리스트는 배경을 그리지 않아 칸이 보이지 않으므로, 어두운 테두리로 감싸 칸을 항상 보이게 한다.
    SlateIM::Padding(FMargin(4.f, 2.f));
    SlateIM::MinHeight(KataDebugTabPrivate::TargetListHeight);
    SlateIM::MaxHeight(KataDebugTabPrivate::TargetListHeight);
    SlateIM::HAlign(HAlign_Fill);
    SlateIM::VAlign(VAlign_Fill);
    SlateIM::BeginBorder(FAppStyle::GetBrush("Brushes.Recessed"));
    // 테두리 안의 유일한 위젯이라 문구와 목록이 바뀌어도 밖의 위젯 순서는 그대로다.
    if (TargetCandidateNames.IsEmpty())
    {
        SlateIM::Padding(FMargin(4.f));
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::Text(TEXT("No other pawns with an ability system."), FStyleColors::Foreground);
    }
    else
    {
        // 여러 대상을 고르도록 줄마다 체크박스를 둔다. 체크 상태는 매 프레임 선택 목록에서 다시 읽으므로
        // 거리순 정렬로 줄 순서가 바뀌어도 선택은 액터를 따라간다.
        SlateIM::Fill();
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::VAlign(VAlign_Fill);
        SlateIM::BeginScrollBox();
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::BeginVerticalStack();
        for (int32 Index = 0; Index < TargetCandidates.Num(); ++Index)
        {
            AActor* Candidate = TargetCandidates[Index].Get();
            bool bChecked = IsSelected(Candidate);
            SlateIM::Padding(FMargin(4.f, 1.f));
            if (SlateIM::CheckBox(TargetCandidateNames[Index], bChecked) && Candidate != nullptr)
            {
                SetSelected(Candidate, bChecked);
            }
        }
        SlateIM::EndVerticalStack();
        SlateIM::EndScrollBox();
    }
    SlateIM::EndBorder();
}

void FKataDebugTab::RefreshTargetCandidates(APlayerController& PlayerController)
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
    for (AActor* Actor : Actors)
    {
        // 거리는 미터 단위 정수로 보여 목록이 매 갱신마다 바뀌지 않게 한다.
        const int32 Meters = FMath::RoundToInt(FVector::Dist(Origin, Actor->GetActorLocation()) / 100.0);
        TargetCandidates.Add(Actor);
        TargetCandidateNames.Add(FString::Printf(TEXT("%s  (%d m)"), *Actor->GetActorNameOrLabel(), Meters));
    }
}

TArray<AActor*> FKataDebugTab::GetSelectedActors()
{
    SelectedActors.RemoveAll([](const TWeakObjectPtr<AActor>& Actor) { return !Actor.IsValid(); });
    TArray<AActor*> Result;
    Result.Reserve(SelectedActors.Num());
    for (const TWeakObjectPtr<AActor>& Actor : SelectedActors)
    {
        Result.Add(Actor.Get());
    }
    return Result;
}

bool FKataDebugTab::IsSelected(const AActor* Actor) const
{
    return Actor != nullptr && SelectedActors.ContainsByPredicate([Actor](const TWeakObjectPtr<AActor>& Entry) { return Entry.Get() == Actor; });
}

void FKataDebugTab::SetSelected(AActor* Actor, bool bSelected)
{
    if (Actor == nullptr)
    {
        return;
    }
    if (!bSelected)
    {
        SelectedActors.RemoveAll([Actor](const TWeakObjectPtr<AActor>& Entry) { return Entry.Get() == Actor; });
    }
    else if (!IsSelected(Actor))
    {
        SelectedActors.Add(Actor);
    }
}

void FKataDebugTab::SelectOnly(AActor* Actor)
{
    SelectedActors.Reset();
    SetSelected(Actor, true);
}

UWorld* FKataDebugTab::GetPlayWorld()
{
    return GEditor != nullptr ? GEditor->PlayWorld.Get() : nullptr;
}
