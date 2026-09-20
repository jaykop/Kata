#include "SKataPreviewViewport.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Definition/KataAsset.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "Materials/MaterialInterface.h"
#include "PreviewScene.h"
#include "Runtime/KataComponent.h"
#include "Runtime/KataInstance.h"

/** 에셋별 배경색을 반환하는 가벼운 프리뷰 장면. */
class FKataPreviewScene final : public FPreviewScene
{
public:
    explicit FKataPreviewScene(const FPreviewScene::ConstructionValues& Values)
        : FPreviewScene(Values)
    {
    }

    virtual FLinearColor GetBackgroundColor() const override { return BackgroundColor; }
    void SetKataBackgroundColor(const FLinearColor& InColor) { BackgroundColor = InColor; }

private:
    FLinearColor BackgroundColor = FLinearColor(0.015f, 0.02f, 0.025f);
};

void SKataPreviewViewport::Construct(const FArguments& Args)
{
    TargetMovedEvent = Args._OnTargetMoved;
    PreviewScene = MakeUnique<FKataPreviewScene>(FPreviewScene::ConstructionValues()
        .SetEditor(false).SetCreatePhysicsScene(true).ShouldSimulatePhysics(true).AllowAudioPlayback(true));
    PreviewScene->GetWorld()->BeginPlay();
    SEditorViewport::Construct(SEditorViewport::FArguments());
}

SKataPreviewViewport::~SKataPreviewViewport()
{
    Stop();
    // 뷰포트 클라이언트가 장면보다 먼저 해제되어야 장면 포인터가 남지 않는다.
    if (Client)
    {
        Client->Viewport = nullptr;
    }
    PreviewClient.Reset();
    Client.Reset();
}

namespace
{
    const FVector PerspectiveLocation(-400, -450, 250);
    const FRotator PerspectiveRotation(-15, 45, 0);
    /** 캐릭터 크기의 프리뷰를 담을 정도의 직교 확대 배율. */
    constexpr float PreviewOrthoZoom = 2000.0f;
}

TSharedRef<FEditorViewportClient> SKataPreviewViewport::MakeEditorViewportClient()
{
    PreviewClient = MakeShared<FKataPreviewViewportClient>(PreviewScene.Get(), SharedThis(this));
    PreviewClient->OnTargetTransformChanged = TargetMovedEvent;
    PreviewClient->SetTargetSelectionEnabled(bTargetSelectionEnabled);
    PreviewClient->SetViewLocation(PerspectiveLocation);
    PreviewClient->SetViewRotation(PerspectiveRotation);
    PreviewClient->SetViewMode(VMI_Lit);
    PreviewClient->SetRealtime(true);
    PreviewClient->EngineShowFlags.SetModeWidgets(true);
    // 엔진 기본 격자 대신 에셋 크기에 맞는 원점 기준 측정 격자를 사용한다.
    PreviewClient->EngineShowFlags.SetGrid(false);
    return PreviewClient.ToSharedRef();
}

void SKataPreviewViewport::SetTargetSelectionEnabled(bool bEnabled)
{
    bTargetSelectionEnabled = bEnabled;
    if (PreviewClient.IsValid())
    {
        PreviewClient->SetTargetSelectionEnabled(bEnabled);
    }
}

bool SKataPreviewViewport::IsTargetSelectionEnabled() const
{
    return bTargetSelectionEnabled;
}

