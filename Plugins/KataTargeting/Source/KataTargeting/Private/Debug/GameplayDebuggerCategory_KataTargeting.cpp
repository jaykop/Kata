#include "Debug/GameplayDebuggerCategory_KataTargeting.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Targeting/KataPlayerTargetingComponent.h"
#include "Targeting/KataTargetPointComponent.h"
#include "Targeting/Tasks/KataTargetingFilterTask_ForwardAngle.h"
#include "Tasks/TargetingSelectionTask_AOE.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Types/TargetingSystemTypes.h"

namespace
{
    FString DescribePoint(const UKataTargetPointComponent* Point)
    {
        return FString::Printf(TEXT("%s.%s"), *GetNameSafe(Point->GetOwner()), *Point->GetName());
    }

    /** 소프트 타겟 범위와 후보는 노랑으로 그려 Kata.Targeting.Debug의 소프트 타겟 표시와 맞춘다. */
    const FColor SoftTargetColor = FColor::Yellow;
}

void FGameplayDebuggerCategory_KataTargeting::CollectSoftTargetRange(const UKataPlayerTargetingComponent& Targeting)
{
    const UTargetingPreset* Preset = Targeting.SoftTargetPreset;
    AActor* Owner = Targeting.GetOwner();
    const FTargetingTaskSet* TaskSet = Preset != nullptr ? Preset->GetTargetingTaskSet() : nullptr;
    if (TaskSet == nullptr || Owner == nullptr)
    {
        AddTextLine(TEXT("{white}Soft Target Range: {grey}none (Soft Target Preset is empty)"));
        return;
    }

    // AOE 태스크의 원점·회전은 요청 핸들의 소스 컨텍스트에서 읽으므로 실행 없이 핸들만 만든다.
    // 컨텍스트는 UKataTargetingComponent::FindTargets()와 같게 맞춰야 실제 수집 범위와 일치한다.
    FTargetingSourceContext SourceContext;
    SourceContext.SourceActor = Owner;
    SourceContext.InstigatorActor = Owner;
    SourceContext.SourceLocation = Owner->GetActorLocation();
    FTargetingRequestHandle Handle = UTargetingSubsystem::MakeTargetRequestHandle(Preset, SourceContext);

    int32 NumRanges = 0;
    // 각도 필터 부채꼴을 AOE 범위 끝까지 그리려고 수평 도달 거리 중 가장 큰 값을 모은다.
    double RangeLength = 0.0;
    for (const UTargetingTask* Task : TaskSet->Tasks)
    {
        const UTargetingSelectionTask_AOE* AOETask = Cast<UTargetingSelectionTask_AOE>(Task);
        if (AOETask == nullptr)
        {
            continue;
        }

        // 엔진 UTargetingSelectionTask_AOE::DebugDrawBoundingVolume()과 같은 방식으로 원점과 회전을 구한다.
        const FVector Location = AOETask->GetSourceLocation(Handle) + AOETask->GetSourceOffset(Handle);
        const FRotator Rotation = (AOETask->GetSourceRotation(Handle) * AOETask->GetSourceRotationOffset(Handle).Quaternion()).Rotator();
        const FCollisionShape Shape = AOETask->GetCollisionShape();
        const FString Label = FString::Printf(TEXT("Soft Range %s"), *AOETask->GetName());

        switch (AOETask->GetShapeType())
        {
        case ETargetingAOEShape::Box:
            AddShape(FGameplayDebuggerShape::MakeBox(Location, Rotation, Shape.GetExtent(), SoftTargetColor, Label));
            RangeLength = FMath::Max(RangeLength, FMath::Max(Shape.GetExtent().X, Shape.GetExtent().Y));
            break;
        case ETargetingAOEShape::Sphere:
            AddShape(FGameplayDebuggerShape::MakeCapsule(Location, Rotation, Shape.GetSphereRadius(), Shape.GetSphereRadius(), SoftTargetColor, Label));
            RangeLength = FMath::Max(RangeLength, static_cast<double>(Shape.GetSphereRadius()));
            break;
        case ETargetingAOEShape::Capsule:
            AddShape(FGameplayDebuggerShape::MakeCapsule(Location, Rotation, Shape.GetCapsuleRadius(), Shape.GetCapsuleHalfHeight(), SoftTargetColor, Label));
            RangeLength = FMath::Max(RangeLength, static_cast<double>(Shape.GetCapsuleRadius()));
            break;
        case ETargetingAOEShape::Cylinder:
            // GameplayDebugger의 원기둥은 회전을 받지 않으므로 월드 Z축 기준으로 그린다. 반지름은 Half Extent X, 반높이는 Z다.
            AddShape(FGameplayDebuggerShape::MakeCylinder(Location, Shape.GetExtent().X, Shape.GetExtent().Z, SoftTargetColor, Label));
            RangeLength = FMath::Max(RangeLength, Shape.GetExtent().X);
            break;
        default:
            // SourceComponent 형태는 소유 액터의 컴포넌트 충돌체를 그대로 쓰므로 여기서 모양을 다시 만들지 않는다.
            AddTextLine(FString::Printf(TEXT("{white}Soft Target Range: {grey}%s uses Source Component shape (not drawn)"), *AOETask->GetName()));
            break;
        }
        ++NumRanges;
    }
    UTargetingSubsystem::ReleaseTargetRequestHandle(Handle);

    if (NumRanges == 0)
    {
        AddTextLine(TEXT("{white}Soft Target Range: {grey}none (Soft Target Preset has no AOE selection task)"));
    }

    // AOE가 없거나 Source Component 형태뿐이면 부채꼴 방향만 보이도록 기본 길이로 그린다.
    constexpr double DefaultFanLength = 500.0;
    const double FanLength = RangeLength > 0.0 ? RangeLength : DefaultFanLength;
    for (const UTargetingTask* Task : TaskSet->Tasks)
    {
        if (const UKataTargetingFilterTask_ForwardAngle* AngleTask = Cast<UKataTargetingFilterTask_ForwardAngle>(Task))
        {
            CollectForwardAngleFan(*Owner, AngleTask->GetMaxAngle(), FanLength,
                FString::Printf(TEXT("Soft Angle %s"), *AngleTask->GetName()));
        }
    }
}

