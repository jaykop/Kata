#include "KataDebugPanel_Spawner.h"

#include "Action/KataAction.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Character/KataCharacter.h"
#include "Character/KataCharacterRow.h"
#include "Character/KataCharacterSpawnSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Data/KataRowId.h"
#include "Engine/DataTable.h"
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
#include "Styling/StyleColors.h"
#include "UObject/StrongObjectPtr.h"

/** 생성 요청 하나가 완료 콜백까지 들고 가는 반복 설정이다. 로드한 에셋이 콜백 전에 GC되지 않게 강한 참조로 잡는다. */
struct UKataDebugPanel_Spawner::FRepeatRequest
{
    EKataRuntimeSpawnAIMode Mode = EKataRuntimeSpawnAIMode::Default;
    TStrongObjectPtr<UKataAction> Action;
    TStrongObjectPtr<UKataGraph> Graph;
    FGameplayTag EntryTrigger;
    float Interval = 0.f;
};

namespace KataDebugPanelSpawnerPrivate
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

    void CollectAssets(UClass* Class, TArray<FSoftObjectPath>& OutPaths, TArray<FString>& OutFolders)
    {
        OutPaths.Reset();
        OutFolders.Reset();
        TArray<FAssetData> Assets;
        IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
        Registry.GetAssetsByClass(Class->GetClassPathName(), Assets, true);
        Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.AssetName.LexicalLess(B.AssetName); });
        for (const FAssetData& Asset : Assets)
        {
            OutPaths.Add(Asset.GetSoftObjectPath());
            OutFolders.Add(Asset.PackagePath.ToString());
        }
    }

    /** 비우면 묶음 없음, 아니면 폴더 경로를 묶음 이름으로 쓴다. 폴더 필터가 켜져 있으면 캐릭터 폴더 기준 상대 경로다. */
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
     * 폴더 필터를 통과한 항목으로 선택 목록을 채운다. 현재 선택이 걸러지면 첫 항목으로, 남은 항목이 없으면 INDEX_NONE으로 바꾼다.
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

    /** 라벨과 선택 메뉴 버튼을 한 줄에 그린다. 사용자가 고른 원본 인덱스를 돌려주며, 고르지 않았으면 INDEX_NONE이다. */
    int32 DrawPicker(const TCHAR* Label, FKataRuntimeSpawnerPicker& Picker, const FString& CurrentLabel)
    {
        BeginRow(Label);
        Picker.SetCurrentLabel(CurrentLabel);
        SlateIM::Widget(Picker.GetWidget());
        EndRow();
        return Picker.ConsumeSelection();
    }
}

UKataDebugPanel_Spawner::UKataDebugPanel_Spawner()
    : RowPicker(MakeShared<FKataRuntimeSpawnerPicker>())
    , ActionPicker(MakeShared<FKataRuntimeSpawnerPicker>())
    , GraphPicker(MakeShared<FKataRuntimeSpawnerPicker>())
{
}

void UKataDebugPanel_Spawner::Draw(const FKataDebugPanelContext& Context)
{
    using namespace KataDebugPanelSpawnerPrivate;
    using namespace KataDebugLayout;
    if (!bUserSettingsSanitized)
    {
        SanitizeUserSettings();
        bUserSettingsSanitized = true;
    }
    RefreshTables(false);

    // 섹션마다 독립 묶음이라 한 섹션의 줄 수가 바뀌어도 다른 섹션의 위젯 순서는 그대로다.
    BeginSection();
    DrawCharacterSection();
    EndSection();
    BeginSection();
    DrawPlacementSection();
    EndSection();
    BeginSection();
    DrawAISection();
    EndSection();
    BeginSection();
    DrawActionsSection(Context);
    EndSection();

    bRefreshTableCombo = false;
    bRefreshTriggerCombo = false;
}

void UKataDebugPanel_Spawner::OnHidden()
{
    SaveUserSettings();
}