void SKataPreviewViewport::ApplySceneSettings(UKataAsset* Asset)
{
    if (PreviewClient.IsValid())
    {
        PreviewClient->SetTargetActor(TargetActor);
    }
    if (!Asset)
    {
        return;
    }
    PreviewEnvironmentSize = Asset->PreviewEnvironmentSize.ComponentMax(FVector(100.0));
    // 프리뷰 조명은 에셋의 editor-only 설정만 사용한다. 게임 월드에는 영향을 주지 않는다.
    PreviewScene->SetLightDirection(Asset->PreviewLightRotation);
    PreviewScene->SetLightBrightness(Asset->PreviewLightBrightness);
    PreviewScene->SetLightColor(Asset->PreviewLightColor.ToFColor(true));
    static_cast<FKataPreviewScene*>(PreviewScene.Get())->SetKataBackgroundColor(Asset->PreviewBackgroundColor);
    if (PreviewClient.IsValid())
    {
        PreviewClient->SetMeasurementSettings(Asset->PreviewEnvironmentSize, Asset->PreviewGridCellSize,
            Asset->bPreviewShowDebugShape, Asset->PreviewDebugShape == EKataPreviewDebugShape::Sphere,
            Asset->PreviewDebugColor, Asset->PreviewDebugThickness);
    }
}

void SKataPreviewViewport::SetPreviewViewportType(ELevelViewportType Type)
{
    if (!Client.IsValid())
    {
        return;
    }
    Client->SetViewportType(Type);
    if (Type == LVT_Perspective)
    {
        Client->SetViewLocation(PerspectiveLocation);
        Client->SetViewRotation(PerspectiveRotation);
    }
    else
    {
        // 직교 카메라는 두 액터의 중점을 바라보되 Back View는 Self Actor를 기준으로 잡는다.
        FVector FocusLocation = FVector::ZeroVector;
        int32 FocusCount = 0;
        if (Type == LVT_OrthoBack && PreviewActor)
        {
            FocusLocation = PreviewActor->GetComponentsBoundingBox(true).GetCenter();
            FocusCount = 1;
        }
        else if (PreviewActor)
        {
            FocusLocation += PreviewActor->GetActorLocation();
            ++FocusCount;
        }
        if (TargetActor)
        {
            FocusLocation += TargetActor->GetActorLocation();
            ++FocusCount;
        }
        if (FocusCount > 0)
        {
            FocusLocation /= FocusCount;
        }
        FVector CameraLocation;
        if (Type == LVT_OrthoBack)
        {
            // Self Actor의 기본 Forward(+X)를 따라 뒤쪽(-X)에서 등을 바라본다.
            CameraLocation = FocusLocation;
            const double InsideWallX = -PreviewEnvironmentSize.X * 0.5 + 50.0;
            CameraLocation.X = FMath::Max(FocusLocation.X - 600.0, InsideWallX);
        }
        else
        {
            constexpr double OrthoCameraDistance = 10000.0;
            CameraLocation = FocusLocation - Client->GetForwardVector() * OrthoCameraDistance;
        }
        Client->SetViewLocation(CameraLocation);
        Client->SetOrthoZoom(PreviewOrthoZoom);
    }
    Client->Invalidate();
}

bool SKataPreviewViewport::IsPreviewViewportType(ELevelViewportType Type) const
{
    return Client.IsValid() && Client->GetViewportType() == Type;
}

UAbilitySystemComponent* SKataPreviewViewport::PrepareAbilitySystem(AActor* Actor)
{
    if (!Actor)
    {
        return nullptr;
    }
    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
    if (!ASC)
    {
        ASC = Actor->FindComponentByClass<UAbilitySystemComponent>();
    }
    if (!ASC)
    {
        ASC = NewObject<UAbilitySystemComponent>(Actor, NAME_None, RF_Transient);
        Actor->AddInstanceComponent(ASC);
        ASC->RegisterComponent();
    }
    ASC->InitAbilityActorInfo(Actor, Actor);
    return ASC;
}