void FGameplayDebuggerCategory_KataTargeting::CollectForwardAngleFan(const AActor& Owner, float MaxAngle, double Length, const FString& Label)
{
    // 필터와 같은 기준을 쓴다. 액터 정면을 수평면에 투영하고 그 위에서 좌우로 MaxAngle만큼 벌린다.
    const FVector Forward = Owner.GetActorForwardVector().GetSafeNormal2D();
    if (Forward.IsZero())
    {
        return;
    }
    if (MaxAngle >= 180.0f)
    {
        AddTextLine(FString::Printf(TEXT("{white}%s: {grey}180 deg (no filtering)"), *Label));
        return;
    }

    const FVector Origin = Owner.GetActorLocation();
    constexpr int32 NumArcSegments = 16;
    TArray<FVector, TInlineAllocator<NumArcSegments + 1>> ArcPoints;
    for (int32 Index = 0; Index <= NumArcSegments; ++Index)
    {
        const double Angle = FMath::Lerp(-static_cast<double>(MaxAngle), static_cast<double>(MaxAngle), static_cast<double>(Index) / NumArcSegments);
        ArcPoints.Add(Origin + Forward.RotateAngleAxis(Angle, FVector::UpVector) * Length);
    }

    AddShape(FGameplayDebuggerShape::MakeSegment(Origin, ArcPoints[0], SoftTargetColor));
    AddShape(FGameplayDebuggerShape::MakeSegment(Origin, ArcPoints.Last(), SoftTargetColor));
    AddShape(FGameplayDebuggerShape::MakePolyline(ArcPoints, SoftTargetColor,
        FString::Printf(TEXT("%s (+-%.0f)"), *Label, MaxAngle)));
}

