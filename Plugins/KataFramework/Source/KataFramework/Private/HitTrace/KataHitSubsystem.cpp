#include "HitTrace/KataHitSubsystem.h"

#include "AbilitySystemComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "HitTrace/KataHitBoxComponent.h"
#include "HitTrace/KataHitBoxPreset.h"
#include "HitTrace/KataHitGeometry.h"
#include "HitTrace/KataHitHandler.h"
#include "KataFrameworkLog.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/BodySetup.h"
#include "Runtime/KataActionInstance.h"
#include "Runtime/KataTaskInstance.h"
#include "Tasks/KataTask_HitTrace.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Tasks/TargetingTask.h"
#include "Types/TargetingSystemTypes.h"

#if ENABLE_DRAW_DEBUG
static TAutoConsoleVariable<int32> CVarKataHitTraceDebug(
    TEXT("Kata.HitTrace.Debug"),
    0,
    TEXT("Draws Kata Hit Trace areas. 0: off, 1: current hit area, 2: area with substep trails, start/end checks and hit points. ")
    TEXT("Worlds with their own setting (such as the action editor preview toggle) ignore this value."),
    ECVF_Cheat);
#endif

/** 판정이 찾은 후보. 대상마다 가장 이른 히트 하나만 남긴다. */
struct UKataHitSubsystem::FHitCandidate
{
    TWeakObjectPtr<AActor> Actor;
    FHitResult Hit;
    /** 시작 시점 판정은 -1, 구간 판정은 서브스텝 번호. 작을수록 먼저 일어난 히트다. */
    int32 Order = 0;
};

namespace KataHitTrace
{
    /** ShapeSweep 한 추적에서 Block 반응 대상을 제외하고 다시 추적하는 최대 횟수. 한 궤적에 여러 대상이 겹칠 때의 비용 상한이다. */
    constexpr int32 MaxRetraceCount = 8;

    /** Detailed 단계의 궤적과, 모든 단계의 히트 표시를 남겨 두는 시간(초). 한 프레임만 그리면 히트를 알아보기 어렵다. */
    constexpr float DetailedDrawLifetime = 1.0f;

    /** 소켓 순서대로 담은 한 시점의 소켓 월드 트랜스폼. */
    using FSocketPose = TArray<FTransform, TInlineAllocator<8>>;

    /** SocketTrace 판정 면을 이루는 삼각형. Order는 시작 시점 판정이 -1, 구간 판정은 서브스텝 번호다. */
    struct FTriangle
    {
        FVector A;
        FVector B;
        FVector C;
        int32 Order = 0;
    };

    /** 두 소켓 트랜스폼 사이를 위치는 Lerp, 회전은 Slerp로 보간한다. 판정에 스케일은 쓰지 않는다. */
    FTransform BlendTransform(const FTransform& From, const FTransform& To, float Alpha)
    {
        return FTransform(
            FQuat::Slerp(From.GetRotation(), To.GetRotation(), Alpha).GetNormalized(),
            FMath::Lerp(From.GetLocation(), To.GetLocation(), Alpha));
    }

    FSocketPose BlendPose(const FSocketPose& From, const FSocketPose& To, float Alpha)
    {
        FSocketPose Result;
        Result.SetNum(From.Num());
        for (int32 Index = 0; Index < From.Num(); ++Index)
        {
            Result[Index] = BlendTransform(From[Index], To[Index], Alpha);
        }
        return Result;
    }

    /**
     * 칼날이 From 포즈에서 To 포즈로 움직이며 쓸고 간 면을 삼각형 띠로 만든다.
     * 이웃한 두 소켓 i, i+1마다 사각형 (From_i, From_i+1, To_i+1, To_i)를 두 삼각형으로 나눈다.
     */
    void AppendSweptStrip(const FSocketPose& From, const FSocketPose& To, int32 Order, TArray<FTriangle>& OutTriangles)
    {
        for (int32 Index = 0; Index + 1 < From.Num(); ++Index)
        {
            const FVector FromA = From[Index].GetLocation();
            const FVector FromB = From[Index + 1].GetLocation();
            const FVector ToA = To[Index].GetLocation();
            const FVector ToB = To[Index + 1].GetLocation();
            OutTriangles.Add({ FromA, FromB, ToB, Order });
            OutTriangles.Add({ FromA, ToB, ToA, Order });
        }
    }

    /** 한 포즈의 칼날 자체를 넓이 없는 삼각형(선분)들로 만든다. 시작 시점 판정에 쓴다. */
    void AppendBlade(const FSocketPose& Pose, int32 Order, TArray<FTriangle>& OutTriangles)
    {
        for (int32 Index = 0; Index + 1 < Pose.Num(); ++Index)
        {
            const FVector Start = Pose[Index].GetLocation();
            const FVector End = Pose[Index + 1].GetLocation();
            OutTriangles.Add({ Start, End, End, Order });
        }
    }

    /** 좁은 단계에서 찾은 히트와 그 히트를 만든 삼각형. */
    struct FTriangleHit
    {
        FHitResult Hit;
        int32 Order = 0;
        int32 TriangleIndex = INDEX_NONE;
    };

