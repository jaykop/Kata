#include "KataGASInspectionSession.h"

#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "KataGASSnapshotCollector.h"
#include "UObject/UObjectIterator.h"

namespace KataGASSession
{
    void IndexRows(const TArray<TSharedPtr<FKataGASInspectionRow>>& Rows,
        TMap<FString, TSharedPtr<FKataGASInspectionRow>>& Index)
    {
        for (const TSharedPtr<FKataGASInspectionRow>& Row : Rows)
        {
            Index.Add(Row->Key, Row);
            IndexRows(Row->Children, Index);
        }
    }

    void ReuseRows(TArray<TSharedPtr<FKataGASInspectionRow>>& Rows,
        const TMap<FString, TSharedPtr<FKataGASInspectionRow>>& Index)
    {
        for (TSharedPtr<FKataGASInspectionRow>& Row : Rows)
        {
            ReuseRows(Row->Children, Index);
            if (const TSharedPtr<FKataGASInspectionRow>* Existing = Index.Find(Row->Key))
            {
                **Existing = MoveTemp(*Row);
                Row = *Existing;
            }
        }
    }
}

void FKataGASInspectionSession::Tick(double Now)
{
    UpdateWorlds();
    if (LastCatalog < 0.0 || Now - LastCatalog >= 1.0)
    {
        RescanTargets();
        LastCatalog = Now;
    }
    if (!bFrozen && (LastRefresh < 0.0 || Now - LastRefresh >= Interval))
    {
        Refresh();
        LastRefresh = Now;
    }
}

void FKataGASInspectionSession::UpdateWorlds()
{
    TArray<TSharedPtr<FKataGASInspectionWorld>> NewWorlds;
    FName PreferredContext;
    int32 PreferredPriority = -1;
    if (GEngine)
    {
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            // 외부 게임 프로세스나 네트워크 대상은 이 도구의 수집 범위가 아니다.
            if ((Context.WorldType != EWorldType::Editor && Context.WorldType != EWorldType::PIE)
                || Context.RunAsDedicated || !IsValid(Context.World()))
            {
                continue;
            }
            TSharedPtr<FKataGASInspectionWorld> Item = MakeShared<FKataGASInspectionWorld>();
            Item->Context = Context.ContextHandle;
            Item->World = Context.World();
            Item->Label = Context.WorldType == EWorldType::PIE
                ? FString::Printf(TEXT("PIE %d | %s"), Context.PIEInstance, *Context.World()->GetName())
                : TEXT("Editor | ") + Context.World()->GetName();
            NewWorlds.Add(Item);
            const int32 Priority = Context.WorldType == EWorldType::PIE ? 1 : 0;
            if (Priority > PreferredPriority)
            {
                PreferredPriority = Priority;
                PreferredContext = Item->Context;
            }
        }
    }
    bool bChanged = NewWorlds.Num() != Worlds.Num();
    if (!bChanged)
    {
        for (int32 Index = 0; Index < NewWorlds.Num(); ++Index)
        {
            bChanged |= NewWorlds[Index]->Context != Worlds[Index]->Context
                || NewWorlds[Index]->World != Worlds[Index]->World
                || NewWorlds[Index]->Label != Worlds[Index]->Label;
        }
    }
    if (bChanged)
    {
        Worlds = MoveTemp(NewWorlds);
        ++CatalogRevision;
    }
    if (bAutomaticWorld && PreferredContext != SelectedContext)
    {
        ChangeWorld(PreferredContext);
    }
    else if (!SelectedContext.IsNone())
    {
        const TSharedPtr<FKataGASInspectionWorld>* Found = Worlds.FindByPredicate([this](const auto& Item)
        {
            return Item->Context == SelectedContext;
        });
        if (!Found || (*Found)->World != SelectedWorld)
        {
            ChangeWorld(bAutomaticWorld ? PreferredContext : NAME_None);
        }
    }
}

void FKataGASInspectionSession::ChangeWorld(FName Context)
{
    SelectedContext = Context;
    SelectedWorld.Reset();
    for (const auto& Item : Worlds)
    {
        if (Item->Context == Context)
        {
            SelectedWorld = Item->World;
            break;
        }
    }
    SelectedASC.Reset();
    Targets.Reset();
    bChooseInitialTarget = true;
    LastRefresh = -1.0;
    if (!bFrozen)
    {
        Snapshot = FKataGASInspectionSnapshot();
        ++Revision;
    }
    RescanTargets();
}