void SKataPreviewViewport::ResetScene(UKataAsset* Asset)
{
    Stop();
    UWorld* World = PreviewScene->GetWorld();
    if (PreviewClient.IsValid())
    {
        // 선택 집합에서 이전 Target을 먼저 빼고 프리뷰 액터를 파괴한다.
        PreviewClient->SetTargetActor(nullptr);
    }
    // 태스크가 프리뷰 중 생성한 액터도 함께 정리한다. 편집 중인 레벨 월드는 건드리지 않는다.
    TArray<AActor*> Actors;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (*It != World->GetWorldSettings())
        {
            Actors.Add(*It);
        }
    }
    for (AActor* Actor : Actors)
    {
        World->DestroyActor(Actor);
    }
    PreviewActor = nullptr;
    TargetActor = nullptr;
    Component = nullptr;
    Instance = nullptr;
    SimulatedTime = 0;
    FActorSpawnParameters Params;
    Params.ObjectFlags = RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto Spawn = [&](UClass* Class, const FTransform& Transform)
    {
        AActor* Actor = World->SpawnActor<AActor>(Class ? Class : AActor::StaticClass(), Transform, Params);
        if (Actor && !Actor->GetRootComponent())
        {
            USceneComponent* Root = NewObject<USceneComponent>(Actor, NAME_None, RF_Transient);
            Actor->AddInstanceComponent(Root);
            Actor->SetRootComponent(Root);
            Root->RegisterComponent();
            Actor->SetActorTransform(Transform);
        }
        if (Actor && !Class)
        {
            UStaticMeshComponent* Marker = NewObject<UStaticMeshComponent>(Actor, NAME_None, RF_Transient);
            Actor->AddInstanceComponent(Marker);
            Marker->SetupAttachment(Actor->GetRootComponent());
            Marker->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
            Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Marker->SetRelativeScale3D(FVector(0.4));
            Marker->RegisterComponent();
        }
        return Actor;
    };
    const FVector EnvironmentSize = Asset ? Asset->PreviewEnvironmentSize.ComponentMax(FVector(100.0))
        : FVector(2000.0, 2000.0, 1000.0);
    constexpr double SurfaceThickness = 10.0;
    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UMaterialInterface* GridMaterial = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Engine/OpenWorldTemplate/LandscapeMaterial/MI_ProcGrid.MI_ProcGrid"));
    auto AddSurface = [&](const FVector& Location, const FVector& Size)
    {
        AActor* Surface = Spawn(AActor::StaticClass(), FTransform(Location));
        if (!Surface)
        {
            return;
        }
        UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Surface, NAME_None, RF_Transient);
        Surface->AddInstanceComponent(Mesh);
        Mesh->SetupAttachment(Surface->GetRootComponent());
        Mesh->SetStaticMesh(CubeMesh);
        Mesh->SetRelativeScale3D(Size / 100.0);
        if (GridMaterial)
        {
            Mesh->SetMaterial(0, GridMaterial);
        }
        Mesh->SetCollisionProfileName(TEXT("BlockAll"));
        Mesh->RegisterComponent();
    };
    // 바닥의 윗면은 프리뷰 월드 원점의 Z=0에 맞춘다.
    AddSurface(FVector(0, 0, -SurfaceThickness * 0.5),
        FVector(EnvironmentSize.X, EnvironmentSize.Y, SurfaceThickness));
    // 앞쪽 벽과 왼쪽 벽이 같은 환경 크기를 사용해 하나의 코너를 이룬다.
    AddSurface(FVector(-EnvironmentSize.X * 0.5 - SurfaceThickness * 0.5, 0, EnvironmentSize.Z * 0.5),
        FVector(SurfaceThickness, EnvironmentSize.Y, EnvironmentSize.Z));
    AddSurface(FVector(0, EnvironmentSize.Y * 0.5 + SurfaceThickness * 0.5, EnvironmentSize.Z * 0.5),
        FVector(EnvironmentSize.X, SurfaceThickness, EnvironmentSize.Z));
    if (Asset)
    {
        PreviewActor = Spawn(Asset->PreviewActorClass, Asset->PreviewActorTransform);
        TargetActor = Spawn(Asset->PreviewTargetClass, Asset->PreviewTargetTransform);
    }
    PrepareAbilitySystem(PreviewActor);
    PrepareAbilitySystem(TargetActor);
    if (PreviewActor)
    {
        Component = PreviewActor->FindComponentByClass<UKataComponent>();
        if (!Component)
        {
            Component = NewObject<UKataComponent>(PreviewActor, NAME_None, RF_Transient);
            PreviewActor->AddInstanceComponent(Component);
            Component->RegisterComponent();
        }
    }
    ApplySceneSettings(Asset);
    Status = TEXT("Ready");
    Invalidate();
}

