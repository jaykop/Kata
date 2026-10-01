#include "Debug/GameplayDebuggerCategory_KataCamera.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "CanvasItem.h"
#include "GameFramework/PlayerController.h"
#include "KataCameraFeature.h"
#include "KataPlayerCameraManager.h"

FGameplayDebuggerCategory_KataCamera::FGameplayDebuggerCategory_KataCamera()
{
    // 카메라는 플레이어 단위라 디버그 대상 액터를 고르지 않아도 표시한다.
    bShowOnlyWithDebugActor = false;
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_KataCamera::MakeInstance()
{
    return MakeShareable(new FGameplayDebuggerCategory_KataCamera());
}

void FGameplayDebuggerCategory_KataCamera::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
    CollectedSnapshot = FKataCameraDebugSnapshot();
    const AKataPlayerCameraManager* CameraManager =
        OwnerPC != nullptr ? Cast<AKataPlayerCameraManager>(OwnerPC->PlayerCameraManager) : nullptr;
    if (CameraManager == nullptr)
    {
        AddTextLine(TEXT("{red}Player camera manager is not AKataPlayerCameraManager"));
        return;
    }

    const FKataCameraDebugSnapshot& Snapshot = CameraManager->GetDebugSnapshot();
    CollectedSnapshot = Snapshot;
    AddTextLine(FString::Printf(TEXT("{white}Camera Data: {yellow}%s"), *Snapshot.CameraDataName));

    if (!Snapshot.bPipelineActive)
    {
        AddTextLine(TEXT("{orange}Kata pipeline inactive (no pawn view target, camera data or placement). Using engine default camera."));
        return;
    }

    AddTextLine(FString::Printf(TEXT("{white}Placement: {yellow}%s"), *Snapshot.PlacementName));
    AddTextLine(FString::Printf(TEXT("{white}View Rotation: {yellow}P=%.1f Y=%.1f"), Snapshot.ViewRotation.Pitch, Snapshot.ViewRotation.Yaw));
    AddTextLine(FString::Printf(TEXT("{white}Distance: {yellow}%.1f  {white}FOV: {yellow}%.1f"),
        FVector::Dist(Snapshot.PivotLocation, Snapshot.CameraLocation), Snapshot.FieldOfView));

    if (!Snapshot.RailDiagnostic.IsEmpty())
    {
        AddTextLine(FString::Printf(TEXT("{white}Rail: {yellow}%s  %s%s"),
            *Snapshot.RailTag, Snapshot.bUsingSpline ? TEXT("{green}") : TEXT("{orange}"), *Snapshot.RailDiagnostic));
        if (Snapshot.bUsingSpline)
        {
            AddTextLine(FString::Printf(TEXT("{white}Rail Alpha: {yellow}%.3f  {white}Distance: {yellow}%.1f / %.1f cm"),
                Snapshot.RailAlpha, Snapshot.RailDistance, Snapshot.RailLength));
            AddTextLine(FString::Printf(TEXT("{white}Placement Offset: {yellow}X=%.1f Y=%.1f Z=%.1f"),
                Snapshot.OrbitOffset.X, Snapshot.OrbitOffset.Y, Snapshot.OrbitOffset.Z));
        }
        else
        {
            AddTextLine(TEXT("{orange}Using Boom Arm fallback with CameraData PivotOffset."));
        }
    }

    const TArray<TObjectPtr<UKataCameraFeature>>& Features = CameraManager->GetOrderedFeatures();
    if (Features.IsEmpty())
    {
        AddTextLine(TEXT("{white}Features: {grey}none"));
    }
    for (const UKataCameraFeature* Feature : Features)
    {
        if (Feature == nullptr)
        {
            continue;
        }

        const FString StageName = StaticEnum<EKataCameraStage>()->GetNameStringByValue(static_cast<int64>(Feature->GetStage()));
        const FString DebugString = Feature->GetDebugString();
        AddTextLine(FString::Printf(TEXT("{white}Feature [%s %d]: %s%s%s%s"),
            *StageName, Feature->GetPriority(), Feature->IsEnabled() ? TEXT("{green}") : TEXT("{grey}"), *Feature->GetClass()->GetName(),
            DebugString.IsEmpty() ? TEXT("") : TEXT("  {white}"), *DebugString));
    }

    AddShape(FGameplayDebuggerShape::MakePoint(Snapshot.PivotLocation, 8.0f, FColor::Yellow, TEXT("Pivot")));
}

