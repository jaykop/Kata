#include "KataRuntimeSpawnerTab.h"

#include "Action/KataAction.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Character/KataCharacter.h"
#include "Character/KataCharacterRow.h"
#include "Character/KataCharacterSpawnSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Data/KataRowId.h"
#include "Editor.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagsManager.h"
#include "ISettingsModule.h"
#include "KataGraph.h"
#include "KataRuntimeSpawnLayout.h"
#include "KataRuntimeSpawnerLog.h"
#include "KataRuntimeSpawnerSettings.h"
#include "KataRuntimeSpawnerUserSettings.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "SlateIM.h"
#include "StructUtils/InstancedStruct.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "UObject/StrongObjectPtr.h"

/** 생성 요청 하나가 완료 콜백까지 들고 가는 반복 설정이다. 로드한 에셋이 콜백 전에 GC되지 않게 강한 참조로 잡는다. */
struct FKataRuntimeSpawnerTab::FRepeatRequest
{
    EKataRuntimeSpawnAIMode Mode = EKataRuntimeSpawnAIMode::Default;
    TStrongObjectPtr<UKataAction> Action;
    TStrongObjectPtr<UKataGraph> Graph;
    FGameplayTag EntryTrigger;
    float Interval = 0.f;
};

namespace KataRuntimeSpawnerTabPrivate
{
    const TArray<FString> PatternNames = { TEXT("Line"), TEXT("Circle") };
    const TArray<FString> AIModeNames = { TEXT("Default StateTree"), TEXT("Idle"), TEXT("Repeat Action"), TEXT("Repeat Graph") };

    /** 라벨 칸 폭. 모든 줄의 값 칸이 같은 위치에서 시작하도록 고정한다. */
    constexpr float LabelWidth = 140.f;

    /**
     * 라벨과 입력 위젯을 한 줄에 놓는다. bFillNext면 다음 위젯이 남은 폭을 가로로 채운다.
     * 한 줄에 위젯을 더 놓을 때는 각 위젯 앞에서 CenterNext를 불러 세로 위치를 맞춘다.
     */
    void BeginRow(const TCHAR* Label, bool bFillNext = true)
    {
        SlateIM::Padding(FMargin(4.f, 2.f));
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::BeginHorizontalStack();
        SlateIM::VAlign(VAlign_Center);
        SlateIM::MinWidth(LabelWidth);
        SlateIM::MaxWidth(LabelWidth);
        SlateIM::Text(Label);
        if (bFillNext)
        {
            SlateIM::Fill();
            SlateIM::HAlign(HAlign_Fill);
        }
        SlateIM::VAlign(VAlign_Center);
    }

    /** 같은 줄의 다음 위젯을 세로 가운데에 놓는다. bFill이면 남은 폭을 채운다. */
    void CenterNext(bool bFill = false)
    {
        if (bFill)
        {
            SlateIM::Fill();
            SlateIM::HAlign(HAlign_Fill);
        }
        else
        {
            SlateIM::AutoSize();
        }
        SlateIM::VAlign(VAlign_Center);
    }

    void EndRow()
    {
        SlateIM::EndHorizontalStack();
    }

    void SectionHeader(const TCHAR* Title)
    {
        SlateIM::Padding(FMargin(4.f, 10.f, 4.f, 2.f));
        SlateIM::Text(Title, { .Color = FSlateColor(EStyleColor::AccentBlue) });
    }

    /** 경로 목록에서 저장된 경로를 찾고, 없으면 목록이 비지 않았을 때 첫 항목을 고른다. */
    int32 FindIndexOrFirst(const TArray<FSoftObjectPath>& Paths, const FSoftObjectPath& Saved)
    {
        const int32 Index = Paths.IndexOfByKey(Saved);
        return Index != INDEX_NONE ? Index : (Paths.IsEmpty() ? INDEX_NONE : 0);
    }

    /** AssetFolder가 Folder 자신이거나 그 하위 폴더이면 true다. */
    bool IsInFolder(const FString& AssetFolder, const FString& Folder)
    {
        return AssetFolder == Folder || AssetFolder.StartsWith(Folder + TEXT("/"));
    }