    /**
     * 삼각형 목록과 닿는 대상을 찾는다.
     * 넓은 단계는 모든 삼각형을 감싸는 상자로 채널 Overlap 질의를 한 번 하고,
     * 좁은 단계는 후보 컴포넌트의 도형과 삼각형을 순서대로 교차 계산해 컴포넌트마다 가장 이른 접점 하나를 돌려준다.
     */
    void QueryTriangles(const UWorld& World, const TArray<FTriangle>& Triangles, float Inflate, ECollisionChannel Channel,
        const FCollisionQueryParams& Params, TArray<FTriangleHit>& OutHits)
    {
        if (Triangles.IsEmpty())
        {
            return;
        }

        FBox Bounds(ForceInit);
        for (const FTriangle& Triangle : Triangles)
        {
            Bounds += Triangle.A;
            Bounds += Triangle.B;
            Bounds += Triangle.C;
        }
        // 넓이 없는 칼날도 상자가 납작해지지 않도록 두께에 여유를 더해 부풀린다.
        Bounds = Bounds.ExpandBy(Inflate + 1.0f);

        TArray<FOverlapResult> Overlaps;
        World.OverlapMultiByChannel(Overlaps, Bounds.GetCenter(), FQuat::Identity, Channel, FCollisionShape::MakeBox(Bounds.GetExtent()), Params);

        TSet<const UPrimitiveComponent*> VisitedComponents;
        TArray<KataHitGeometry::FShape> Shapes;
        for (const FOverlapResult& Overlap : Overlaps)
        {
            UPrimitiveComponent* Component = Overlap.GetComponent();
            AActor* HitActor = Overlap.GetActor();
            if (Component == nullptr || HitActor == nullptr)
            {
                continue;
            }
            // 스켈레탈 메시는 바디마다 겹침 결과가 따로 온다. 도형은 컴포넌트 단위로 한 번만 꺼낸다.
            bool bAlreadyVisited = false;
            VisitedComponents.Add(Component, &bAlreadyVisited);
            if (bAlreadyVisited)
            {
                continue;
            }

            Shapes.Reset();
            KataHitGeometry::CollectShapes(*Component, Shapes);

            bool bFound = false;
            for (int32 TriangleIndex = 0; TriangleIndex < Triangles.Num() && !bFound; ++TriangleIndex)
            {
                const FTriangle& Triangle = Triangles[TriangleIndex];
                for (const KataHitGeometry::FShape& Shape : Shapes)
                {
                    FVector Contact;
                    if (!KataHitGeometry::IntersectTriangle(Triangle.A, Triangle.B, Triangle.C, Shape, Inflate, Contact))
                    {
                        continue;
                    }
                    FHitResult Hit(HitActor, Component, Contact, (Contact - Shape.Center).GetSafeNormal());
                    Hit.bBlockingHit = true;
                    Hit.BoneName = Shape.BoneName;
                    Hit.Item = Shape.BodyIndex;
                    Hit.TraceStart = Contact;
                    Hit.TraceEnd = Contact;
                    // 같은 서브스텝 안에서는 삼각형 순서가 칼날이 먼저 지나간 순서에 가깝다.
                    Hit.Time = static_cast<float>(TriangleIndex) / static_cast<float>(Triangles.Num());
                    OutHits.Add({ Hit, Triangle.Order, TriangleIndex });
                    bFound = true;
                    break;
                }
            }
        }
    }

    /** ShapeSweep 선분 하나를 추적한다. Block으로 끝났으면 그 액터를 제외하고 다시 추적해 궤적 위의 대상을 모두 찾는다. */
    void SweepSegment(const UWorld& World, const FVector& Start, const FVector& End, const FQuat& Rotation,
        const FCollisionShape& Shape, ECollisionChannel Channel, FCollisionQueryParams Params, TArray<FHitResult>& OutHits)
    {
        for (int32 Attempt = 0; Attempt < MaxRetraceCount; ++Attempt)
        {
            TArray<FHitResult> Hits;
            World.SweepMultiByChannel(Hits, Start, End, Rotation, Channel, Shape, Params);

            bool bBlocked = false;
            for (const FHitResult& Hit : Hits)
            {
                AActor* HitActor = Hit.GetActor();
                if (HitActor == nullptr)
                {
                    continue;
                }
                OutHits.Add(Hit);
                if (Hit.bBlockingHit)
                {
                    Params.AddIgnoredActor(HitActor);
                    bBlocked = true;
                }
            }
            if (!bBlocked)
            {
                break;
            }
        }
    }

