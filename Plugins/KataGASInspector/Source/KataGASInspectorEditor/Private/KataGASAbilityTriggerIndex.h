#pragma once

#include "CoreMinimal.h"
#include "KataGASInspectionTypes.h"

class IAssetRegistry;
class UClass;
struct FAssetData;

/** 검색은 설정 카탈로그를 읽으며 Ability를 실행하거나 에셋을 저장하지 않는다. */
class FKataGASAbilityTriggerIndex
{
public:
    FKataGASAbilityTriggerIndex();
    ~FKataGASAbilityTriggerIndex();
    void Start(const FString& InContentRoot);
    void Cancel();
    void Tick();
    void SetQuery(const FString& InTags, bool bInIncludeChildren);
    const TArray<TSharedPtr<FKataGASInspectionRow>>& GetRows() const { return Results; }
    FString GetStatus() const;
    uint64 GetRevision() const { return Revision; }
    bool IsScanning() const { return bScanning; }

private:
    void BuildCandidates();
    void ReadClass(UClass* Class, const FString& Origin);
    void RebuildResults();
    void OnAssetChanged(const FAssetData& Asset);
    void OnAssetRenamed(const FAssetData& Asset, const FString& OldPath);
    void OnBlueprintCompiled();
    IAssetRegistry* Registry = nullptr;
    FDelegateHandle AddedHandle;
    FDelegateHandle RemovedHandle;
    FDelegateHandle UpdatedHandle;
    FDelegateHandle RenamedHandle;
    FDelegateHandle CompiledHandle;
    TAtomic<bool> bDirty { false };
    bool bScanning = false;
    bool bWaitingForRegistry = false;
    bool bCancelled = false;
    bool bHasScanned = false;
    bool bIncludeChildren = false;
    FString ContentRoot = TEXT("/Game");
    TArray<FString> QueryTags;
    TArray<FSoftObjectPath> Candidates;
    int32 NextCandidate = 0;
    int32 FailedLoads = 0;
    int32 UnsupportedSchemas = 0;
    uint64 Revision = 0;
    TSet<FString> IndexedClasses;
    TSet<FString> RecordKeys;
    TArray<TSharedPtr<FKataGASInspectionRow>> Records;
    TArray<TSharedPtr<FKataGASInspectionRow>> Results;
};
