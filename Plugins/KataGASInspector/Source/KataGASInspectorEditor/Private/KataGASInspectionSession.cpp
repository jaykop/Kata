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

    // 화면에 영향을 주는 값만 비교한다. VisibleChildren은 화면이 매번 다시 계산하므로 제외한다.
    bool SameRows(const TArray<TSharedPtr<FKataGASInspectionRow>>& A, const TArray<TSharedPtr<FKataGASInspectionRow>>& B)
    {
        if (A.Num() != B.Num())
        {
            return false;
        }
        for (int32 Index = 0; Index < A.Num(); ++Index)
        {
            const FKataGASInspectionRow& Left = *A[Index];
            const FKataGASInspectionRow& Right = *B[Index];
            if (Left.Key != Right.Key || Left.Page != Right.Page || Left.Name != Right.Name
                || Left.State != Right.State || Left.Value != Right.Value || Left.Tags != Right.Tags
                || Left.Source != Right.Source || Left.Detail != Right.Detail || Left.AssetPath != Right.AssetPath
                || Left.bActive != Right.bActive || Left.bBlocked != Right.bBlocked
                || !SameRows(Left.Children, Right.Children))
            {
                return false;
            }
        }
        return true;
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
            Target->Label = Owner->GetActorNameOrLabel();
            NewTargets.Add(Target);
        }
    }
    // 목록은 캐릭터 이름만 보여 준다. 한 액터가 ASC를 여러 개 가진 경우에만 컴포넌트 이름으로 구분한다.
    TMap<FString, int32> LabelCounts;
    for (const auto& Target : NewTargets)
    {
        ++LabelCounts.FindOrAdd(Target->Label);
    }
    for (const auto& Target : NewTargets)
    {
        if (LabelCounts[Target->Label] > 1)
        {
            Target->Label += FString::Printf(TEXT(" (%s)"), *Target->ASC->GetName());
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
    // Frozen은 당시 값을 유지해야 하므로 대상만 바꾸고 화면은 Previous target으로 안내한다.
    // 새 값은 Refresh 또는 Resume에서 읽으며, 그때 MergeSnapshot이 대상 경계에서 행 재사용을 끊는다.
    if (bFrozen)
    {
        return;
    }
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
    // 핸들 문자열은 다른 대상에서 재사용될 수 있으므로 대상이 바뀌면 이전 행을 재사용하지 않는다.
    if (NewSnapshot.TargetPath != Snapshot.TargetPath)
    {
        Snapshot = MoveTemp(NewSnapshot);
        ++Revision;
        return;
    }
    // 표시 값이 그대로면 수집 시각과 준비 상태만 갱신한다. 화면의 정렬·필터 재구성은 revision 변경 시에만 일어난다.
    if (NewSnapshot.World == Snapshot.World && NewSnapshot.Target == Snapshot.Target
        && NewSnapshot.bActorInfoReady == Snapshot.bActorInfoReady
        && KataGASSession::SameRows(NewSnapshot.Rows, Snapshot.Rows))
    {
        Snapshot.CapturedAt = NewSnapshot.CapturedAt;
        Snapshot.Readiness = MoveTemp(NewSnapshot.Readiness);
        return;
    }
    TMap<FString, TSharedPtr<FKataGASInspectionRow>> Previous;
    KataGASSession::IndexRows(Snapshot.Rows, Previous);
    KataGASSession::ReuseRows(NewSnapshot.Rows, Previous);
    Snapshot = MoveTemp(NewSnapshot);
    ++Revision;
}