void FGameplayDebuggerCategory_KataTargeting::CollectSoftTargetCandidates(const UKataPlayerTargetingComponent& Targeting)
{
    TArray<AActor*> Candidates;
    Targeting.GetSoftTargetCandidates(Candidates);
    if (Candidates.IsEmpty())
    {
        AddTextLine(TEXT("{white}Soft Target Candidates: {grey}none"));
        return;
    }
    for (int32 Index = 0; Index < Candidates.Num(); ++Index)
    {
        const AActor* Candidate = Candidates[Index];
        AddTextLine(FString::Printf(TEXT("{white}Soft Candidate #%d: {yellow}%s"), Index + 1, *GetNameSafe(Candidate)));
        AddShape(FGameplayDebuggerShape::MakePoint(Candidate->GetActorLocation(), 10.0f, SoftTargetColor, FString::Printf(TEXT("S#%d"), Index + 1)));
    }
}

FGameplayDebuggerCategory_KataTargeting::FGameplayDebuggerCategory_KataTargeting()
{
    // 락온은 플레이어 단위라 디버그 대상 액터를 고르지 않아도 표시한다.
    bShowOnlyWithDebugActor = false;
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_KataTargeting::MakeInstance()
{
    return MakeShareable(new FGameplayDebuggerCategory_KataTargeting());
}

void FGameplayDebuggerCategory_KataTargeting::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
    const APawn* Pawn = OwnerPC != nullptr ? OwnerPC->GetPawn() : nullptr;
    const UKataPlayerTargetingComponent* Targeting = Pawn != nullptr ? Pawn->FindComponentByClass<UKataPlayerTargetingComponent>() : nullptr;
    if (Targeting == nullptr)
    {
        AddTextLine(TEXT("{red}Player pawn has no UKataPlayerTargetingComponent"));
        return;
    }

    const UKataTargetPointComponent* LockPoint = Targeting->GetLockPoint();
    AddTextLine(LockPoint != nullptr
        ? FString::Printf(TEXT("{white}Lock Point: {red}%s"), *DescribePoint(LockPoint))
        : FString(TEXT("{white}Lock Point: {grey}none")));
    AddTextLine(FString::Printf(TEXT("{white}Soft Target: {yellow}%s"), *GetNameSafe(Targeting->GetSoftTarget())));
    CollectSoftTargetRange(*Targeting);
    CollectSoftTargetCandidates(*Targeting);

    TArray<UKataTargetPointComponent*> Candidates;
    Targeting->GetLockOnCandidates(Candidates);
    if (Candidates.IsEmpty())
    {
        AddTextLine(TEXT("{white}Lock-On Candidates: {grey}none (check Lock On Preset and Kata Expand Target Points)"));
    }
    for (int32 Index = 0; Index < Candidates.Num(); ++Index)
    {
        const UKataTargetPointComponent* Point = Candidates[Index];
        const bool bIsLockPoint = Point == LockPoint;
        AddTextLine(FString::Printf(TEXT("{white}Candidate #%d: %s%s"), Index + 1, bIsLockPoint ? TEXT("{red}") : TEXT("{green}"), *DescribePoint(Point)));
        AddShape(FGameplayDebuggerShape::MakePoint(Point->GetComponentLocation(), 10.0f,
            bIsLockPoint ? FColor::Red : FColor::Green, FString::Printf(TEXT("#%d"), Index + 1)));
    }

    // 후보에서 빠졌지만 아직 잡고 있는 지점(거리 초과 직전 등)도 위치를 보인다.
    if (LockPoint != nullptr && !Candidates.Contains(LockPoint))
    {
        AddShape(FGameplayDebuggerShape::MakePoint(LockPoint->GetComponentLocation(), 10.0f, FColor::Red, TEXT("Lock")));
    }
}

#endif // WITH_GAMEPLAY_DEBUGGER
