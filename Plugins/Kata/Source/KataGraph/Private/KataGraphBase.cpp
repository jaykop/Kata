#include "KataGraphBase.h"

#include "Engine/Engine.h"
#include "KataRuntimeLog.h"

namespace
{
    /**
     * 다음 단계의 노드를 모은다. 이미 지난 노드는 건너뛴다.
     *
     * 콤보 그래프는 중립 상태로 돌아오는 순환을 허용하므로, 방문 기록이 없으면
     * 단계 순회가 끝나지 않는다.
     */
    void CollectNextLevel(const TArray<TObjectPtr<UKataGraphNodeBase>>& CurrentLevel,
        TSet<const UKataGraphNodeBase*>& Visited, TArray<TObjectPtr<UKataGraphNodeBase>>& OutNextLevel)
    {
        OutNextLevel.Reset();
        for (const TObjectPtr<UKataGraphNodeBase>& Node : CurrentLevel)
        {
            if (Node == nullptr)
            {
                continue;
            }
            for (const TObjectPtr<UKataGraphNodeBase>& Child : Node->ChildrenNodes)
            {
                if (Child == nullptr || Visited.Contains(Child))
                {
                    continue;
                }
                Visited.Add(Child);
                OutNextLevel.Add(Child);
            }
        }
    }
}

UKataGraphBase::UKataGraphBase()
{
    NodeType = UKataGraphNodeBase::StaticClass();
    EdgeType = UKataGraphEdgeBase::StaticClass();

    bEdgeEnabled = true;

#if WITH_EDITORONLY_DATA
    EdGraph = nullptr;
    bCanRenameNode = true;
    bCanBeCyclical = true;
#endif
}

void UKataGraphBase::Print(bool bToConsole, bool bToScreen) const
{
    int32 Level = 0;
    TSet<const UKataGraphNodeBase*> Visited;
    TArray<TObjectPtr<UKataGraphNodeBase>> CurrentLevel = RootNodes;
    TArray<TObjectPtr<UKataGraphNodeBase>> NextLevel;

    for (const TObjectPtr<UKataGraphNodeBase>& Node : CurrentLevel)
    {
        Visited.Add(Node);
    }

    while (CurrentLevel.Num() > 0)
    {
        for (const TObjectPtr<UKataGraphNodeBase>& Node : CurrentLevel)
        {
            if (Node == nullptr)
            {
                continue;
            }

            const FString Message = FString::Printf(TEXT("%s, Level %d"), *Node->GetDescription().ToString(), Level);

            if (bToConsole)
            {
                UE_LOG(LogKata, Log, TEXT("%s"), *Message);
            }
            if (bToScreen && GEngine != nullptr)
            {
                GEngine->AddOnScreenDebugMessage(INDEX_NONE, 15.0f, FColor::Blue, Message);
            }
        }

        CollectNextLevel(CurrentLevel, Visited, NextLevel);
        CurrentLevel = NextLevel;
        ++Level;
    }
}

int32 UKataGraphBase::GetLevelNum() const
{
    int32 Level = 0;
    TSet<const UKataGraphNodeBase*> Visited;
    TArray<TObjectPtr<UKataGraphNodeBase>> CurrentLevel = RootNodes;
    TArray<TObjectPtr<UKataGraphNodeBase>> NextLevel;

    for (const TObjectPtr<UKataGraphNodeBase>& Node : CurrentLevel)
    {
        Visited.Add(Node);
    }

    while (CurrentLevel.Num() > 0)
    {
        CollectNextLevel(CurrentLevel, Visited, NextLevel);
        CurrentLevel = NextLevel;
        ++Level;
    }

    return Level;
}

void UKataGraphBase::GetNodesByLevel(int32 Level, TArray<UKataGraphNodeBase*>& OutNodes) const
{
    OutNodes.Reset();

    int32 CurrentLevelIndex = 0;
    TSet<const UKataGraphNodeBase*> Visited;
    TArray<TObjectPtr<UKataGraphNodeBase>> CurrentLevel = RootNodes;
    TArray<TObjectPtr<UKataGraphNodeBase>> NextLevel;

    for (const TObjectPtr<UKataGraphNodeBase>& Node : CurrentLevel)
    {
        Visited.Add(Node);
    }

    while (CurrentLevel.Num() > 0 && CurrentLevelIndex < Level)
    {
        CollectNextLevel(CurrentLevel, Visited, NextLevel);
        CurrentLevel = NextLevel;
        ++CurrentLevelIndex;
    }

    if (CurrentLevelIndex == Level)
    {
        OutNodes.Reserve(CurrentLevel.Num());
        for (const TObjectPtr<UKataGraphNodeBase>& Node : CurrentLevel)
        {
            if (Node != nullptr)
            {
                OutNodes.Add(Node);
            }
        }
    }
}

void UKataGraphBase::ClearGraph()
{
    for (const TObjectPtr<UKataGraphNodeBase>& Node : AllNodes)
    {
        if (Node != nullptr)
        {
            Node->ParentNodes.Empty();
            Node->ChildrenNodes.Empty();
            Node->Edges.Empty();
        }
    }

    AllNodes.Empty();
    RootNodes.Empty();
}