bool SKataPreviewViewport::Start(UKataAsset* Asset)
{
    ResetScene(Asset);
    if (!Component)
    {
        Status = TEXT("Preview actor could not be created");
        return false;
    }
    FKataContext Context;
    Context.OwnerActor = PreviewActor;
    Context.AvatarActor = PreviewActor;
    Context.TargetActor = TargetActor;
    Context.AbilitySystem = PrepareAbilitySystem(PreviewActor);
    UKataInstance* Started = nullptr;
    const EKataStartResult Result = Component->PlayKataAsset(Asset, Context, Started);
    Instance = Started;
    if (Result != EKataStartResult::Started)
    {
        Status = FString::Printf(TEXT("Start failed: %s"), *StaticEnum<EKataStartResult>()->GetNameStringByValue(static_cast<int64>(Result)));
        return false;
    }
    bPlaying = Instance && Instance->IsRunning();
    Status = bPlaying ? TEXT("Playing") : TEXT("Completed");
    return true;
}

void SKataPreviewViewport::Play(UKataAsset* Asset)
{
    SeekTarget = -1;
    if (Instance && Instance->IsRunning())
    {
        bPlaying = true;
        Status = TEXT("Playing");
        return;
    }
    Start(Asset);
}

void SKataPreviewViewport::Pause()
{
    bPlaying = false;
    SeekTarget = -1;
    Status = TEXT("Paused");
}

void SKataPreviewViewport::Stop()
{
    bPlaying = false;
    SeekTarget = -1;
    if (Component)
    {
        Component->StopKata(EKataEndReason::Cancelled);
    }
    Instance = nullptr;
    Status = TEXT("Stopped");
}

void SKataPreviewViewport::Seek(UKataAsset* Asset, float Time)
{
    if (Start(Asset) && Instance && Instance->IsRunning())
    {
        // 임의 시각을 직접 대입하지 않고 액터를 초기화한 프리뷰 월드에서 처음부터 재생한다.
        SeekTarget = FMath::Clamp(Time, 0.0f, Instance->GetTimelineDuration());
        bPlaying = false;
        Status = TEXT("Seeking");
    }
}

void SKataPreviewViewport::TickSimulation(float DeltaTime)
{
    if (!Instance || !Instance->IsRunning())
    {
        return;
    }
    UWorld* World = PreviewScene->GetWorld();
    if (SeekTarget >= 0)
    {
        // 긴 탐색도 프레임마다 나눠 진행해 편집기 입력을 막지 않는다.
        for (int32 Step = 0; Step < 8 && Instance->IsRunning() && SimulatedTime < SeekTarget; ++Step)
        {
            const float Delta = FMath::Min(1.0f / 60.0f, SeekTarget - SimulatedTime);
            World->Tick(LEVELTICK_All, Delta);
            SimulatedTime += Delta;
        }
        if (SimulatedTime >= SeekTarget - UE_KINDA_SMALL_NUMBER || !Instance->IsRunning())
        {
            SeekTarget = -1;
            Status = TEXT("Paused");
        }
    }
    else if (bPlaying)
    {
        World->Tick(LEVELTICK_All, FMath::Min(DeltaTime, 1.0f / 15.0f));
        if (!Instance->IsRunning())
        {
            bPlaying = false;
            Status = TEXT("Completed");
        }
    }
    Invalidate();
}

float SKataPreviewViewport::GetTime() const
{
    return Instance ? Instance->GetCurrentTime() : 0.0f;
}

void SKataPreviewViewport::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(PreviewActor);
    Collector.AddReferencedObject(TargetActor);
    Collector.AddReferencedObject(Component);
    Collector.AddReferencedObject(Instance);
}