    /** 스켈레탈 메시의 겹침 결과에서 부위를 구분할 본 이름을 찾는다. */
    FName FindBoneName(const UPrimitiveComponent* Component, int32 ItemIndex)
    {
        const USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(Component);
        if (SkeletalMesh != nullptr && SkeletalMesh->Bodies.IsValidIndex(ItemIndex))
        {
            if (const FBodyInstance* Body = SkeletalMesh->Bodies[ItemIndex])
            {
                if (const UBodySetup* BodySetup = Body->GetBodySetup())
                {
                    return BodySetup->BoneName;
                }
            }
        }
        return NAME_None;
    }

#if ENABLE_DRAW_DEBUG
    void DrawPose(const UWorld& World, const UKataHitBoxPreset& Preset, const FSocketPose& Pose, const FColor& Color, float Lifetime)
    {
        const bool bPersistent = false;
        if (Preset.Mode == EKataHitBoxMode::SocketTrace)
        {
            const float PointRadius = FMath::Max(Preset.Thickness, 1.5f);
            for (int32 Index = 0; Index < Pose.Num(); ++Index)
            {
                DrawDebugSphere(&World, Pose[Index].GetLocation(), PointRadius, 6, Color, bPersistent, Lifetime);
                if (Index + 1 < Pose.Num())
                {
                    DrawDebugLine(&World, Pose[Index].GetLocation(), Pose[Index + 1].GetLocation(), Color, bPersistent, Lifetime, 0, 1.5f);
                }
            }
            return;
        }

        const FTransform ShapeTransform = Preset.MakeShapeTransform(Pose[0]);
        const FVector Center = ShapeTransform.GetLocation();
        const FQuat Rotation = ShapeTransform.GetRotation();
        switch (Preset.Shape)
        {
        case EKataHitBoxShape::Capsule:
            DrawDebugCapsule(&World, Center, FMath::Max(Preset.Radius, Preset.CapsuleHalfHeight), Preset.Radius, Rotation, Color, bPersistent, Lifetime);
            break;
        case EKataHitBoxShape::Box:
            DrawDebugBox(&World, Center, Preset.BoxExtent, Rotation, Color, bPersistent, Lifetime);
            break;
        case EKataHitBoxShape::Sphere:
        default:
            DrawDebugSphere(&World, Center, Preset.Radius, 12, Color, bPersistent, Lifetime);
            break;
        }
    }

    void DrawTriangles(const UWorld& World, TConstArrayView<FTriangle> Triangles, const FColor& Color, float Lifetime, float Thickness = 0.0f)
    {
        for (const FTriangle& Triangle : Triangles)
        {
            DrawDebugLine(&World, Triangle.A, Triangle.B, Color, false, Lifetime, 0, Thickness);
            DrawDebugLine(&World, Triangle.B, Triangle.C, Color, false, Lifetime, 0, Thickness);
            DrawDebugLine(&World, Triangle.C, Triangle.A, Color, false, Lifetime, 0, Thickness);
        }
    }

    /**
     * 맞은 컴포넌트에서 맞은 도형을 그린다. 바디 번호가 있으면 그 바디만, 없으면 컴포넌트의 도형을 모두 그린다.
     * 판정과 같은 도형 수집을 쓰므로 무엇과 교차했는지 그대로 보인다.
     */
    void DrawHitShapes(const UWorld& World, const FHitResult& Hit, const FColor& Color, float Lifetime)
    {
        UPrimitiveComponent* Component = Hit.GetComponent();
        if (Component == nullptr)
        {
            return;
        }
        TArray<KataHitGeometry::FShape> Shapes;
        KataHitGeometry::CollectShapes(*Component, Shapes);
        for (const KataHitGeometry::FShape& Shape : Shapes)
        {
            if (Hit.Item != INDEX_NONE && Shape.BodyIndex != INDEX_NONE && Shape.BodyIndex != Hit.Item)
            {
                continue;
            }
            switch (Shape.Type)
            {
            case KataHitGeometry::EShapeType::Sphere:
                DrawDebugSphere(&World, Shape.Center, Shape.Radius, 12, Color, false, Lifetime, 0, 1.5f);
                break;
            case KataHitGeometry::EShapeType::Capsule:
                DrawDebugCapsule(&World, Shape.Center, Shape.HalfSegment + Shape.Radius, Shape.Radius, Shape.Rotation, Color, false, Lifetime, 0, 1.5f);
                break;
            case KataHitGeometry::EShapeType::Box:
            default:
                DrawDebugBox(&World, Shape.Center, Shape.BoxExtent, Shape.Rotation, Color, false, Lifetime, 0, 1.5f);
                break;
            }
        }
    }
#endif
}

bool UKataHitSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UKataHitSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 프리뷰는 뷰포트 툴바 토글만 따른다. 게임용 CVar가 켜져 있어도 프리뷰 화면을 덮지 않도록
    // 에디터가 저장해 둔 프리뷰 기본 단계에서 시작한다.
    const UWorld* World = GetWorld();
    if (World != nullptr && World->WorldType == EWorldType::EditorPreview)
    {
        DebugDrawModeOverride = PreviewDefaultDebugDrawMode;
    }
}

void UKataHitSubsystem::Deinitialize()
{
    ActiveHitBoxes.Reset();
    PendingHits.Reset();
    HitBoxComponents.Reset();
    Super::Deinitialize();
}

TStatId UKataHitSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UKataHitSubsystem, STATGROUP_Tickables);
}

void UKataHitSubsystem::SetDebugDrawMode(EKataHitTraceDebugMode Mode)
{
    DebugDrawModeOverride = Mode;
}

EKataHitTraceDebugMode UKataHitSubsystem::PreviewDefaultDebugDrawMode = EKataHitTraceDebugMode::Off;

void UKataHitSubsystem::SetPreviewDefaultDebugDrawMode(EKataHitTraceDebugMode Mode)
{
    PreviewDefaultDebugDrawMode = Mode;
}

EKataHitTraceDebugMode UKataHitSubsystem::GetPreviewDefaultDebugDrawMode()
{
    return PreviewDefaultDebugDrawMode;
}

