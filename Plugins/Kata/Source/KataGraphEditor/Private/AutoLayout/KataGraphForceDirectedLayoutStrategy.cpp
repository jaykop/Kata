#include "AutoLayout/KataGraphForceDirectedLayoutStrategy.h"

UKataGraphForceDirectedLayoutStrategy::UKataGraphForceDirectedLayoutStrategy()
{
    bRandomInit = false;
    CoolDownRate = 10;
    InitTemperature = 10.f;
}

UKataGraphForceDirectedLayoutStrategy::~UKataGraphForceDirectedLayoutStrategy()
{
}

void UKataGraphForceDirectedLayoutStrategy::Layout(UEdGraph* SourceGraph)
{
    if (!BuildLayoutGraph(SourceGraph, false))
    {
        return;
    }
    if (Settings != nullptr)
    {
        OptimalDistance = FMath::Max(1, Settings->OptimalDistance);
        MaxIteration = Settings->MaxIteration;
        bRandomInit = Settings->bRandomInit;
    }

    // 여러 진입점이나 순환을 가진 연결 성분도 한 번만 옮긴다.
    TSet<UKataGraphNodeBase*> Placed;
    FBox2D PreviousBounds(ForceInitToZero);
    for (UKataGraphNodeBase* Root : LayoutRootNodes)
    {
        if (Placed.Contains(Root))
        {
            continue;
        }
        const TArray<UKataGraphNodeBase*> Component = CollectConnectedNodes(Root);
        for (UKataGraphNodeBase* Node : Component)
        {
            Placed.Add(Node);
        }
        PreviousBounds = LayoutOneTree(Root, PreviousBounds);
    }
}

FBox2D UKataGraphForceDirectedLayoutStrategy::LayoutOneTree(UKataGraphNodeBase* RootNode, const FBox2D& PreviousBounds)
{
    const TArray<UKataGraphNodeBase*> Component = CollectConnectedNodes(RootNode);
    FBox2D TargetBounds = GetActualBounds(RootNode);
    const double OffsetX = PreviousBounds.Max.X + OptimalDistance - TargetBounds.Min.X;
    TargetBounds.Min.X += OffsetX;
    TargetBounds.Max.X += OffsetX;
    if (bRandomInit)
    {
        RandomLayoutOneTree(RootNode, TargetBounds);
    }

    float Temperature = InitTemperature;
    TMap<UKataGraphNodeBase*, FVector2D> Displacements;
    for (int32 Iteration = 0; Iteration < MaxIteration; ++Iteration)
    {
        Displacements.Reset();
        for (UKataGraphNodeBase* Node : Component)
        {
            Displacements.Add(Node, FVector2D::ZeroVector);
        }
        for (UKataGraphNodeBase* Node : Component)
        {
            UKataEdNode* EdNode = LayoutNodeMap.FindChecked(Node);
            for (UKataGraphNodeBase* Other : Component)
            {
                if (Node == Other)
                {
                    continue;
                }
                UKataEdNode* OtherEdNode = LayoutNodeMap.FindChecked(Other);
                FVector2D Delta(double(EdNode->NodePosX) - OtherEdNode->NodePosX,
                    double(EdNode->NodePosY) - OtherEdNode->NodePosY);
                const double Distance = Delta.Size();
                if (Distance > UE_SMALL_NUMBER && Distance <= 2.0 * OptimalDistance)
                {
                    Displacements.FindChecked(Node) += Delta / Distance * (double(OptimalDistance) * OptimalDistance / Distance);
                }
            }
            for (UKataGraphNodeBase* Child : LayoutChildren.FindChecked(Node))
            {
                UKataEdNode* ChildEdNode = LayoutNodeMap.FindChecked(Child);
                FVector2D Delta(double(ChildEdNode->NodePosX) - EdNode->NodePosX,
                    double(ChildEdNode->NodePosY) - EdNode->NodePosY);
                const double Distance = Delta.Size();
                if (Distance > UE_SMALL_NUMBER)
                {
                    const FVector2D Force = Delta / Distance * Distance;
                    Displacements.FindChecked(Node) += Force;
                    Displacements.FindChecked(Child) -= Force;
                }
            }
        }
        for (UKataGraphNodeBase* Node : Component)
        {
            UKataEdNode* EdNode = LayoutNodeMap.FindChecked(Node);
            const FVector2D Delta = Displacements.FindChecked(Node);
            const double Distance = Delta.Size();
            if (Distance > UE_SMALL_NUMBER)
            {
                const FVector2D Movement = Delta / Distance * FMath::Min(Distance, double(Temperature));
                EdNode->NodePosX += Movement.X;
                EdNode->NodePosY += Movement.Y;
            }
        }
        Temperature = FMath::Max(0.01f, Temperature - Temperature / FMath::Max(1.f, CoolDownRate));
    }

    const FBox2D ActualBounds = GetActualBounds(RootNode);
    const FVector2D ActualSize = ActualBounds.GetSize();
    const FVector2D TargetSize = TargetBounds.GetSize();
    const FVector2D Scale(TargetSize.X / FMath::Max(1.0, ActualSize.X),
        TargetSize.Y / FMath::Max(1.0, ActualSize.Y));
    for (UKataGraphNodeBase* Node : Component)
    {
        UKataEdNode* EdNode = LayoutNodeMap.FindChecked(Node);
        EdNode->NodePosX = TargetBounds.GetCenter().X + Scale.X * (EdNode->NodePosX - ActualBounds.GetCenter().X);
        EdNode->NodePosY = TargetBounds.GetCenter().Y + Scale.Y * (EdNode->NodePosY - ActualBounds.GetCenter().Y);
    }
    return TargetBounds;
}
