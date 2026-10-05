#include "KataGASAbilityTriggerIndex.h"

#include "Abilities/GameplayAbility.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Blueprint/BlueprintSupport.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "HAL/PlatformTime.h"
#include "KataGASAbilityMetadata.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectIterator.h"

FKataGASAbilityTriggerIndex::FKataGASAbilityTriggerIndex()
{
    Registry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    AddedHandle = Registry->OnAssetAdded().AddRaw(this, &FKataGASAbilityTriggerIndex::OnAssetChanged);
    RemovedHandle = Registry->OnAssetRemoved().AddRaw(this, &FKataGASAbilityTriggerIndex::OnAssetChanged);
    UpdatedHandle = Registry->OnAssetUpdated().AddRaw(this, &FKataGASAbilityTriggerIndex::OnAssetChanged);
    RenamedHandle = Registry->OnAssetRenamed().AddRaw(this, &FKataGASAbilityTriggerIndex::OnAssetRenamed);
    if (GEditor)
    {
        CompiledHandle = GEditor->OnBlueprintCompiled().AddRaw(this, &FKataGASAbilityTriggerIndex::OnBlueprintCompiled);
    }
}

FKataGASAbilityTriggerIndex::~FKataGASAbilityTriggerIndex()
{
    if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
    {
        Registry->OnAssetAdded().Remove(AddedHandle);
        Registry->OnAssetRemoved().Remove(RemovedHandle);
        Registry->OnAssetUpdated().Remove(UpdatedHandle);
        Registry->OnAssetRenamed().Remove(RenamedHandle);
    }
    if (GEditor)
    {
        GEditor->OnBlueprintCompiled().Remove(CompiledHandle);
    }
}

void FKataGASAbilityTriggerIndex::Start(const FString& InContentRoot)
{
    ContentRoot = InContentRoot.TrimStartAndEnd();
    ContentRoot.RemoveFromEnd(TEXT("/"));
    Records.Reset();
    Results.Reset();
    Candidates.Reset();
    IndexedClasses.Reset();
    RecordKeys.Reset();
    NextCandidate = 0;
    FailedLoads = 0;
    UnsupportedSchemas = 0;
    bCancelled = false;
    bHasScanned = true;
    bDirty.Store(false);
    bScanning = true;
    bWaitingForRegistry = Registry->IsLoadingAssets();
    ++Revision;
    if (!bWaitingForRegistry)
    {
        BuildCandidates();
    }
}

void FKataGASAbilityTriggerIndex::BuildCandidates()
{
    check(IsInGameThread());
    TSet<FTopLevelAssetPath> DerivedClasses;
    Registry->GetDerivedClassNames({ UGameplayAbility::StaticClass()->GetClassPathName() }, {}, DerivedClasses);
    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    if (!ContentRoot.IsEmpty())
    {
        Filter.PackagePaths.Add(FName(*ContentRoot));
        Filter.bRecursivePaths = true;
    }
    TArray<FAssetData> Assets;
    Registry->GetAssets(Filter, Assets);
    TSet<FSoftObjectPath> Paths;
    for (const FAssetData& Asset : Assets)
    {
        FString GeneratedPath;
        Asset.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedPath);
        const FSoftObjectPath ClassPath(FPackageName::ExportTextPathToObjectPath(GeneratedPath));
        if (DerivedClasses.Contains(ClassPath.GetAssetPath()))
        {
            Paths.Add(Asset.GetSoftObjectPath());
        }
    }

    // 참조 인덱스는 후보를 보강할 뿐이며 실제 Trigger 설정은 CDO에서 다시 판정한다.
    for (const FString& Query : QueryTags)
    {
        const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Query), false);
        if (!Tag.IsValid())
        {
            continue;
        }
        TArray<FAssetDependency> References;
        Registry->GetReferencers(FAssetIdentifier(FGameplayTag::StaticStruct(), Tag.GetTagName()),
            References, UE::AssetRegistry::EDependencyCategory::SearchableName);
        for (const FAssetDependency& Reference : References)
        {
            TArray<FAssetData> PackageAssets;
            Registry->GetAssetsByPackageName(Reference.AssetId.PackageName, PackageAssets);
            for (const FAssetData& Asset : PackageAssets)
            {
                const FString Package = Asset.PackageName.ToString();
                if (ContentRoot.IsEmpty() || Package.StartsWith(ContentRoot + TEXT("/")))
                {
                    FString GeneratedPath;
                    Asset.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedPath);
                    if (DerivedClasses.Contains(FSoftObjectPath(FPackageName::ExportTextPathToObjectPath(GeneratedPath)).GetAssetPath()))
                    {
                        Paths.Add(Asset.GetSoftObjectPath());
                    }
                }
            }
        }
    }
    Candidates = Paths.Array();
    Candidates.Sort([](const FSoftObjectPath& A, const FSoftObjectPath& B) { return A.ToString() < B.ToString(); });

    // 네이티브 클래스와 현재 로드된 미저장 설정은 별도 출처로 포함한다.
    for (TObjectIterator<UClass> It; It; ++It)
    {
        UClass* Class = *It;
        if (!Class->IsChildOf(UGameplayAbility::StaticClass())
            || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
            || Class->GetName().StartsWith(TEXT("SKEL_")) || Class->GetName().StartsWith(TEXT("REINST_")))
        {
            continue;
        }
        if (!Class->ClassGeneratedBy)
        {
            ReadClass(Class, TEXT("Loaded native class"));
        }
        else
        {
            const FString Package = Class->ClassGeneratedBy->GetOutermost()->GetName();
            if (ContentRoot.IsEmpty() || Package.StartsWith(ContentRoot + TEXT("/")))
            {
                ReadClass(Class, TEXT("Loaded Blueprint CDO (may include unsaved changes)"));
            }
        }
    }
    RebuildResults();
}