void UKataHitSubsystem::ClearDebugDrawMode()
{
    DebugDrawModeOverride.Reset();
}

EKataHitTraceDebugMode UKataHitSubsystem::GetDebugDrawMode() const
{
#if ENABLE_DRAW_DEBUG
    if (DebugDrawModeOverride.IsSet())
    {
        return DebugDrawModeOverride.GetValue();
    }
    return static_cast<EKataHitTraceDebugMode>(FMath::Clamp(CVarKataHitTraceDebug.GetValueOnGameThread(),
        static_cast<int32>(EKataHitTraceDebugMode::Off), static_cast<int32>(EKataHitTraceDebugMode::Detailed)));
#else
    return EKataHitTraceDebugMode::Off;
#endif
}

void UKataHitSubsystem::RegisterHitBoxComponent(UKataHitBoxComponent* Component)
{
    if (Component != nullptr)
    {
        HitBoxComponents.AddUnique(Component);
    }
}

void UKataHitSubsystem::UnregisterHitBoxComponent(UKataHitBoxComponent* Component)
{
    HitBoxComponents.Remove(Component);
}

int32 UKataHitSubsystem::RegisterHitBox(const FKataHitBoxRegistration& Registration)
{
    const UKataHitBoxPreset* Preset = Registration.Task != nullptr ? Registration.Task->HitBoxPreset.Get() : nullptr;
    if (Preset == nullptr || Registration.Mesh == nullptr)
    {
        return INDEX_NONE;
    }

    TArray<FName, TInlineAllocator<8>> Sockets;
    Preset->GetRequiredSockets(Sockets);
    for (const FName SocketName : Sockets)
    {
        // 없는 소켓을 읽으면 엔진이 컴포넌트 트랜스폼을 돌려주어 엉뚱한 위치를 판정한다. 조용히 넘기지 않는다.
        if (!Registration.Mesh->DoesSocketExist(SocketName))
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Kata hit box '%s' needs socket '%s' but mesh '%s' has none"),
                *GetNameSafe(Preset), *SocketName.ToString(), *GetPathNameSafe(Registration.Mesh));
            return INDEX_NONE;
        }
    }

    FKataActiveHitBox& Entry = ActiveHitBoxes.AddDefaulted_GetRef();
    Entry.Handle = NextHandle++;
    Entry.Task = Registration.Task;
    Entry.Preset = Registration.Task->HitBoxPreset;
    Entry.TaskInstance = Registration.TaskInstance;
    Entry.ActionInstance = Registration.ActionInstance;
    Entry.Mesh = Registration.Mesh;
    Entry.HitBoxComponent = Registration.HitBoxComponent;
    Entry.InstigatorActor = Registration.InstigatorActor;
    Entry.SourceAbilitySystem = Registration.SourceAbilitySystem;
    Entry.StartTime = Registration.StartTime;
    Entry.EndTime = FMath::Max(Registration.StartTime, Registration.EndTime);
    Entry.bStartChecked = !Registration.Task->bCheckOnStart;
    return Entry.Handle;
}

void UKataHitSubsystem::CloseHitBox(int32 Handle, bool bFinalSweep)
{
    const int32 Index = ActiveHitBoxes.IndexOfByPredicate([Handle](const FKataActiveHitBox& Entry) { return Entry.Handle == Handle; });
    if (Index == INDEX_NONE)
    {
        return;
    }
    if (!bFinalSweep)
    {
        // 취소·중단된 공격은 마지막 판정 없이 바로 지운다. 캔슬한 공격이 끝에서 맞히지 않게 한다.
        ActiveHitBoxes.RemoveAt(Index);
        return;
    }

    FKataActiveHitBox& Entry = ActiveHitBoxes[Index];
    Entry.bClosing = true;
    if (const UKataActionInstance* Action = Entry.ActionInstance.Get())
    {
        // 의존성 대기로 늦게 시작한 태스크는 정의상의 종료 시각보다 늦게 끝날 수 있으므로 실제 종료 시각으로 맞춘다.
        Entry.EndTime = FMath::Max(Entry.StartTime, Action->GetCurrentTime());
    }
}

void UKataHitSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    ++TickIndex;

    // 처리기는 이 Tick의 끝에서 호출하므로 판정 루프 안에서는 등록 목록이 바뀌지 않는다.
    for (int32 Index = 0; Index < ActiveHitBoxes.Num();)
    {
        FKataActiveHitBox& Entry = ActiveHitBoxes[Index];
        const bool bKeep = ProcessHitBox(Entry, DeltaTime) && !Entry.bClosing;
        if (bKeep)
        {
            ++Index;
        }
        else
        {
            ActiveHitBoxes.RemoveAt(Index);
        }
    }

    // 판정이 이번 Tick의 직전 포즈 기록을 읽은 뒤에 새 기록으로 덮는다.
    HitBoxComponents.RemoveAll([](const TWeakObjectPtr<UKataHitBoxComponent>& Component) { return !Component.IsValid(); });
    for (const TWeakObjectPtr<UKataHitBoxComponent>& Component : HitBoxComponents)
    {
        Component->RecordPoses(TickIndex);
    }

    DispatchPendingHits();
}