void UKataDebugPanel_Spawner::OnPlaySessionEnded()
{
    // PIE 월드와 함께 사라진 기록을 비우고, 늦게 도착한 이전 세션의 생성 결과는 세션 번호로 버린다.
    ++Session;
    Loop.Reset();
    SpawnedCharacters.Reset();
    PendingCount = 0;
    SaveUserSettings();
}

void UKataDebugPanel_Spawner::DrawCharacterSection()
{
    using namespace KataDebugPanelSpawnerPrivate;
    using namespace KataDebugLayout;
    UKataRuntimeSpawnerUserSettings* User = GetMutableDefault<UKataRuntimeSpawnerUserSettings>();

    KataDebugLayout::SectionHeader(TEXT("Character"));
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

    const int32 PickedRow = DrawPicker(TEXT("Row"), *RowPicker, RowNames.IsValidIndex(RowIndex) ? RowNames[RowIndex] : FString());
    if (RowNames.IsValidIndex(PickedRow) && PickedRow != RowIndex)
    {
        RowIndex = PickedRow;
        User->RowName = FName(*RowNames[RowIndex]);
        bUserSettingsDirty = true;
        UpdateCharacterFolder();
    }

    // 조건부 문구는 섹션 마지막에 두어 앞의 위젯 순서를 바꾸지 않는다.
    if (TableNames.IsEmpty())
    {
        SlateIM::Padding(FMargin(4.f, 2.f));
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::Text(TEXT("Add NPC tables in Project Settings > Editor > Kata Runtime Spawner."));
    }
}

void UKataDebugPanel_Spawner::DrawPlacementSection()
{
    using namespace KataDebugPanelSpawnerPrivate;
    using namespace KataDebugLayout;
    UKataRuntimeSpawnerUserSettings* User = GetMutableDefault<UKataRuntimeSpawnerUserSettings>();

    KataDebugLayout::SectionHeader(TEXT("Placement"));
    BeginRow(TEXT("Distance (cm)"));
    bUserSettingsDirty |= SlateIM::SpinBox(User->Distance, { .Min = 0.f });
    EndRow();

    BeginRow(TEXT("Yaw Offset (deg)"));
    bUserSettingsDirty |= SlateIM::SpinBox(User->YawOffset, { .Min = -180.f, .Max = 180.f });
    EndRow();

    BeginRow(TEXT("Count"));
    // SlateIM은 SSpinBox<float>와 SSpinBox<int32>를 같은 종류로 보고 재사용할 수 있어 정수도 float 입력칸으로 받는다.
    float CountValue = static_cast<float>(User->Count);
    if (SlateIM::SpinBox(CountValue, { .Min = 1.f, .Max = 50.f }))
    {
        User->Count = FMath::Clamp(FMath::RoundToInt(CountValue), 1, 50);
        bUserSettingsDirty = true;
    }
    EndRow();

    BeginRow(TEXT("Pattern"));
    int32 PatternIndex = static_cast<int32>(User->Pattern);
    if (SlateIM::ComboBox(PatternNames, PatternIndex) && PatternNames.IsValidIndex(PatternIndex))
    {
        User->Pattern = static_cast<EKataRuntimeSpawnPattern>(PatternIndex);
        bUserSettingsDirty = true;
    }
    EndRow();

    // 원형은 원 전체에 균등하게 놓아 간격을 쓰지 않는다. 마지막 줄이라 숨겨도 앞 위젯 순서는 그대로지만 묶음으로 감싼다.
    BeginSection();
    if (User->Pattern == EKataRuntimeSpawnPattern::Line)
    {
        BeginRow(TEXT("Spacing (cm)"));
        bUserSettingsDirty |= SlateIM::SpinBox(User->Spacing, { .Min = 0.f });
        EndRow();
    }
    EndSection();
}