void FKataGASAbilityTriggerIndex::ReadClass(UClass* Class, const FString& Origin)
{
    if (!IsValid(Class) || !Class->IsChildOf(UGameplayAbility::StaticClass())
        || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
    {
        return;
    }
    const FString ClassPath = Class->GetPathName();
    if (IndexedClasses.Contains(ClassPath))
    {
        return;
    }
    IndexedClasses.Add(ClassPath);
    const UGameplayAbility* Ability = Class->GetDefaultObject<UGameplayAbility>();
    TArray<FAbilityTriggerData> Triggers;
    if (!FKataGASAbilityMetadata::ReadTriggers(Ability, Triggers))
    {
        ++UnsupportedSchemas;
        return;
    }
    for (const FAbilityTriggerData& Trigger : Triggers)
    {
        const FString TagName = Trigger.TriggerTag.ToString();
        const FString SourceType = UEnum::GetValueAsString(Trigger.TriggerSource);
        const FString Key = ClassPath + TEXT("|") + TagName + TEXT("|") + SourceType;
        if (RecordKeys.Contains(Key))
        {
            continue;
        }
        RecordKeys.Add(Key);
        TSharedPtr<FKataGASInspectionRow> Row = MakeShared<FKataGASInspectionRow>();
        Row->Page = EKataGASInspectionPage::Triggers;
        Row->Key = Key;
        Row->Name = Class->GetName();
        Row->State = SourceType;
        Row->Tags = TagName;
        Row->Source = ClassPath;
        Row->Value = Origin;
        Row->AssetPath = FKataGASAbilityMetadata::GetSourceAsset(Class);
        Row->Detail = TEXT("Trigger setting only. This does not indicate that an event occurred or activation succeeded.");
        Records.Add(Row);
    }
}

void FKataGASAbilityTriggerIndex::Tick()
{
    if (!bScanning)
    {
        return;
    }
    if (bDirty.Load())
    {
        // 변경된 CDO와 이전 결과를 섞지 않고 명시적인 재검색을 기다린다.
        bScanning = false;
        ++Revision;
        return;
    }
    if (bWaitingForRegistry)
    {
        if (Registry->IsLoadingAssets())
        {
            return;
        }
        bWaitingForRegistry = false;
        BuildCandidates();
    }
    const double BudgetEnd = FPlatformTime::Seconds() + 0.004;
    int32 LoadedThisTick = 0;
    while (NextCandidate < Candidates.Num() && LoadedThisTick < 2)
    {
        const FAssetData Asset = Registry->GetAssetByObjectPath(Candidates[NextCandidate++]);
        UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
        if (IsValid(Blueprint) && IsValid(Blueprint->GeneratedClass))
        {
            ReadClass(Blueprint->GeneratedClass, TEXT("Blueprint CDO"));
        }
        else
        {
            ++FailedLoads;
        }
        ++LoadedThisTick;
        if (FPlatformTime::Seconds() >= BudgetEnd)
        {
            break;
        }
    }
    if (NextCandidate >= Candidates.Num())
    {
        bScanning = false;
    }
    RebuildResults();
}

void FKataGASAbilityTriggerIndex::Cancel()
{
    bScanning = false;
    bWaitingForRegistry = false;
    bCancelled = true;
    ++Revision;
}

void FKataGASAbilityTriggerIndex::SetQuery(const FString& InTags, bool bInIncludeChildren)
{
    FString Normalized = InTags;
    Normalized.ReplaceInline(TEXT(","), TEXT(" "));
    Normalized.ParseIntoArrayWS(QueryTags);
    bIncludeChildren = bInIncludeChildren;
    RebuildResults();
}

void FKataGASAbilityTriggerIndex::RebuildResults()
{
    Results.Reset();
    for (const auto& Row : Records)
    {
        bool bMatches = QueryTags.IsEmpty();
        for (const FString& Query : QueryTags)
        {
            bMatches |= Row->Tags.Equals(Query, ESearchCase::CaseSensitive)
                || (bIncludeChildren && Row->Tags.StartsWith(Query + TEXT("."), ESearchCase::CaseSensitive));
        }
        if (bMatches)
        {
            Results.Add(Row);
        }
    }
    Results.Sort([](const auto& A, const auto& B) { return A->Key < B->Key; });
    ++Revision;
}

FString FKataGASAbilityTriggerIndex::GetStatus() const
{
    const TCHAR* State = !bHasScanned ? TEXT("Not scanned")
        : bDirty.Load() ? TEXT("Stale; rescan after asset changes")
        : bCancelled ? TEXT("Cancelled; partial results")
        : bWaitingForRegistry ? TEXT("Waiting for Asset Registry")
        : bScanning ? TEXT("Scanning; partial results")
        : TEXT("Scan complete for selected scope");
    return FString::Printf(TEXT("%s | Scope: %s | %d/%d assets | %d classes | %d matches | Failed: %d | Unsupported: %d | Native: loaded classes only"),
        State, ContentRoot.IsEmpty() ? TEXT("All content") : *ContentRoot,
        NextCandidate, Candidates.Num(), IndexedClasses.Num(), Results.Num(), FailedLoads, UnsupportedSchemas);
}

void FKataGASAbilityTriggerIndex::OnAssetChanged(const FAssetData& Asset)
{
    bDirty.Store(true);
}

void FKataGASAbilityTriggerIndex::OnAssetRenamed(const FAssetData& Asset, const FString& OldPath)
{
    bDirty.Store(true);
}

void FKataGASAbilityTriggerIndex::OnBlueprintCompiled()
{
    bDirty.Store(true);
}