bool UKataHitSubsystem::ProcessHitBox(FKataActiveHitBox& Entry, float DeltaTime)
{
    using namespace KataHitTrace;

    UWorld* World = GetWorld();
    const UKataHitBoxPreset* Preset = Entry.Preset;
    UMeshComponent* Mesh = Entry.Mesh.Get();
    AActor* Instigator = Entry.InstigatorActor.Get();
    if (World == nullptr || Preset == nullptr || Mesh == nullptr || Instigator == nullptr)
    {
        return false;
    }
    // 태스크 인스턴스가 사라졌는데 닫히지 않았다면 소유 액션이 정리 없이 사라진 것이다. 더 판정하지 않는다.
    if (!Entry.bClosing && !Entry.TaskInstance.IsValid())
    {
        return false;
    }

    TArray<FName, TInlineAllocator<8>> SocketNames;
    Preset->GetRequiredSockets(SocketNames);
    const int32 SocketCount = SocketNames.Num();
    const bool bSocketTrace = Preset->Mode == EKataHitBoxMode::SocketTrace;

    FSocketPose CurrentPose;
    CurrentPose.SetNum(SocketCount);
    for (int32 Index = 0; Index < SocketCount; ++Index)
    {
        CurrentPose[Index] = Mesh->GetSocketTransform(SocketNames[Index], RTS_World);
    }

    // 이번 Tick에 해당하는 Kata 시각. 액션이 먼저 사라졌으면 종료 시각을 이번 포즈의 시각으로 본다.
    const UKataActionInstance* Action = Entry.ActionInstance.Get();
    const float CurrentTime = Action != nullptr ? Action->GetCurrentTime() : Entry.EndTime;

    // 루프 경계에서 Kata 시각이 되돌아가면 이전 기록은 다른 회차의 것이다.
    if (Entry.bHasPreviousPose && (CurrentTime < Entry.PreviousTime || Entry.PreviousSockets.Num() != SocketCount))
    {
        Entry.bHasPreviousPose = false;
    }

    FSocketPose PreviousPose = CurrentPose;
    float PreviousTime = CurrentTime;
    if (Entry.bHasPreviousPose)
    {
        PreviousPose = FSocketPose(Entry.PreviousSockets);
        PreviousTime = Entry.PreviousTime;
    }
    else if (const UKataHitBoxComponent* Component = Entry.HitBoxComponent.Get())
    {
        // 첫 Tick: 구간이 이 프레임 안에서 시작했으므로 직전 프레임 포즈가 있어야 시작 시각부터 쓸 수 있다.
        FSocketPose ComponentPose;
        ComponentPose.SetNum(SocketCount);
        bool bHasAll = true;
        for (int32 Index = 0; Index < SocketCount && bHasAll; ++Index)
        {
            bHasAll = Component->GetPreviousSocketTransform(Mesh, SocketNames[Index], TickIndex, ComponentPose[Index]);
        }
        if (bHasAll)
        {
            PreviousPose = MoveTemp(ComponentPose);
            // Kata 시각은 액터별 시간 배율이 적용된 DeltaTime으로 진행하므로 직전 프레임 시각도 같은 배율로 되짚는다.
            PreviousTime = CurrentTime - DeltaTime * Instigator->CustomTimeDilation;
        }
    }

    const float FrameSpan = CurrentTime - PreviousTime;
    const auto AlphaAt = [PreviousTime, FrameSpan](float Time)
    {
        return FrameSpan > UE_KINDA_SMALL_NUMBER ? FMath::Clamp((Time - PreviousTime) / FrameSpan, 0.0f, 1.0f) : 1.0f;
    };

    FCollisionQueryParams Params(SCENE_QUERY_STAT(KataHitTrace), Preset->bTraceComplex, Instigator);
    TArray<AActor*> AttachedActors;
    Instigator->GetAttachedActors(AttachedActors, true, true);
    Params.AddIgnoredActors(AttachedActors);
    for (const TWeakObjectPtr<AActor>& HitActor : Entry.HitActors)
    {
        if (AActor* Actor = HitActor.Get())
        {
            Params.AddIgnoredActor(Actor);
        }
    }

    const FCollisionShape SweepShape = Preset->MakeCollisionShape();
    const ECollisionChannel Channel = Preset->TraceChannel;

#if ENABLE_DRAW_DEBUG
    const EKataHitTraceDebugMode DebugMode = GetDebugDrawMode();
    const bool bDrawDetailed = DebugMode == EKataHitTraceDebugMode::Detailed;
#endif

    TArray<FHitCandidate> Candidates;
    const auto AddHit = [&Candidates](const FHitResult& Hit, int32 Order)
    {
        AActor* HitActor = Hit.GetActor();
        FHitCandidate* Existing = Candidates.FindByPredicate([HitActor](const FHitCandidate& Candidate) { return Candidate.Actor.Get() == HitActor; });
        if (Existing == nullptr)
        {
            Candidates.Add({ HitActor, Hit, Order });
        }
        else if (Order < Existing->Order || (Order == Existing->Order && Hit.Time < Existing->Hit.Time))
        {
            Existing->Hit = Hit;
            Existing->Order = Order;
        }
    };

    // SocketTrace는 시작 판정과 구간 판정의 삼각형을 모아 넓은 단계 질의를 한 번만 한다.
    TArray<FTriangle> Triangles;

    // 시작 시점 판정: 구간이 열린 순간 판정 영역 안에 이미 있는 대상을 찾는다.
    if (!Entry.bStartChecked)
    {
        Entry.bStartChecked = true;
        const FSocketPose StartPose = BlendPose(PreviousPose, CurrentPose, AlphaAt(Entry.StartTime));
        if (bSocketTrace)
        {
            AppendBlade(StartPose, -1, Triangles);
        }
        else
        {
            const FTransform ShapeTransform = Preset->MakeShapeTransform(StartPose[0]);
            TArray<FOverlapResult> Overlaps;
            World->OverlapMultiByChannel(Overlaps, ShapeTransform.GetLocation(), ShapeTransform.GetRotation(), Channel, SweepShape, Params);
            for (const FOverlapResult& Overlap : Overlaps)
            {
                AActor* OverlapActor = Overlap.GetActor();
                UPrimitiveComponent* OverlapComponent = Overlap.GetComponent();
                if (OverlapActor == nullptr || OverlapComponent == nullptr)
                {
                    continue;
                }
                const FVector Center = ShapeTransform.GetLocation();
                FHitResult Hit(OverlapActor, OverlapComponent, Center, (Center - OverlapComponent->GetComponentLocation()).GetSafeNormal());
                Hit.bStartPenetrating = true;
                Hit.Item = Overlap.ItemIndex;
                Hit.BoneName = FindBoneName(OverlapComponent, Overlap.ItemIndex);
                Hit.TraceStart = Center;
                Hit.TraceEnd = Center;
                AddHit(Hit, -1);
            }
        }
    }

    // 구간 판정: 이번 프레임 중 태스크 구간 [StartTime, EndTime]에 속하는 부분만 쓴다.
    // 종료 Tick에서는 애니메이션이 EndTime을 지나쳤더라도 EndTime 포즈까지만 판정한다.
    const float SegmentStart = FMath::Max(PreviousTime, Entry.StartTime);
    const float SegmentEnd = FMath::Min(CurrentTime, Entry.EndTime);
    if (FrameSpan > UE_KINDA_SMALL_NUMBER && SegmentEnd > SegmentStart + UE_KINDA_SMALL_NUMBER)
    {
        const float StartAlpha = AlphaAt(SegmentStart);
        const float EndAlpha = AlphaAt(SegmentEnd);
        const FSocketPose SegmentStartPose = BlendPose(PreviousPose, CurrentPose, StartAlpha);
        const FSocketPose SegmentEndPose = BlendPose(PreviousPose, CurrentPose, EndAlpha);

        // 서브스텝 수: 소켓의 최대 이동거리와 최대 회전각 중 더 많은 칸을 요구하는 쪽을 따른다.
        double MaxDistance = 0.0;
        double MaxAngleDegrees = 0.0;
        if (bSocketTrace)
        {
            for (int32 Index = 0; Index < SocketCount; ++Index)
            {
                MaxDistance = FMath::Max(MaxDistance, FVector::Dist(SegmentStartPose[Index].GetLocation(), SegmentEndPose[Index].GetLocation()));
            }
            // 칼날의 회전은 소켓 자체의 회전이 아니라 첫 소켓에서 끝 소켓으로 향하는 방향의 변화다.
            const FVector StartDirection = (SegmentStartPose.Last().GetLocation() - SegmentStartPose[0].GetLocation()).GetSafeNormal();
            const FVector EndDirection = (SegmentEndPose.Last().GetLocation() - SegmentEndPose[0].GetLocation()).GetSafeNormal();
            MaxAngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(StartDirection, EndDirection), -1.0, 1.0)));
        }
        else
        {
            const FTransform StartShape = Preset->MakeShapeTransform(SegmentStartPose[0]);
            const FTransform EndShape = Preset->MakeShapeTransform(SegmentEndPose[0]);
            MaxDistance = FVector::Dist(StartShape.GetLocation(), EndShape.GetLocation());
            MaxAngleDegrees = FMath::RadiansToDegrees(StartShape.GetRotation().AngularDistance(EndShape.GetRotation()));
        }
        const double RequiredSteps = FMath::Max(MaxDistance / Preset->MaxStepDistance, MaxAngleDegrees / Preset->MaxStepAngle);
        const int32 StepCount = FMath::Clamp(FMath::CeilToInt(RequiredSteps), 1, FMath::Max(1, Preset->MaxSubsteps));

        const int32 FirstSegmentTriangle = Triangles.Num();
        FSocketPose StepFrom = SegmentStartPose;
        for (int32 Step = 0; Step < StepCount; ++Step)
        {
            const float StepAlpha = FMath::Lerp(StartAlpha, EndAlpha, static_cast<float>(Step + 1) / static_cast<float>(StepCount));
            FSocketPose StepTo = BlendPose(PreviousPose, CurrentPose, StepAlpha);

            if (bSocketTrace)
            {
                AppendSweptStrip(StepFrom, StepTo, Step, Triangles);
            }
            else
            {
                // SweepMulti는 회전 하나만 받아 스윕 도중 도형이 돌지 않는다. 칸마다 가운데 회전을 써서 오차를 줄인다.
                const FTransform FromShape = Preset->MakeShapeTransform(StepFrom[0]);
                const FTransform ToShape = Preset->MakeShapeTransform(StepTo[0]);
                const FQuat MidRotation = FQuat::Slerp(FromShape.GetRotation(), ToShape.GetRotation(), 0.5f).GetNormalized();
                TArray<FHitResult> Hits;
                SweepSegment(*World, FromShape.GetLocation(), ToShape.GetLocation(), MidRotation, SweepShape, Channel, Params, Hits);
                for (const FHitResult& Hit : Hits)
                {
                    AddHit(Hit, Step);
                }
#if ENABLE_DRAW_DEBUG
                if (bDrawDetailed)
                {
                    DrawDebugLine(World, FromShape.GetLocation(), ToShape.GetLocation(), FColor::Cyan, false, DetailedDrawLifetime);
                }
#endif
            }
            StepFrom = MoveTemp(StepTo);
        }