void UKataDebugPanel_Spawner::DrawAISection()
{
    using namespace KataDebugPanelSpawnerPrivate;
    using namespace KataDebugLayout;
    UKataRuntimeSpawnerUserSettings* User = GetMutableDefault<UKataRuntimeSpawnerUserSettings>();

    KataDebugLayout::SectionHeader(TEXT("AI"));
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
    BeginSection();
    if (bRepeatAction || bRepeatGraph)
    {
        DrawRepeatRows(bRepeatAction, bRepeatGraph);
    }
    EndSection();
}

void UKataDebugPanel_Spawner::DrawRepeatRows(bool bRepeatAction, bool bRepeatGraph)
{
    using namespace KataDebugPanelSpawnerPrivate;
    using namespace KataDebugLayout;
    UKataRuntimeSpawnerUserSettings* User = GetMutableDefault<UKataRuntimeSpawnerUserSettings>();
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
    BeginSection();
    if ((bRepeatAction ? VisibleActionCount : VisibleGraphCount) == 0)
    {
        SlateIM::Padding(FMargin(4.f, 2.f));
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::Text(User->bFilterByCharacterFolder && !CharacterFolder.IsEmpty()
            ? TEXT("No assets in the character folder. Turn off \"Character folder only\" to see all assets.")
            : TEXT("No assets found. Press Refresh after the asset registry finishes loading."));
    }
    EndSection();

    BeginSection();
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
    EndSection();

    BeginRow(TEXT("Repeat Interval (s)"));
    bUserSettingsDirty |= SlateIM::SpinBox(User->RepeatInterval, { .Min = 0.f });
    EndRow();
}

void UKataDebugPanel_Spawner::DrawActionsSection(const FKataDebugPanelContext& Context)
{
    KataDebugLayout::SectionHeader(TEXT("Spawn"));
    const UKataRuntimeSpawnerUserSettings* User = GetDefault<UKataRuntimeSpawnerUserSettings>();
    SpawnedCharacters.RemoveAllSwap([](const TWeakObjectPtr<AKataCharacter>& Character) { return !Character.IsValid(); });

    const bool bHasPlayer = Context.PlayerController != nullptr && Context.PlayerController->GetPawn() != nullptr;
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

    SlateIM::Padding(FMargin(4.f, 2.f));
    SlateIM::BeginHorizontalStack();
    if (SlateIM::Button(TEXT("Spawn"), { .bEnabled = bCanSpawn }))
    {
        SpawnCharacters(Context.World, Context.PlayerController);
    }
    SlateIM::Padding(FMargin(8.f, 0.f, 0.f, 0.f));
    if (SlateIM::Button(FString::Printf(TEXT("Despawn All (%d)"), SpawnedCharacters.Num()), { .bEnabled = !SpawnedCharacters.IsEmpty() }))
    {
        DespawnCharacters();
    }
    SlateIM::EndHorizontalStack();

    SlateIM::Padding(FMargin(4.f, 6.f, 4.f, 2.f));
    if (Context.World == nullptr)
    {
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::Text(TEXT("Start PIE to spawn."));
    }
    else if (!bHasPlayer)
    {
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::Text(TEXT("Waiting for the player pawn."));
    }
    else
    {
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::Text(FString::Printf(TEXT("Alive %d  Pending %d  Repeating %d"), SpawnedCharacters.Num(), PendingCount, Loop.Num()));
    }

    // 마지막 동작 결과. 칼럼 마지막 줄이라 나타나고 사라져도 앞 위젯 순서는 그대로다.
    if (!StatusText.IsEmpty())
    {
        SlateIM::Padding(FMargin(4.f, 2.f));
        SlateIM::HAlign(HAlign_Fill);
        SlateIM::Text(StatusText);
    }
}

void UKataDebugPanel_Spawner::RefreshTables(bool bForce)
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
    TableIndex = KataDebugPanelSpawnerPrivate::FindIndexOrFirst(TablePaths, GetDefault<UKataRuntimeSpawnerUserSettings>()->Table);
    bRefreshTableCombo = true;
    RefreshRows();
}

