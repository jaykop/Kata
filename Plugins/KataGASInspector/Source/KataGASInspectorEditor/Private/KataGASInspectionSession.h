#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "KataGASInspectionTypes.h"

/** 창마다 대상과 갱신 상태를 독립적으로 소유한다. 게임의 실행 상태는 바꾸지 않는다. */
class FKataGASInspectionSession
{
public:
    void Tick(double Now);
    void Refresh();
    void RescanTargets();
    void SelectWorld(FName Context);
    void SelectTarget(UAbilitySystemComponent* ASC);
    void SetAutomaticWorld(bool bAutomatic);
    void SetFrozen(bool bInFrozen) { bFrozen = bInFrozen; }
    bool IsFrozen() const { return bFrozen; }
    bool IsAutomaticWorld() const { return bAutomaticWorld; }
    void SetInterval(float Seconds) { Interval = FMath::Clamp(Seconds, 0.1f, 1.0f); }
    float GetInterval() const { return Interval; }
    const TArray<TSharedPtr<FKataGASInspectionWorld>>& GetWorlds() const { return Worlds; }
    const TArray<TSharedPtr<FKataGASInspectionTarget>>& GetTargets() const { return Targets; }
    const FKataGASInspectionSnapshot& GetSnapshot() const { return Snapshot; }
    FName GetSelectedWorld() const { return SelectedContext; }
    UAbilitySystemComponent* GetSelectedTarget() const { return SelectedASC.Get(); }
    FString GetStatus() const { return Status; }
    uint64 GetRevision() const { return Revision; }
    uint64 GetCatalogRevision() const { return CatalogRevision; }

private:
    void UpdateWorlds();
    void ChangeWorld(FName Context);
    void MergeSnapshot(FKataGASInspectionSnapshot&& NewSnapshot);
    TWeakObjectPtr<UWorld> SelectedWorld;
    TWeakObjectPtr<UAbilitySystemComponent> SelectedASC;
    FName SelectedContext;
    TArray<TSharedPtr<FKataGASInspectionWorld>> Worlds;
    TArray<TSharedPtr<FKataGASInspectionTarget>> Targets;
    FKataGASInspectionSnapshot Snapshot;
    FString Status = TEXT("Select an ability system component.");
    bool bAutomaticWorld = true;
    bool bFrozen = false;
    bool bChooseInitialTarget = true;
    float Interval = 0.2f;
    double LastRefresh = -1.0;
    double LastCatalog = -1.0;
    uint64 Revision = 0;
    uint64 CatalogRevision = 0;
};