#if ENABLE_DRAW_DEBUG
        if (bDrawDetailed)
        {
            if (bSocketTrace)
            {
                // 칼날이 쓸고 간 판정 면을 삼각형 그대로 그린다.
                DrawTriangles(*World, TConstArrayView<FTriangle>(Triangles).RightChop(FirstSegmentTriangle), FColor::Cyan, DetailedDrawLifetime);
            }
        }
#else
        (void)FirstSegmentTriangle;
#endif
    }

    TArray<FTriangleHit> TriangleHits;
    if (bSocketTrace)
    {
        QueryTriangles(*World, Triangles, Preset->Thickness, Channel, Params, TriangleHits);
        for (const FTriangleHit& TriangleHit : TriangleHits)
        {
            AddHit(TriangleHit.Hit, TriangleHit.Order);
        }
    }

    Entry.PreviousSockets = TArray<FTransform>(CurrentPose);
    Entry.PreviousTime = CurrentTime;
    Entry.bHasPreviousPose = true;

    const int32 HitCountBefore = PendingHits.Num();
    SubmitHits(Entry, Candidates);

#if ENABLE_DRAW_DEBUG
    if (DebugMode != EKataHitTraceDebugMode::Off && !Entry.bClosing)
    {
        const bool bHitThisTick = PendingHits.Num() > HitCountBefore;
        DrawPose(*World, *Preset, CurrentPose, bHitThisTick ? FColor::Red : FColor::Green, -1.0f);
    }
    if (DebugMode != EKataHitTraceDebugMode::Off)
    {
        // 판정에 쓰인 첫 교차 삼각형. 주황 표시에서 빼고 마지막에 빨간색으로 덮어 그린다.
        TArray<int32, TInlineAllocator<4>> FirstHitTriangles;
        for (int32 Index = HitCountBefore; Index < PendingHits.Num(); ++Index)
        {
            const FHitResult& Hit = PendingHits[Index].HitResult;
            if (UPrimitiveComponent* HitComponent = Hit.GetComponent())
            {
                Entry.DebugHitComponents.AddUnique(HitComponent);
            }
            const AActor* HitActor = Hit.GetActor();
            if (const FTriangleHit* TriangleHit = TriangleHits.FindByPredicate([HitActor](const FTriangleHit& Candidate) { return Candidate.Hit.GetActor() == HitActor; }))
            {
                FirstHitTriangles.Add(TriangleHit->TriangleIndex);
            }
        }

        // 판정은 첫 교차 하나만 쓰지만, 칼날 면이 대상을 얼마나 지나갔는지 보이도록 맞은 컴포넌트와 교차한 나머지 삼각형을 주황색으로 그린다.
        // 한 번 맞은 대상은 이후 Tick부터 판정에서 빠지므로 컴포넌트를 기억해 두고 구간이 끝날 때까지 계속 그린다.
        // 이 계산은 디버그를 켰을 때만 한다.
        Entry.DebugHitComponents.RemoveAll([](const TWeakObjectPtr<UPrimitiveComponent>& Component) { return !Component.IsValid(); });
        TArray<KataHitGeometry::FShape> HitShapes;
        for (const TWeakObjectPtr<UPrimitiveComponent>& HitComponent : Entry.DebugHitComponents)
        {
            HitShapes.Reset();
            KataHitGeometry::CollectShapes(*HitComponent.Get(), HitShapes);
            for (int32 TriangleIndex = 0; TriangleIndex < Triangles.Num(); ++TriangleIndex)
            {
                if (FirstHitTriangles.Contains(TriangleIndex))
                {
                    continue;
                }
                const FTriangle& Triangle = Triangles[TriangleIndex];
                FVector Contact;
                const bool bIntersects = HitShapes.ContainsByPredicate([&](const KataHitGeometry::FShape& Shape)
                {
                    return KataHitGeometry::IntersectTriangle(Triangle.A, Triangle.B, Triangle.C, Shape, Preset->Thickness, Contact);
                });
                if (bIntersects)
                {
                    DrawTriangles(*World, MakeArrayView(&Triangle, 1), FColor::Orange, DetailedDrawLifetime, 1.5f);
                }
            }
        }

        // 히트는 한 프레임에만 일어나므로 맞은 도형, 첫 교차 삼각형, 히트 지점을 잠시 남겨 둔다.
        for (int32 Index = HitCountBefore; Index < PendingHits.Num(); ++Index)
        {
            const FHitResult& Hit = PendingHits[Index].HitResult;
            DrawHitShapes(*World, Hit, FColor::Red, DetailedDrawLifetime);
            DrawDebugPoint(World, Hit.ImpactPoint, 12.0f, FColor::Red, false, DetailedDrawLifetime);
            DrawDebugDirectionalArrow(World, Hit.ImpactPoint, Hit.ImpactPoint + Hit.ImpactNormal * 20.0f, 8.0f, FColor::Red, false, DetailedDrawLifetime);
        }
        for (const int32 TriangleIndex : FirstHitTriangles)
        {
            DrawTriangles(*World, MakeArrayView(&Triangles[TriangleIndex], 1), FColor::Red, DetailedDrawLifetime, 2.0f);
        }
    }