void UKataDebugPanel_Spawner::RefreshRows()
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

void UKataDebugPanel_Spawner::ApplyRowFilter()
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

void UKataDebugPanel_Spawner::UpdateCharacterFolder()
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

void UKataDebugPanel_Spawner::ApplyAssetFilters()
{
    using namespace KataDebugPanelSpawnerPrivate;
    using namespace KataDebugLayout;
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

void UKataDebugPanel_Spawner::RefreshCatalog()
{
    using namespace KataDebugPanelSpawnerPrivate;
    using namespace KataDebugLayout;
    const UKataRuntimeSpawnerUserSettings* User = GetDefault<UKataRuntimeSpawnerUserSettings>();

    CollectAssets(UKataAction::StaticClass(), ActionPaths, ActionFolders);
    CollectAssets(UKataGraph::StaticClass(), GraphPaths, GraphFolders);
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

void UKataDebugPanel_Spawner::SpawnCharacters(UWorld* World, APlayerController* PlayerController)
{
    SaveUserSettings();
    const UKataRuntimeSpawnerUserSettings* User = GetDefault<UKataRuntimeSpawnerUserSettings>();
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

    // 캐릭터 중심이 바닥 위에 놓이도록 Character Class의 캡슐 절반 높이를 쓴다. 디버그 도구라 동기 로드한다.
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
    const TWeakObjectPtr<UKataDebugPanel_Spawner> WeakThis(this);
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
        // 비동기 생성이 끝나기 전에 패널이 사라질 수 있으므로 약한 참조로 돌아온다.
        Subsystem->RequestSpawnFromRow(CharacterId, RowData, Transform,
            FKataCharacterSpawnDelegate::CreateLambda([WeakThis, RequestSession, Repeat](AKataCharacter* Character)
            {
                if (UKataDebugPanel_Spawner* Panel = WeakThis.Get())
                {
                    Panel->HandleSpawned(Character, RequestSession, Repeat);
                }
            }),
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    }
    StatusText = FString::Printf(TEXT("Requested %d x %s."), Transforms.Num(), *RowName.ToString());
}

void UKataDebugPanel_Spawner::HandleSpawned(AKataCharacter* Character, uint32 RequestSession, const TSharedRef<FRepeatRequest>& Repeat)
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

void UKataDebugPanel_Spawner::DespawnCharacters()
{
    // 이 패널로 만든 NPC만 제거한다. 스폰 서비스가 만든 AI Controller는 Pawn과 함께 정리한다.
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

void UKataDebugPanel_Spawner::SanitizeUserSettings()
{
    // 범위를 벗어난 저장값(위젯 재사용 문제로 깨진 값 포함)은 기본값으로 되돌린다.
    UKataRuntimeSpawnerUserSettings* User = GetMutableDefault<UKataRuntimeSpawnerUserSettings>();
    auto FixFloat = [this](float& Value, float Min, float Max, float Default)
    {
        if (!FMath::IsFinite(Value) || Value < Min || Value > Max)
        {
            Value = Default;
            bUserSettingsDirty = true;
        }
    };
    constexpr float MaxDistance = 100000.f;
    FixFloat(User->Distance, 0.f, MaxDistance, 300.f);
    FixFloat(User->YawOffset, -180.f, 180.f, 0.f);
    FixFloat(User->Spacing, 0.f, MaxDistance, 150.f);
    FixFloat(User->RepeatInterval, 0.f, 3600.f, 1.f);
    if (User->Count < 1 || User->Count > 50)
    {
        User->Count = 1;
        bUserSettingsDirty = true;
    }
}

void UKataDebugPanel_Spawner::SaveUserSettings()
{
    if (!bUserSettingsDirty)
    {
        return;
    }
    bUserSettingsDirty = false;
    GetMutableDefault<UKataRuntimeSpawnerUserSettings>()->SaveConfig();
}
