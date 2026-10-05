#include "Debug/GameplayDebuggerCategory_KataCamera.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "CanvasItem.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/PlayerController.h"
#include "KataCameraData.h"
#include "KataCameraFeature.h"
#include "KataCameraPlacement.h"
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

    AddTextLine(Snapshot.StateTreeName == TEXT("None")
        ? FString(TEXT("{white}State Tree: {grey}none (DefaultCameraData)"))
        : FString::Printf(TEXT("{white}State Tree: {yellow}%s  %s"), *Snapshot.StateTreeName,
            Snapshot.bStateTreeRunning ? TEXT("{green}running") : TEXT("{orange}not running (DefaultCameraData)")));
    for (int32 Index = Snapshot.Layers.Num() - 1; Index >= 0; --Index)
    {
        const FKataCameraDebugLayer& Layer = Snapshot.Layers[Index];
        AddTextLine(FString::Printf(TEXT("{white}Layer %d: {yellow}%s  {white}w=%.2f  left=%.2fs%s"),
            Index, *Layer.CameraDataName, Layer.Weight, Layer.RemainingTime, Layer.bFrozen ? TEXT("  {orange}frozen") : TEXT("")));
    }

    AddTextLine(FString::Printf(TEXT("{white}Placement: {yellow}%s"), *Snapshot.PlacementName));
    AddTextLine(FString::Printf(TEXT("{white}View Rotation: {yellow}P=%.1f Y=%.1f  {white}Camera Pitch: {yellow}%.2f"),
        Snapshot.ViewRotation.Pitch, Snapshot.ViewRotation.Yaw, Snapshot.CameraRotation.Pitch));
    AddTextLine(FString::Printf(TEXT("{white}Distance: {yellow}%.1f  {white}FOV: {yellow}%.1f"),
        FVector::Dist(Snapshot.PivotLocation, Snapshot.CameraLocation), Snapshot.FieldOfView));

    // 에셋 값을 매 수집마다 다시 읽으므로 PIE 중 Details에서 바꾼 값이 바로 보인다.
    if (const UKataCameraData* Data = CameraManager->GetActiveCameraData())
    {
        AddTextLine(FString::Printf(TEXT("{white}Data: {yellow}FOV %.1f  {white}Pitch {yellow}%.1f ~ %.1f"),
            Data->FieldOfView, Data->PitchMin, Data->PitchMax));
        AddTextLine(FString::Printf(TEXT("{white}Pivot Lag: {yellow}H %.2fs  V %.2fs  Max %.0fcm  {white}offset {yellow}%.1fcm"),
            Data->PivotLagTimeHorizontal, Data->PivotLagTimeVertical, Data->PivotLagMaxDistance, Snapshot.PivotLagOffset.Size()));
        if (const UKataCameraPlacement_BoomArm* BoomArm = Cast<UKataCameraPlacement_BoomArm>(Data->Placement))
        {
            AddTextLine(FString::Printf(TEXT("{white}Boom Arm: {yellow}Distance %.1f  {white}Pivot Offset {yellow}X=%.1f Y=%.1f Z=%.1f"),
                BoomArm->Distance, BoomArm->PivotOffset.X, BoomArm->PivotOffset.Y, BoomArm->PivotOffset.Z));
        }
        else if (const UKataCameraPlacement_Spline* Spline = Cast<UKataCameraPlacement_Spline>(Data->Placement))
        {
            AddTextLine(FString::Printf(TEXT("{white}Spline Rail: {yellow}%s  {white}Fallback Distance {yellow}%.1f"),
                *Spline->RailTag.ToString(), Spline->FallbackDistance));
        }
    }

    if (Snapshot.bLockOnActive || Snapshot.LockOnWeight > 0.0f)
    {
        const FKataLockOnFramingSettings& Settings = CameraManager->GetLockOnSettings();
        AddTextLine(FString::Printf(TEXT("{white}Lock On: %s  {white}source {yellow}%s  {white}w={yellow}%.2f  {white}side={yellow}%.2f"),
            Snapshot.bLockOnActive ? TEXT("{green}active") : TEXT("{orange}releasing"),
            Snapshot.LockOnDataName.IsEmpty() ? TEXT("Default Lock On Settings") : *Snapshot.LockOnDataName,
            Snapshot.LockOnWeight, Snapshot.LockOnSide));
        AddTextLine(FString::Printf(TEXT("{white}  Align {yellow}%s  {white}Side Offset {yellow}%.0fcm  {white}Screen {yellow}(%.2f, %.2f)  {white}Look At {yellow}%.2f"),
            *StaticEnum<EKataLockOnAlignment>()->GetNameStringByValue(static_cast<int64>(Settings.Alignment)),
            Settings.SideOffset, Settings.TargetScreenPosition.X, Settings.TargetScreenPosition.Y, Settings.LookAtAlpha));
        AddTextLine(FString::Printf(TEXT("{white}  Rotation Lag {yellow}%.2fs  {white}Blend In {yellow}%.2fs %s"),
            Settings.RotationLagTime, Settings.BlendInDuration,
            Settings.BlendInCurve != nullptr ? *Settings.BlendInCurve->GetName() : TEXT("(EaseInOut)")));
        AddTextLine(FString::Printf(TEXT("{white}  By Distance: {yellow}Pitch %+.1f deg  {white}Boom Scale {yellow}%.2f"),
            Snapshot.LockOnPitchOffset, Snapshot.LockOnDistanceScale));
        AddTextLine(FString::Printf(TEXT("{white}  Aim Line: {yellow}t=%.2f  {white}pivot-focus {yellow}%.0fcm  {white}pivot-aim {yellow}%.0fcm"),
            Snapshot.LockOnLookAtAlpha, FVector::Dist(Snapshot.LockOnLineStart, Snapshot.LockOnLineEnd),
            FVector::Dist(Snapshot.LockOnLineStart, Snapshot.LockOnAimPoint)));

        // 조준선: 피벗(노랑) → 락온 초점(빨강) 선과 화면 위치에 맞춘 조준점(청록). Max Look Distance로 잘리면 조준점이 선 위에서 피벗 쪽으로 당겨진다.
        // 해제 블렌드 중에는 조준점을 새로 구하지 않으므로 그리지 않는다.
        if (Snapshot.bLockOnActive)
        {
            AddShape(FGameplayDebuggerShape::MakeSegment(Snapshot.LockOnLineStart, Snapshot.LockOnLineEnd, 2.0f, FColor::Orange));
            AddShape(FGameplayDebuggerShape::MakePoint(Snapshot.LockOnLineEnd, 10.0f, FColor::Red, TEXT("Lock Focus")));
            AddShape(FGameplayDebuggerShape::MakePoint(Snapshot.LockOnAimPoint, 10.0f, FColor::Cyan,
                FString::Printf(TEXT("Aim t=%.2f"), Snapshot.LockOnLookAtAlpha)));
        }
    }
    else
    {
        AddTextLine(TEXT("{white}Lock On: {grey}none"));
    }

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
            AddTextLine(TEXT("{orange}Using Boom Arm fallback at the view target location."));
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