#endif

    return true;
}

void UKataHitSubsystem::SubmitHits(FKataActiveHitBox& Entry, TArray<FHitCandidate>& Candidates)
{
    Candidates.RemoveAll([&Entry](const FHitCandidate& Candidate)
    {
        return !Candidate.Actor.IsValid() || Entry.HitActors.Contains(Candidate.Actor);
    });
    if (Candidates.IsEmpty())
    {
        return;
    }

    AActor* Instigator = Entry.InstigatorActor.Get();
    const UTargetingPreset* FilterPreset = Entry.Task != nullptr ? Entry.Task->FilterPreset.Get() : nullptr;
    if (FilterPreset != nullptr && Instigator != nullptr)
    {
        // UTargetingSubsystem은 GameInstance Subsystem이라 프리뷰 월드에는 없다.
        // 즉시 실행에 필요한 것은 요청 핸들의 데이터 저장소뿐이므로 태스크를 직접 순서대로 실행해 게임과 프리뷰가 같은 경로를 쓴다.
        FTargetingSourceContext SourceContext;
        SourceContext.SourceActor = Instigator;
        SourceContext.InstigatorActor = Instigator;
        SourceContext.SourceLocation = Instigator->GetActorLocation();
        FTargetingRequestHandle Handle = UTargetingSubsystem::MakeTargetRequestHandle(FilterPreset, SourceContext);

        FTargetingDefaultResultsSet& ResultsSet = FTargetingDefaultResultsSet::FindOrAdd(Handle);
        for (const FHitCandidate& Candidate : Candidates)
        {
            ResultsSet.TargetResults.AddDefaulted_GetRef().HitResult = Candidate.Hit;
        }

        if (const FTargetingTaskSet* TaskSet = FilterPreset->GetTargetingTaskSet())
        {
            for (const UTargetingTask* FilterTask : TaskSet->Tasks)
            {
                if (FilterTask != nullptr)
                {
                    FilterTask->Init(Handle);
                    FilterTask->Execute(Handle);
                }
            }
        }

        TSet<const AActor*> Survivors;
        if (const FTargetingDefaultResultsSet* FilteredSet = FTargetingDefaultResultsSet::Find(Handle))
        {
            for (const FTargetingDefaultResultData& Result : FilteredSet->TargetResults)
            {
                Survivors.Add(Result.HitResult.GetActor());
            }
        }
        UTargetingSubsystem::ReleaseTargetRequestHandle(Handle);

        Candidates.RemoveAll([&Survivors](const FHitCandidate& Candidate) { return !Survivors.Contains(Candidate.Actor.Get()); });
    }

    Candidates.StableSort([](const FHitCandidate& A, const FHitCandidate& B)
    {
        return A.Order != B.Order ? A.Order < B.Order : A.Hit.Time < B.Hit.Time;
    });

    for (const FHitCandidate& Candidate : Candidates)
    {
        Entry.HitActors.Add(Candidate.Actor);

        UE_LOG(LogKataFramework, Verbose, TEXT("Kata hit: '%s' hit '%s' (component '%s', bone '%s') with task '%s'"),
            *GetNameSafe(Instigator), *GetNameSafe(Candidate.Actor.Get()), *GetNameSafe(Candidate.Hit.GetComponent()),
            *Candidate.Hit.BoneName.ToString(), *GetNameSafe(Entry.Task));

        FKataHitRecord& Record = PendingHits.AddDefaulted_GetRef();
        Record.InstigatorActor = Instigator;
        Record.TargetActor = Candidate.Actor.Get();
        Record.SourceAbilitySystem = Entry.SourceAbilitySystem.Get();
        Record.SourceTask = Entry.Task;
        Record.HitResult = Candidate.Hit;
        Record.Sequence = NextSequence++;
    }
}

void UKataHitSubsystem::DispatchPendingHits()
{
    if (PendingHits.IsEmpty())
    {
        return;
    }

    // 처리기가 액션을 끝내거나 새 판정을 등록해도 이번 목록은 바뀌지 않도록 옮겨서 처리한다.
    TArray<FKataHitRecord> Hits = MoveTemp(PendingHits);
    PendingHits.Reset();

    for (const FKataHitRecord& Record : Hits)
    {
        if (!IsValid(Record.TargetActor) || Record.SourceTask == nullptr)
        {
            continue;
        }
        for (const UKataHitHandler* Handler : Record.SourceTask->HitHandlers)
        {
            if (Handler != nullptr)
            {
                Handler->HandleHit(Record.InstigatorActor, Record.TargetActor, Record.HitResult, Record.SourceAbilitySystem, Record.SourceTask);
            }
        }
    }
}