    void CollectAssets(UClass* Class, TArray<FSoftObjectPath>& OutPaths, TArray<FString>& OutNames, TArray<FString>& OutFolders)
    {
        OutPaths.Reset();
        OutNames.Reset();
        OutFolders.Reset();
        TArray<FAssetData> Assets;
        IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
        Registry.GetAssetsByClass(Class->GetClassPathName(), Assets, true);
        Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.AssetName.LexicalLess(B.AssetName); });
        for (const FAssetData& Asset : Assets)
        {
            OutPaths.Add(Asset.GetSoftObjectPath());
            OutNames.Add(Asset.AssetName.ToString());
            OutFolders.Add(Asset.PackagePath.ToString());
        }
    }

    /** 비우면 최상위, 아니면 폴더 경로를 묶음 이름으로 쓴다. 폴더 필터가 켜져 있으면 캐릭터 폴더 기준 상대 경로다. */
    FString MakeFolderCategory(const FString& AssetFolder, const FString& BaseFolder)
    {
        FString Category = AssetFolder;
        if (!BaseFolder.IsEmpty() && Category.StartsWith(BaseFolder))
        {
            Category.RightChopInline(BaseFolder.Len());
        }
        else
        {
            Category.RemoveFromStart(TEXT("/Game"));
        }
        Category.RemoveFromStart(TEXT("/"));
        return Category;
    }

    /**
     * 폴더 필터를 통과한 항목으로 선택 메뉴를 채운다. 현재 선택이 걸러지면 첫 항목으로, 남은 항목이 없으면 INDEX_NONE으로 바꾼다.
     * 선택이 바뀌면 true다.
     */
    bool FillAssetPicker(FKataRuntimeSpawnerPicker& Picker, const TArray<FSoftObjectPath>& Paths, const TArray<FString>& Folders,
        const FString& Folder, int32& InOutIndex, int32& OutVisibleCount)
    {
        TArray<FKataRuntimeSpawnerPickerItem> Items;
        for (int32 Index = 0; Index < Paths.Num(); ++Index)
        {
            if (!Folder.IsEmpty() && !IsInFolder(Folders[Index], Folder))
            {
                continue;
            }
            FKataRuntimeSpawnerPickerItem& Item = Items.AddDefaulted_GetRef();
            Item.Label = Paths[Index].GetAssetName();
            Item.Category = MakeFolderCategory(Folders[Index], Folder);
            Item.ToolTip = Paths[Index].ToString();
            Item.SourceIndex = Index;
        }
        const int32 Previous = InOutIndex;
        if (!Items.ContainsByPredicate([InOutIndex](const FKataRuntimeSpawnerPickerItem& Item) { return Item.SourceIndex == InOutIndex; }))
        {
            InOutIndex = Items.IsEmpty() ? INDEX_NONE : Items[0].SourceIndex;
        }
        OutVisibleCount = Items.Num();
        Picker.SetItems(MoveTemp(Items));
        return Previous != InOutIndex;
    }
}

