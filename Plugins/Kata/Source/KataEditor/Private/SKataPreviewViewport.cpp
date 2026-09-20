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
    // SetEditor(false)는 월드의 RequiresHitProxies를 끄고, 그러면 FHitProxyMeshProcessor가
    // 메시 히트 프록시를 아예 만들지 않아 메시로 그리는 이동 기즈모 축을 집을 수 없다.
    // EditorPreview 월드는 게임 월드가 아니므로 MovementComponent 갱신을 따로 켜 준다.
    PreviewScene = MakeUnique<FKataPreviewScene>(FPreviewScene::ConstructionValues()
        .SetEditor(true).SetCreatePhysicsScene(true).ShouldSimulatePhysics(true).AllowAudioPlayback(true)
        .ForceUseMovementComponentInNonGameWorld(true));
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

FBox SKataPreviewViewport::GetPreviewFocusBox() const
{
    FBox Box(ForceInit);
    auto AddActor = [&Box](const AActor* Actor)
    {
        if (!Actor)
        {
            return;
        }
        // Primitive가 없는 액터는 바운드가 비어 있으므로 위치만 포함한다.
        const FBox ActorBox = Actor->GetComponentsBoundingBox(true);
        if (ActorBox.IsValid)
        {
            Box += ActorBox;
        }
        else
        {
            Box += Actor->GetActorLocation();
        }
    };
    AddActor(PreviewActor);
    AddActor(TargetActor);
    if (!Box.IsValid)
    {
        Box = FBox(FVector(-100.0), FVector(100.0));
    }
    // 액터가 한 점에 가까울 때 과도하게 확대되지 않도록 최소 여유를 둔다.
    return Box.ExpandBy(FVector(50.0));
}

ELevelViewportType SKataPreviewViewport::GetBackViewportType() const
{
    // Self Actor의 Forward를 수평면에서 가장 가까운 월드 축으로 스냅한다.
    FVector Forward = PreviewActor ? PreviewActor->GetActorForwardVector() : FVector::ForwardVector;
    Forward.Z = 0;
    if (!Forward.Normalize())
    {
        Forward = FVector::ForwardVector;
    }
    // 카메라가 Self의 시선 방향을 그대로 바라보는 뷰를 고른다. 즉 Self의 등 뒤에서 본다.
    // 각 뷰의 시선 방향은 FEditorViewportClient::GetForwardVector가 정의한다.
    if (FMath::Abs(Forward.X) >= FMath::Abs(Forward.Y))
    {
        return Forward.X >= 0 ? LVT_OrthoBack : LVT_OrthoFront;
    }
    return Forward.Y >= 0 ? LVT_OrthoRight : LVT_OrthoLeft;
}

ELevelViewportType SKataPreviewViewport::GetViewportTypeFor(EKataPreviewView View) const
{
    switch (View)
    {
    case EKataPreviewView::Back:
        return GetBackViewportType();
    case EKataPreviewView::Top:
        return LVT_OrthoTop;
    case EKataPreviewView::Right:
        return LVT_OrthoRight;
    case EKataPreviewView::Perspective:
    default:
        return LVT_Perspective;
    }
}

void SKataPreviewViewport::ApplyDefaultPlacement(EKataPreviewView View)
{
    if (View == EKataPreviewView::Perspective)
    {
        Client->SetViewLocation(PerspectiveLocation);
        Client->SetViewRotation(PerspectiveRotation);
        Client->SetLookAtLocation(GetPreviewFocusBox().GetCenter());
        return;
    }
    // 직교 중심과 확대 배율은 엔진의 포커스 계산을 사용한다.
    Client->FocusViewportOnBox(GetPreviewFocusBox(), true);
}

void SKataPreviewViewport::StoreCurrentViewState()
{
    if (!Client.IsValid())
    {
        return;
    }
    FKataPreviewViewState& State = ViewStates[static_cast<int32>(CurrentView)];
    State.Location = Client->GetViewLocation();
    State.Rotation = Client->GetViewRotation();
    State.LookAt = Client->GetLookAtLocation();
    State.OrthoZoom = Client->GetOrthoZoom();
    State.Type = Client->GetViewportType();
    State.bStored = true;
}

void SKataPreviewViewport::SetPreviewView(EKataPreviewView View, bool bResetCamera)
{
    if (!Client.IsValid())
    {
        CurrentView = View;
        return;
    }
    // 뷰포트 종류를 바꾸기 전에 지금 구도의 카메라를 기록한다.
    // 위치와 확대 배율은 현재 뷰포트 종류에 해당하는 트랜스폼에서만 읽을 수 있다.
    StoreCurrentViewState();
    CurrentView = View;

    const ELevelViewportType Type = GetViewportTypeFor(View);
    Client->SetViewportType(Type);

    // Back View는 Self Actor의 방향이 바뀌면 다른 축을 쓰므로 기록한 구도를 버리고 다시 맞춘다.
    FKataPreviewViewState& State = ViewStates[static_cast<int32>(View)];
    if (bResetCamera || !State.bStored || State.Type != Type)
    {
        ApplyDefaultPlacement(View);
        StoreCurrentViewState();
    }
    else
    {
        Client->SetViewLocation(State.Location);
        Client->SetViewRotation(State.Rotation);
        Client->SetLookAtLocation(State.LookAt);
        // SetOrthoZoom은 0을 받으면 단언에 걸린다.
        if (State.OrthoZoom != 0.0f)
        {
            Client->SetOrthoZoom(State.OrthoZoom);
        }
    }
    Client->Invalidate();
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
            // 위젯으로 옮기는 액터의 표시용 메시이므로 Movable로 둔다.
            Marker->SetMobility(EComponentMobility::Movable);
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
    // Static 루트는 등록 후 이동이 거부되므로 프리뷰 액터는 항상 Movable로 둔다.
    auto MakeMovable = [](AActor* Actor)
    {
        USceneComponent* Root = Actor ? Actor->GetRootComponent() : nullptr;
        if (Root && Root->Mobility != EComponentMobility::Movable)
        {
            Root->SetMobility(EComponentMobility::Movable);
        }
    };
    if (Asset)
    {
        PreviewActor = Spawn(Asset->PreviewActorClass, Asset->PreviewActorTransform);
        TargetActor = Spawn(Asset->PreviewTargetClass, Asset->PreviewTargetTransform);
        MakeMovable(PreviewActor);
        MakeMovable(TargetActor);
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