void FGameplayDebuggerCategory_KataCamera::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
    const FKataCameraDebugSnapshot& Snapshot = CollectedSnapshot;
    if (!Snapshot.bPipelineActive || !Snapshot.bUsingSpline || Snapshot.RailSamples.Num() < 2)
    {
        return;
    }

    CanvasContext.Print(TEXT("{white}Rail XZ (yaw space, before Features): {yellow}pivot {green}camera {cyan}aim"));
    const FVector2D Origin(CanvasContext.CursorX, CanvasContext.CursorY);
    const FVector2D PanelSize(300.0, 170.0);
    constexpr double Padding = 18.0;

    FVector2D Min(0.0, 0.0);
    FVector2D Max(0.0, 0.0);
    const auto ExpandBounds = [&Min, &Max](const FVector& Point)
    {
        Min.X = FMath::Min(Min.X, Point.X);
        Min.Y = FMath::Min(Min.Y, Point.Z);
        Max.X = FMath::Max(Max.X, Point.X);
        Max.Y = FMath::Max(Max.Y, Point.Z);
    };
    for (const FVector& Point : Snapshot.RailSamples)
    {
        ExpandBounds(Point);
    }
    ExpandBounds(Snapshot.OrbitOffset);
    ExpandBounds(Snapshot.AimOffset);
    const FVector2D Extent = Max - Min;
    const double Scale = FMath::Min((PanelSize.X - Padding * 2.0) / FMath::Max(Extent.X, 1.0),
        (PanelSize.Y - Padding * 2.0) / FMath::Max(Extent.Y, 1.0));
    const FVector2D Center = (Min + Max) * 0.5;
    const auto Project = [Origin, PanelSize, Center, Scale](const FVector& Point)
    {
        return Origin + PanelSize * 0.5 + FVector2D((Point.X - Center.X) * Scale, -(Point.Z - Center.Y) * Scale);
    };
    const auto DrawLine = [&CanvasContext](const FVector2D& Start, const FVector2D& End, const FLinearColor& Color)
    {
        FCanvasLineItem Item(Start, End);
        Item.SetColor(Color);
        Item.LineThickness = 1.0f;
        CanvasContext.DrawItem(Item, static_cast<float>(Start.X), static_cast<float>(Start.Y));
    };
    const auto DrawMarker = [&DrawLine](const FVector2D& Position, const FLinearColor& Color)
    {
        DrawLine(Position - FVector2D(4.0, 0.0), Position + FVector2D(4.0, 0.0), Color);
        DrawLine(Position - FVector2D(0.0, 4.0), Position + FVector2D(0.0, 4.0), Color);
    };

    FCanvasTileItem Background(Origin, PanelSize, FLinearColor(0.02f, 0.02f, 0.02f, 0.8f));
    Background.BlendMode = SE_BLEND_Translucent;
    CanvasContext.DrawItem(Background, static_cast<float>(Origin.X), static_cast<float>(Origin.Y));
    FCanvasBoxItem Border(Origin, PanelSize);
    Border.SetColor(FLinearColor(0.4f, 0.4f, 0.4f));
    CanvasContext.DrawItem(Border, static_cast<float>(Origin.X), static_cast<float>(Origin.Y));

    for (int32 Index = 1; Index < Snapshot.RailSamples.Num(); ++Index)
    {
        DrawLine(Project(Snapshot.RailSamples[Index - 1]), Project(Snapshot.RailSamples[Index]), FLinearColor::White);
    }
    DrawMarker(Project(FVector::ZeroVector), FLinearColor::Yellow);
    DrawMarker(Project(Snapshot.AimOffset), FLinearColor(0.0f, 1.0f, 1.0f));
    DrawMarker(Project(Snapshot.OrbitOffset), FLinearColor::Green);
    const FVector2D Start = Project(Snapshot.RailSamples[0]);
    const FVector2D End = Project(Snapshot.RailSamples.Last());
    CanvasContext.PrintAt(static_cast<float>(Start.X) + 5.0f, static_cast<float>(Start.Y), TEXT("{grey}0"));
    CanvasContext.PrintAt(static_cast<float>(End.X) + 5.0f, static_cast<float>(End.Y), TEXT("{grey}1"));
    CanvasContext.PrintAt(static_cast<float>(Origin.X) + 4.0f, static_cast<float>(Origin.Y) + 2.0f, TEXT("{grey}+Z up / +X right"));
    CanvasContext.CursorY += static_cast<float>(PanelSize.Y) + CanvasContext.GetLineHeight();
}

#endif // WITH_GAMEPLAY_DEBUGGER