FKataRuntimeSpawnerTab::FKataRuntimeSpawnerTab()
    : FSlateIMNomadTabBase(TEXT("Kata Runtime Spawner"), TEXT("Kata.RuntimeSpawner"), TEXT("Open the Kata Runtime Spawner tool."),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Character"))
    , RowPicker(MakeShared<FKataRuntimeSpawnerPicker>())
    , ActionPicker(MakeShared<FKataRuntimeSpawnerPicker>())
    , GraphPicker(MakeShared<FKataRuntimeSpawnerPicker>())
{
    EndPIEHandle = FEditorDelegates::EndPIE.AddRaw(this, &FKataRuntimeSpawnerTab::HandleEndPIE);
}

FKataRuntimeSpawnerTab::~FKataRuntimeSpawnerTab()
{
    FEditorDelegates::EndPIE.Remove(EndPIEHandle);
    if (GEngine != nullptr && EditorCloseHandle.IsValid())
    {
        GEngine->OnEditorClose().Remove(EditorCloseHandle);
    }
    SaveUserSettings();
}

void FKataRuntimeSpawnerTab::EnableWidget()
{
    if (!EditorCloseHandle.IsValid() && GEngine != nullptr)
    {
        EditorCloseHandle = GEngine->OnEditorClose().AddRaw(this, &FKataRuntimeSpawnerTab::HandleEditorClose);
    }
    FSlateIMNomadTabBase::EnableWidget();
}

void FKataRuntimeSpawnerTab::HandleEditorClose()
{
    SaveUserSettings();
    // 메인 창이 닫히면 안의 탭은 OnTabClosed 없이 사라진다. 그리기가 남아 있으면 기반 클래스가 탭이 없다고 보고
    // 새 창으로 다시 열기 때문에, 탭은 레이아웃에 남겨 두고 그리기 Tick만 멈춘다.
    FSlateIMWidgetBase::DisableWidget();
}

void FKataRuntimeSpawnerTab::DisableWidget()
{
    SaveUserSettings();
    FSlateIMNomadTabBase::DisableWidget();
}

void FKataRuntimeSpawnerTab::DrawContent(float DeltaTime)
{
    RefreshTables(false);

    SlateIM::Fill();
    SlateIM::HAlign(HAlign_Fill);
    SlateIM::VAlign(VAlign_Fill);
    SlateIM::BeginBorder(FAppStyle::GetBrush("ToolPanel.GroupBorder"));
    SlateIM::Fill();
    SlateIM::HAlign(HAlign_Fill);
    SlateIM::BeginScrollBox();
    SlateIM::HAlign(HAlign_Fill);
    SlateIM::BeginVerticalStack();
    DrawCharacterSection();
    DrawPlacementSection();
    DrawAISection();
    DrawActionsSection();
    SlateIM::EndVerticalStack();
    SlateIM::EndScrollBox();
    SlateIM::EndBorder();

    bRefreshTableCombo = false;
    bRefreshTriggerCombo = false;
}

int32 FKataRuntimeSpawnerTab::DrawPicker(const TCHAR* Label, FKataRuntimeSpawnerPicker& Picker, const FString& CurrentLabel)
{
    using namespace KataRuntimeSpawnerTabPrivate;
    BeginRow(Label);
    Picker.SetCurrentLabel(CurrentLabel);
    SlateIM::Widget(Picker.GetWidget());
    EndRow();
    return Picker.ConsumeSelection();
}

void FKataRuntimeSpawnerTab::DrawCharacterSection()
{
    using namespace KataRuntimeSpawnerTabPrivate;
    UKataRuntimeSpawnerUserSettings* User = GetMutableDefault<UKataRuntimeSpawnerUserSettings>();

    SectionHeader(TEXT("Character"));
    BeginRow(TEXT("NPC Table"));
    if (SlateIM::ComboBox(TableNames, TableIndex, { .bForceRefresh = bRefreshTableCombo }) && TablePaths.IsValidIndex(TableIndex))
    {
        User->Table = TablePaths[TableIndex];
        User->RowName = NAME_None;
        bUserSettingsDirty = true;
        RefreshRows();
    }
    SlateIM::Padding(FMargin(6.f, 0.f, 0.f, 0.f));
    CenterNext();
    if (SlateIM::Button(TEXT("Settings")))
    {
        const UKataRuntimeSpawnerSettings* Settings = GetDefault<UKataRuntimeSpawnerSettings>();
        FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings"))
            .ShowViewer(Settings->GetContainerName(), Settings->GetCategoryName(), Settings->GetSectionName());
    }
    EndRow();
    if (TableNames.IsEmpty())
    {
        SlateIM::Padding(FMargin(4.f, 2.f));
        SlateIM::Text(TEXT("Add NPC tables in Project Settings > Editor > Kata Runtime Spawner."));
    }

    const int32 PickedRow = DrawPicker(TEXT("Row"), *RowPicker, RowNames.IsValidIndex(RowIndex) ? RowNames[RowIndex] : FString());
    if (RowNames.IsValidIndex(PickedRow) && PickedRow != RowIndex)
    {
        RowIndex = PickedRow;
        User->RowName = FName(*RowNames[RowIndex]);
        bUserSettingsDirty = true;
        UpdateCharacterFolder();
    }
}

void FKataRuntimeSpawnerTab::DrawPlacementSection()
{
    using namespace KataRuntimeSpawnerTabPrivate;
    UKataRuntimeSpawnerUserSettings* User = GetMutableDefault<UKataRuntimeSpawnerUserSettings>();

    SectionHeader(TEXT("Placement"));
    BeginRow(TEXT("Distance (cm)"));
    bUserSettingsDirty |= SlateIM::SpinBox(User->Distance, { .Min = 0.f });
    EndRow();

    BeginRow(TEXT("Yaw Offset (deg)"));
    bUserSettingsDirty |= SlateIM::SpinBox(User->YawOffset, { .Min = -180.f, .Max = 180.f });
    EndRow();

    BeginRow(TEXT("Count"));
    bUserSettingsDirty |= SlateIM::SpinBox(User->Count, { .Min = 1, .Max = 50 });
    EndRow();

    BeginRow(TEXT("Pattern"));
    int32 PatternIndex = static_cast<int32>(User->Pattern);
    if (SlateIM::ComboBox(PatternNames, PatternIndex) && PatternNames.IsValidIndex(PatternIndex))
    {
        User->Pattern = static_cast<EKataRuntimeSpawnPattern>(PatternIndex);
        bUserSettingsDirty = true;
    }
    EndRow();

    BeginRow(User->Pattern == EKataRuntimeSpawnPattern::Circle ? TEXT("Spacing (arc, cm)") : TEXT("Spacing (cm)"));
    bUserSettingsDirty |= SlateIM::SpinBox(User->Spacing, { .Min = 0.f });
    EndRow();
}

void FKataRuntimeSpawnerTab::DrawAISection()
{
    using namespace KataRuntimeSpawnerTabPrivate;
    UKataRuntimeSpawnerUserSettings* User = GetMutableDefault<UKataRuntimeSpawnerUserSettings>();

    SectionHeader(TEXT("AI"));
    BeginRow(TEXT("Mode"));
    int32 ModeIndex = static_cast<int32>(User->AIMode);
    if (SlateIM::ComboBox(AIModeNames, ModeIndex) && AIModeNames.IsValidIndex(ModeIndex))
    {
        User->AIMode = static_cast<EKataRuntimeSpawnAIMode>(ModeIndex);
        bUserSettingsDirty = true;
    }
    EndRow();

    const bool bRepeatAction = User->AIMode == EKataRuntimeSpawnAIMode::RepeatAction;
    const bool bRepeatGraph = User->AIMode == EKataRuntimeSpawnAIMode::RepeatGraph;
    if (!bRepeatAction && !bRepeatGraph)
    {
        return;
    }
    if (!bCatalogBuilt)
    {
        RefreshCatalog();
    }

    BeginRow(TEXT("Asset Filter"), false);
    if (SlateIM::CheckBox(User->bFilterByCharacterFolder, { .Label = TEXT("Character folder only") }))
    {
        bUserSettingsDirty = true;
        ApplyAssetFilters();
    }
    SlateIM::Padding(FMargin(8.f, 0.f, 0.f, 0.f));
    CenterNext(true);
    SlateIM::Text(CharacterFolder.IsEmpty() ? FString(TEXT("(row has no Character Class)")) : CharacterFolder);
    SlateIM::Padding(FMargin(6.f, 0.f, 0.f, 0.f));
    CenterNext();
    if (SlateIM::Button(TEXT("Refresh")))
    {
        RefreshCatalog();
    }
    EndRow();

    if (bRepeatAction)
    {
        const int32 Picked = DrawPicker(TEXT("Action"), *ActionPicker,
            ActionPaths.IsValidIndex(ActionIndex) ? ActionPaths[ActionIndex].GetAssetName() : FString());
        if (ActionPaths.IsValidIndex(Picked))
        {
            ActionIndex = Picked;
            User->ActionAsset = ActionPaths[ActionIndex];
            bUserSettingsDirty = true;
        }
    }
    else
    {
        const int32 Picked = DrawPicker(TEXT("Graph"), *GraphPicker,
            GraphPaths.IsValidIndex(GraphIndex) ? GraphPaths[GraphIndex].GetAssetName() : FString());
        if (GraphPaths.IsValidIndex(Picked))
        {
            GraphIndex = Picked;
            User->GraphAsset = GraphPaths[GraphIndex];
            bUserSettingsDirty = true;
        }
    }
    if ((bRepeatAction ? VisibleActionCount : VisibleGraphCount) == 0)
    {
        SlateIM::Padding(FMargin(4.f, 2.f));
        SlateIM::Text(User->bFilterByCharacterFolder && !CharacterFolder.IsEmpty()
            ? TEXT("No assets in the character folder. Turn off \"Character folder only\" to see all assets.")
            : TEXT("No assets found. Press Refresh after the asset registry finishes loading."));
    }

    if (bRepeatGraph)
    {
        BeginRow(TEXT("Entry Trigger"));
        if (SlateIM::ComboBox(TriggerNames, TriggerIndex, { .bForceRefresh = bRefreshTriggerCombo }) && TriggerTags.IsValidIndex(TriggerIndex))
        {
            User->EntryTrigger = TriggerTags[TriggerIndex];
            bUserSettingsDirty = true;
        }
        EndRow();
    }

    BeginRow(TEXT("Repeat Interval (s)"));
    bUserSettingsDirty |= SlateIM::SpinBox(User->RepeatInterval, { .Min = 0.f });
    EndRow();
}

void FKataRuntimeSpawnerTab::DrawActionsSection()
{
    const UKataRuntimeSpawnerUserSettings* User = GetDefault<UKataRuntimeSpawnerUserSettings>();
    SpawnedCharacters.RemoveAllSwap([](const TWeakObjectPtr<AKataCharacter>& Character) { return !Character.IsValid(); });

    const UWorld* World = GetPlayWorld();
    const APlayerController* PlayerController = World != nullptr ? World->GetFirstPlayerController() : nullptr;
    const bool bHasPlayer = PlayerController != nullptr && PlayerController->GetPawn() != nullptr;
    bool bHasAsset = true;
    if (User->AIMode == EKataRuntimeSpawnAIMode::RepeatAction)
    {
        bHasAsset = ActionPaths.IsValidIndex(ActionIndex);
    }
    else if (User->AIMode == EKataRuntimeSpawnAIMode::RepeatGraph)
    {
        bHasAsset = GraphPaths.IsValidIndex(GraphIndex);
    }
    const bool bCanSpawn = bHasPlayer && TablePaths.IsValidIndex(TableIndex) && RowNames.IsValidIndex(RowIndex) && bHasAsset;

    SlateIM::Padding(FMargin(4.f, 12.f, 4.f, 2.f));
    SlateIM::BeginHorizontalStack();
    if (SlateIM::Button(TEXT("Spawn"), { .bEnabled = bCanSpawn }))
    {
        SpawnCharacters();
    }
    SlateIM::Padding(FMargin(8.f, 0.f, 0.f, 0.f));
    if (SlateIM::Button(FString::Printf(TEXT("Despawn All (%d)"), SpawnedCharacters.Num()), { .bEnabled = !SpawnedCharacters.IsEmpty() }))
    {
        DespawnCharacters();
    }
    SlateIM::EndHorizontalStack();

    SlateIM::Padding(FMargin(4.f, 2.f));
    if (World == nullptr)
    {
        SlateIM::Text(TEXT("Start PIE to spawn."));
    }
    else if (!bHasPlayer)
    {
        SlateIM::Text(TEXT("Waiting for the player pawn."));
    }
    else
    {
        SlateIM::Text(FString::Printf(TEXT("Alive %d  Pending %d  Repeating %d  %s"),
            SpawnedCharacters.Num(), PendingCount, Loop.Num(), *StatusText));
    }
}

void FKataRuntimeSpawnerTab::RefreshTables(bool bForce)
{
    const UKataRuntimeSpawnerSettings* Settings = GetDefault<UKataRuntimeSpawnerSettings>();
    uint32 Hash = GetTypeHash(Settings->NPCTables.Num());
    for (const TSoftObjectPtr<UDataTable>& Table : Settings->NPCTables)
    {
        Hash = HashCombine(Hash, GetTypeHash(Table.ToSoftObjectPath()));
    }
    if (!bForce && bTablesBuilt && Hash == TableListHash)
    {
        return;
    }
    bTablesBuilt = true;
    TableListHash = Hash;

    TablePaths.Reset();
    TableNames.Reset();
    for (const TSoftObjectPtr<UDataTable>& TableRef : Settings->NPCTables)
    {
        if (TableRef.IsNull())
        {
            continue;
        }
        const UDataTable* Table = TableRef.LoadSynchronous();
        const UScriptStruct* RowStruct = Table != nullptr ? Table->GetRowStruct() : nullptr;
        if (RowStruct == nullptr || !RowStruct->IsChildOf(FKataNPCCharacterRow::StaticStruct()))
        {
            UE_LOG(LogKataRuntimeSpawner, Warning, TEXT("Runtime Spawner skipped '%s': not an NPC character table."),
                *TableRef.ToString());
            continue;
        }
        TablePaths.Add(TableRef.ToSoftObjectPath());
        TableNames.Add(Table->GetName());
    }
    TableIndex = KataRuntimeSpawnerTabPrivate::FindIndexOrFirst(TablePaths, GetDefault<UKataRuntimeSpawnerUserSettings>()->Table);
    bRefreshTableCombo = true;
    RefreshRows();
}

void FKataRuntimeSpawnerTab::RefreshRows()
{
    RowNames.Reset();
    RowIndex = INDEX_NONE;
    const UDataTable* Table = TablePaths.IsValidIndex(TableIndex) ? Cast<UDataTable>(TablePaths[TableIndex].TryLoad()) : nullptr;
    if (Table != nullptr)
    {
        for (const FName& RowName : Table->GetRowNames())
        {
            RowNames.Add(RowName.ToString());
        }
        RowIndex = RowNames.IndexOfByKey(GetDefault<UKataRuntimeSpawnerUserSettings>()->RowName.ToString());
    }
    ApplyRowFilter();
}

void FKataRuntimeSpawnerTab::ApplyRowFilter()
{
    TArray<FKataRuntimeSpawnerPickerItem> Items;
    Items.Reserve(RowNames.Num());
    for (int32 Index = 0; Index < RowNames.Num(); ++Index)
    {
        FKataRuntimeSpawnerPickerItem& Item = Items.AddDefaulted_GetRef();
        Item.Label = RowNames[Index];
        Item.SourceIndex = Index;
    }
    RowPicker->SetItems(MoveTemp(Items));
    // 저장된 행이 테이블에 없으면 첫 행을 고른다.
    if (!RowNames.IsValidIndex(RowIndex) && !RowNames.IsEmpty())
    {
        RowIndex = 0;
        GetMutableDefault<UKataRuntimeSpawnerUserSettings>()->RowName = FName(*RowNames[RowIndex]);
        bUserSettingsDirty = true;
    }
    UpdateCharacterFolder();
}

void FKataRuntimeSpawnerTab::UpdateCharacterFolder()
{
    CharacterFolder.Reset();
    const UDataTable* Table = TablePaths.IsValidIndex(TableIndex) ? Cast<UDataTable>(TablePaths[TableIndex].TryLoad()) : nullptr;
    const FKataNPCCharacterRow* Row = Table != nullptr && RowNames.IsValidIndex(RowIndex)
        ? Table->FindRow<FKataNPCCharacterRow>(FName(*RowNames[RowIndex]), TEXT("KataRuntimeSpawner"), false)
        : nullptr;
    // 경로만 읽으므로 Character Class를 로드하지 않는다.
    if (Row != nullptr && !Row->CharacterClass.IsNull())
    {
        CharacterFolder = FPackageName::GetLongPackagePath(Row->CharacterClass.ToSoftObjectPath().GetLongPackageName());
    }
    ApplyAssetFilters();
}

void FKataRuntimeSpawnerTab::ApplyAssetFilters()
{
    using namespace KataRuntimeSpawnerTabPrivate;
    if (!bCatalogBuilt)
    {
        return;
    }
    UKataRuntimeSpawnerUserSettings* User = GetMutableDefault<UKataRuntimeSpawnerUserSettings>();
    const FString Folder = User->bFilterByCharacterFolder ? CharacterFolder : FString();

    if (FillAssetPicker(*ActionPicker, ActionPaths, ActionFolders, Folder, ActionIndex, VisibleActionCount)
        && ActionPaths.IsValidIndex(ActionIndex))
    {
        User->ActionAsset = ActionPaths[ActionIndex];
        bUserSettingsDirty = true;
    }
    if (FillAssetPicker(*GraphPicker, GraphPaths, GraphFolders, Folder, GraphIndex, VisibleGraphCount)
        && GraphPaths.IsValidIndex(GraphIndex))
    {
        User->GraphAsset = GraphPaths[GraphIndex];
        bUserSettingsDirty = true;
    }
}

void FKataRuntimeSpawnerTab::RefreshCatalog()
{
    using namespace KataRuntimeSpawnerTabPrivate;
    const UKataRuntimeSpawnerUserSettings* User = GetDefault<UKataRuntimeSpawnerUserSettings>();

    CollectAssets(UKataAction::StaticClass(), ActionPaths, ActionNames, ActionFolders);
    CollectAssets(UKataGraph::StaticClass(), GraphPaths, GraphNames, GraphFolders);
    ActionIndex = ActionPaths.IndexOfByKey(User->ActionAsset);
    GraphIndex = GraphPaths.IndexOfByKey(User->GraphAsset);

    // 첫 항목은 트리거를 보내지 않는 선택이다. 프로젝트의 Trigger 루트 아래 태그만 보인다.
    TriggerTags = { NAME_None };
    TriggerNames = { TEXT("(None)") };
    const FGameplayTag TriggerRoot = FGameplayTag::RequestGameplayTag(TEXT("Trigger"), false);
    if (TriggerRoot.IsValid())
    {
        const FGameplayTagContainer Children = UGameplayTagsManager::Get().RequestGameplayTagChildren(TriggerRoot);
        for (const FGameplayTag& Tag : Children)
        {
            TriggerTags.Add(Tag.GetTagName());
            TriggerNames.Add(Tag.ToString());
        }
    }
    TriggerIndex = FMath::Max(TriggerTags.IndexOfByKey(User->EntryTrigger), 0);
    bRefreshTriggerCombo = true;

    bCatalogBuilt = true;
    ApplyAssetFilters();
}

void FKataRuntimeSpawnerTab::SpawnCharacters()
{
    SaveUserSettings();
    const UKataRuntimeSpawnerUserSettings* User = GetDefault<UKataRuntimeSpawnerUserSettings>();
    UWorld* World = GetPlayWorld();
    APlayerController* PlayerController = World != nullptr ? World->GetFirstPlayerController() : nullptr;
    APawn* Player = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
    UKataCharacterSpawnSubsystem* Subsystem = World != nullptr ? World->GetSubsystem<UKataCharacterSpawnSubsystem>() : nullptr;
    const UDataTable* Table = TablePaths.IsValidIndex(TableIndex) ? Cast<UDataTable>(TablePaths[TableIndex].TryLoad()) : nullptr;
    if (Player == nullptr || Subsystem == nullptr || Table == nullptr || !RowNames.IsValidIndex(RowIndex))
    {
        StatusText = TEXT("Spawn failed: missing player, table or row.");
        return;
    }
    const FName RowName(*RowNames[RowIndex]);
    const uint8* RowMemory = Table->FindRowUnchecked(RowName);
    if (RowMemory == nullptr)
    {
        StatusText = FString::Printf(TEXT("Spawn failed: row %s not found."), *RowName.ToString());
        return;
    }

    // 원본 테이블은 건드리지 않고 이번 요청의 행 사본만 바꾼다.
    FInstancedStruct RowData;
    RowData.InitializeAs(Table->GetRowStruct(), RowMemory);
    FKataNPCCharacterRow* Row = RowData.GetMutablePtr<FKataNPCCharacterRow>();
    if (Row == nullptr)
    {
        StatusText = TEXT("Spawn failed: not an NPC row.");
        return;
    }

    const TSharedRef<FRepeatRequest> Repeat = MakeShared<FRepeatRequest>();
    Repeat->Mode = User->AIMode;
    Repeat->Interval = User->RepeatInterval;
    if (Repeat->Mode != EKataRuntimeSpawnAIMode::Default)
    {
        // AI Data를 비우면 Controller는 빙의하되 StateTree·Perception을 시작하지 않는다.
        Row->AIData.Reset();
        Row->AIDataOverride = FKataAIDataOverride();
    }
    if (Repeat->Mode == EKataRuntimeSpawnAIMode::RepeatAction)
    {
        Repeat->Action.Reset(ActionPaths.IsValidIndex(ActionIndex) ? Cast<UKataAction>(ActionPaths[ActionIndex].TryLoad()) : nullptr);
        if (!Repeat->Action.IsValid())
        {
            StatusText = TEXT("Spawn failed: select an action.");
            return;
        }
    }
    else if (Repeat->Mode == EKataRuntimeSpawnAIMode::RepeatGraph)
    {
        Repeat->Graph.Reset(GraphPaths.IsValidIndex(GraphIndex) ? Cast<UKataGraph>(GraphPaths[GraphIndex].TryLoad()) : nullptr);
        if (!Repeat->Graph.IsValid())
        {
            StatusText = TEXT("Spawn failed: select a graph.");
            return;
        }
        Repeat->EntryTrigger = FGameplayTag::RequestGameplayTag(User->EntryTrigger, false);
    }

    // 캐릭터 중심이 바닥 위에 놓이도록 Character Class의 캡슐 절반 높이를 쓴다. 에디터 도구라 동기 로드한다.
    float HalfHeight = 0.f;
    if (const UClass* CharacterClass = Row->CharacterClass.LoadSynchronous())
    {
        if (const ACharacter* Defaults = Cast<ACharacter>(CharacterClass->GetDefaultObject()))
        {
            HalfHeight = Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        }
    }

    FKataRuntimeSpawnLayout Layout;
    Layout.Distance = User->Distance;
    Layout.YawOffset = User->YawOffset;
    Layout.Count = User->Count;
    Layout.Pattern = User->Pattern;
    Layout.Spacing = User->Spacing;
    TArray<FTransform> Transforms;
    KataRuntimeSpawnLayout::Build(Player->GetActorLocation(), Player->GetActorRotation().Yaw, Layout, Transforms);

    const FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(KataRuntimeSpawner), false, Player);
    const FCollisionObjectQueryParams TraceObjects(ECC_WorldStatic);
    const FKataCharacterId CharacterId(RowName);
    const uint32 RequestSession = Session;
    for (FTransform& Transform : Transforms)
    {
        // 바닥을 찾지 못하면 플레이어 중심 높이를 유지하고 엔진 충돌 조정에 맡긴다.
        FVector Location = Transform.GetLocation();
        FHitResult Hit;
        if (World->LineTraceSingleByObjectType(Hit, Location + FVector(0.f, 0.f, 500.f), Location - FVector(0.f, 0.f, 2000.f),
            TraceObjects, TraceParams))
        {
            Location.Z = Hit.ImpactPoint.Z + HalfHeight;
            Transform.SetLocation(Location);
        }
        ++PendingCount;
        Subsystem->RequestSpawnFromRow(CharacterId, RowData, Transform,
            FKataCharacterSpawnDelegate::CreateLambda([this, RequestSession, Repeat](AKataCharacter* Character)
            {
                HandleSpawned(Character, RequestSession, Repeat);
            }),
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    }
    StatusText = FString::Printf(TEXT("Requested %d x %s."), Transforms.Num(), *RowName.ToString());
}

void FKataRuntimeSpawnerTab::HandleSpawned(AKataCharacter* Character, uint32 RequestSession, const TSharedRef<FRepeatRequest>& Repeat)
{
    if (RequestSession != Session)
    {
        return;
    }
    PendingCount = FMath::Max(PendingCount - 1, 0);
    if (Character == nullptr)
    {
        StatusText = TEXT("A spawn request failed. See the log.");
        return;
    }
    SpawnedCharacters.Add(Character);
    if (Repeat->Mode == EKataRuntimeSpawnAIMode::RepeatAction)
    {
        Loop.AddAction(Character, Repeat->Action.Get(), Repeat->Interval);
    }
    else if (Repeat->Mode == EKataRuntimeSpawnAIMode::RepeatGraph)
    {
        Loop.AddGraph(Character, Repeat->Graph.Get(), Repeat->EntryTrigger, Repeat->Interval);
    }
}

void FKataRuntimeSpawnerTab::DespawnCharacters()
{
    // 이 도구로 만든 NPC만 제거한다. 스폰 서비스가 만든 AI Controller는 Pawn과 함께 정리한다.
    for (const TWeakObjectPtr<AKataCharacter>& WeakCharacter : SpawnedCharacters)
    {
        AKataCharacter* Character = WeakCharacter.Get();
        if (Character == nullptr)
        {
            continue;
        }
        AController* Controller = Character->GetController();
        Character->Destroy();
        if (Controller != nullptr && !Controller->IsPlayerController())
        {
            Controller->Destroy();
        }
    }
    SpawnedCharacters.Reset();
    StatusText = TEXT("Despawned.");
}

void FKataRuntimeSpawnerTab::HandleEndPIE(bool bIsSimulating)
{
    // 이전 세션의 늦은 콜백을 무시하고 월드와 함께 사라진 기록을 비운다.
    ++Session;
    Loop.Reset();
    SpawnedCharacters.Reset();
    PendingCount = 0;
    StatusText.Reset();
    SaveUserSettings();
}

void FKataRuntimeSpawnerTab::SaveUserSettings()
{
    if (!bUserSettingsDirty)
    {
        return;
    }
    bUserSettingsDirty = false;
    GetMutableDefault<UKataRuntimeSpawnerUserSettings>()->SaveConfig();
}

UWorld* FKataRuntimeSpawnerTab::GetPlayWorld()
{
    return GEditor != nullptr ? GEditor->PlayWorld.Get() : nullptr;
}