void FKataGASInspectionSession::RescanTargets()
{
    TArray<TSharedPtr<FKataGASInspectionTarget>> NewTargets;
    if (SelectedWorld.IsValid())
    {
        for (TObjectIterator<UAbilitySystemComponent> It; It; ++It)
        {
            UAbilitySystemComponent* ASC = *It;
            if (!IsValid(ASC) || ASC->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
                || ASC->IsBeingDestroyed() || ASC->GetWorld() != SelectedWorld.Get())
            {
                continue;
            }
            AActor* Owner = ASC->GetOwner();
            if (!IsValid(Owner) || Owner->IsActorBeingDestroyed())
            {
                continue;
            }
            TSharedPtr<FKataGASInspectionTarget> Target = MakeShared<FKataGASInspectionTarget>();
            Target->ASC = ASC;
            Target->Label = FString::Printf(TEXT("%s | %s | %s"),
                *Owner->GetActorNameOrLabel(), *ASC->GetName(), *ASC->GetClass()->GetName());
            NewTargets.Add(Target);
        }
    }
    NewTargets.Sort([](const auto& A, const auto& B)
    {
        const int32 Compare = A->Label.Compare(B->Label);
        return Compare != 0 ? Compare < 0 : A->ASC->GetUniqueID() < B->ASC->GetUniqueID();
    });
    bool bChanged = NewTargets.Num() != Targets.Num();
    if (!bChanged)
    {
        for (int32 Index = 0; Index < NewTargets.Num(); ++Index)
        {
            bChanged |= NewTargets[Index]->ASC != Targets[Index]->ASC || NewTargets[Index]->Label != Targets[Index]->Label;
        }
    }
    if (bChanged)
    {
        Targets = MoveTemp(NewTargets);
        ++CatalogRevision;
    }
    if (bChooseInitialTarget && !Targets.IsEmpty())
    {
        SelectedASC = Targets[0]->ASC;
        bChooseInitialTarget = false;
    }
}

void FKataGASInspectionSession::SelectWorld(FName Context)
{
    bAutomaticWorld = false;
    bFrozen = false;
    ChangeWorld(Context);
    Refresh();
}

void FKataGASInspectionSession::SetAutomaticWorld(bool bAutomatic)
{
    bAutomaticWorld = bAutomatic;
    UpdateWorlds();
}

void FKataGASInspectionSession::SelectTarget(UAbilitySystemComponent* ASC)
{
    if (!IsValid(ASC) || ASC->GetWorld() != SelectedWorld.Get())
    {
        return;
    }
    SelectedASC = ASC;
    bChooseInitialTarget = false;
    // 핸들이 다른 대상에서 재사용될 수 있으므로 대상 변경은 행 재사용의 경계를 끊는다.
    Snapshot = FKataGASInspectionSnapshot();
    ++Revision;
    Refresh();
}

void FKataGASInspectionSession::Refresh()
{
    UpdateWorlds();
    UAbilitySystemComponent* ASC = SelectedASC.Get();
    if (!IsValid(ASC) || ASC->IsBeingDestroyed() || !SelectedWorld.IsValid()
        || ASC->GetWorld() != SelectedWorld.Get()
        || !IsValid(ASC->GetOwner()) || ASC->GetOwner()->IsActorBeingDestroyed())
    {
        Status = TEXT("Target unavailable. Select an ability system component.");
        if (!bFrozen && Snapshot.CapturedAt.GetTicks() != 0)
        {
            Snapshot = FKataGASInspectionSnapshot();
            ++Revision;
        }
        return;
    }
    FString WorldLabel;
    for (const auto& World : Worlds)
    {
        if (World->Context == SelectedContext)
        {
            WorldLabel = World->Label;
            break;
        }
    }
    MergeSnapshot(FKataGASSnapshotCollector::Collect(*ASC, WorldLabel));
    Status = Snapshot.Readiness;
}

void FKataGASInspectionSession::MergeSnapshot(FKataGASInspectionSnapshot&& NewSnapshot)
{
    TMap<FString, TSharedPtr<FKataGASInspectionRow>> Previous;
    KataGASSession::IndexRows(Snapshot.Rows, Previous);
    KataGASSession::ReuseRows(NewSnapshot.Rows, Previous);
    Snapshot = MoveTemp(NewSnapshot);
    ++Revision;
}
