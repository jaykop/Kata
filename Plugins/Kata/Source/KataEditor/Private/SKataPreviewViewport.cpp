#include "SKataPreviewViewport.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AnimPreviewInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceHelpers.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Action/KataAction.h"
#include "Action/KataResolvedAction.h"
#include "EditorViewportCommands.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Materials/MaterialInterface.h"
#include "PreviewScene.h"
#include "Runtime/KataComponent.h"
#include "Runtime/KataActionInstance.h"
#include "Tasks/KataTask_PlayMontage.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "ViewportToolbar/UnrealEdViewportToolbar.h"
#include "ViewportToolbar/UnrealEdViewportToolbarContext.h"

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
    ActorMovedEvent = Args._OnActorMoved;
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
    PreviewClient->OnTransformChanged = ActorMovedEvent;
    // 클라이언트는 소멸자에서 이 위젯보다 먼저 해제되므로 raw 바인딩으로 충분하다.
    PreviewClient->OnViewportTypeLeaving.BindRaw(this, &SKataPreviewViewport::StoreViewState);
    PreviewClient->OnViewportTypeEntered.BindRaw(this, &SKataPreviewViewport::RestoreViewState);
    PreviewClient->SetManipulatedActor(GetActorForSlot(ManipulatedSlot), ManipulatedSlot);
    PreviewClient->SetViewLocation(PerspectiveLocation);
    PreviewClient->SetViewRotation(PerspectiveRotation);
    PreviewClient->SetViewMode(VMI_Lit);
    PreviewClient->SetRealtime(true);
    PreviewClient->EngineShowFlags.SetModeWidgets(true);
    // 엔진 기본 격자 대신 에셋 크기에 맞는 원점 기준 측정 격자를 사용한다.
    PreviewClient->EngineShowFlags.SetGrid(false);
    return PreviewClient.ToSharedRef();
}

void SKataPreviewViewport::SetManipulatedSlot(EKataPreviewActorSlot Slot)
{
    ManipulatedSlot = Slot;
    if (PreviewClient.IsValid())
    {
        PreviewClient->SetManipulatedActor(GetActorForSlot(Slot), Slot);
    }
}

AActor* SKataPreviewViewport::GetActorForSlot(EKataPreviewActorSlot Slot) const
{
    switch (Slot)
    {
    case EKataPreviewActorSlot::Self:
        return PreviewActor;
    case EKataPreviewActorSlot::Target:
        return TargetActor;
    default:
        return nullptr;
    }
}

void SKataPreviewViewport::ApplySceneSettings(UKataAction* Asset)
{
    if (PreviewClient.IsValid())
    {
        // 장면을 다시 만들면 액터가 바뀌므로 지금 자리에 해당하는 새 액터를 다시 물린다.
        PreviewClient->SetManipulatedActor(GetActorForSlot(ManipulatedSlot), ManipulatedSlot);
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

void SKataPreviewViewport::ApplyDefaultPlacement()
{
    if (Client->GetViewportType() == LVT_Perspective)
    {
        Client->SetViewLocation(PerspectiveLocation);
        Client->SetViewRotation(PerspectiveRotation);
        Client->SetLookAtLocation(GetPreviewFocusBox().GetCenter());
        return;
    }
    // 직교 중심과 확대 배율은 엔진의 포커스 계산을 사용한다.
    Client->FocusViewportOnBox(GetPreviewFocusBox(), true);
}

void SKataPreviewViewport::StoreViewState(ELevelViewportType Type)
{
    if (!Client.IsValid())
    {
        return;
    }
    FKataPreviewViewState& State = ViewStates.FindOrAdd(Type);
    State.Location = Client->GetViewLocation();
    State.Rotation = Client->GetViewRotation();
    State.LookAt = Client->GetLookAtLocation();
    State.OrthoZoom = Client->GetOrthoZoom();
}

void SKataPreviewViewport::RestoreViewState(ELevelViewportType Type)
{
    if (!Client.IsValid())
    {
        return;
    }
    if (const FKataPreviewViewState* State = ViewStates.Find(Type))
    {
        Client->SetViewLocation(State->Location);
        Client->SetViewRotation(State->Rotation);
        Client->SetLookAtLocation(State->LookAt);
        // SetOrthoZoom은 0을 받으면 단언에 걸린다.
        if (State->OrthoZoom != 0.0f)
        {
            Client->SetOrthoZoom(State->OrthoZoom);
        }
    }
    else
    {
        ApplyDefaultPlacement();
        StoreViewState(Type);
    }
    Client->Invalidate();
}

void SKataPreviewViewport::ResetCamera()
{
    if (!Client.IsValid())
    {
        return;
    }
    ApplyDefaultPlacement();
    StoreViewState(Client->GetViewportType());
    Client->Invalidate();
}

void SKataPreviewViewport::BindCommands()
{
    SEditorViewport::BindCommands();
    // 엔진 Camera 메뉴는 Bottom을 직접 추가하므로 항목을 지울 수 없다. 명령을 풀면 메뉴가 오류를 기록하므로
    // 보이지 않고 실행되지 않는 동작으로 다시 연결한다. 바닥 아래에서 올려다볼 일이 없는 프리뷰다.
    const TSharedPtr<FUICommandInfo> BottomCommand = FEditorViewportCommands::Get().Bottom;
    CommandList->UnmapAction(BottomCommand);
    CommandList->MapAction(BottomCommand,
        FExecuteAction(),
        FCanExecuteAction::CreateLambda([]() { return false; }),
        FIsActionChecked(),
        FIsActionButtonVisible::CreateLambda([]() { return false; }));
}

void SKataPreviewViewport::OnFocusViewportToSelection()
{
    if (Client.IsValid())
    {
        Client->FocusViewportOnBox(GetPreviewFocusBox());
    }
}

namespace
{
    const FName KataViewportToolbarName = "KataActionEditor.ViewportToolbar";

    /** 액션 확인에 쓸 만한 뷰 모드만 남긴다. 목록은 블루프린트 에디터 프리뷰를 참고했다. */
    bool IsKataViewModeSupported(EViewModeIndex ViewModeIndex)
    {
        switch (ViewModeIndex)
        {
        case VMI_Unlit:
        case VMI_Lit:
        case VMI_BrushWireframe:
        case VMI_LightingOnly:
        case VMI_CollisionPawn:
        case VMI_CollisionVisibility:
        case VMI_Clay:
            return true;
        default:
            return false;
        }
    }

    /** 노출 등 부가 섹션은 Preview Details의 조명 설정과 겹치므로 모두 숨긴다. */
    bool DoesKataViewModeMenuShowSection(UE::UnrealEd::EHidableViewModeMenuSections)
    {
        return false;
    }

    /**
     * 엔진 Transform 서브메뉴에서 프리뷰가 지원하는 Select·Move·Rotate만 남긴다.
     * 크기 조절, 복합 기즈모와 좌표계 전환은 프리뷰 배치에서 제공하지 않는다.
     */
    FToolMenuEntry CreateKataTransformSubmenu()
    {
        return FToolMenuEntry::InitSubMenu(
            "Transform",
            NSLOCTEXT("Kata", "TransformSubmenu", "Transform"),
            NSLOCTEXT("Kata", "TransformSubmenuTooltip", "Select, move or rotate the selected preview actor"),
            FNewToolMenuDelegate::CreateLambda([](UToolMenu* Submenu)
            {
                FToolMenuSection& Section = Submenu->FindOrAddSection("TransformTools",
                    NSLOCTEXT("Kata", "TransformTools", "Transform Tools"));
                FToolMenuEntryToolBarData ToolBarData;
                ToolBarData.StyleNameOverride = "ViewportToolbar.TransformTools";
                const FEditorViewportCommands& Commands = FEditorViewportCommands::Get();
                const TSharedPtr<FUICommandInfo> TransformCommands[] = {
                    Commands.SelectMode, Commands.TranslateMode, Commands.RotateMode };
                for (const TSharedPtr<FUICommandInfo>& Command : TransformCommands)
                {
                    FToolMenuEntry Entry = FToolMenuEntry::InitMenuEntry(Command);
                    Entry.SetShowInToolbarTopLevel(true);
                    Entry.ToolBarData = ToolBarData;
                    Section.AddEntry(Entry);
                }
            }));
    }

    /** 툴바 메뉴는 전역 등록이므로 처음 여는 Kata 에디터가 한 번만 등록하고 이후에는 재사용한다. */
    void RegisterKataViewportToolbar()
    {
        UToolMenus* Menus = UToolMenus::Get();
        if (Menus->IsMenuRegistered(KataViewportToolbarName))
        {
            return;
        }
        UToolMenu* Toolbar = Menus->RegisterMenu(KataViewportToolbarName, NAME_None, EMultiBoxType::SlimHorizontalToolBar);
        Toolbar->StyleName = "ViewportToolbar";

        FToolMenuSection& LeftSection = Toolbar->AddSection("Left");
        LeftSection.AddEntry(CreateKataTransformSubmenu());

        FToolMenuSection& RightSection = Toolbar->AddSection("Right");
        RightSection.Alignment = EToolMenuSectionAlign::Last;

        // 카메라: 엔진의 뷰 종류·이동 속도·Frame·렌즈 설정에 Reset Camera를 덧붙인다.
        RightSection.AddEntry(UE::UnrealEd::CreateCameraSubmenu(UE::UnrealEd::FViewportCameraMenuOptions().ShowAll()));
        UToolMenu* CameraMenu = Menus->ExtendMenu(UToolMenus::JoinMenuPaths(KataViewportToolbarName, "Camera"));
        CameraMenu->AddDynamicSection("KataCamera", FNewToolMenuDelegate::CreateLambda([](UToolMenu* Menu)
        {
            const UUnrealEdViewportToolbarContext* Context = Menu->FindContext<UUnrealEdViewportToolbarContext>();
            if (!Context)
            {
                return;
            }
            const TWeakPtr<SEditorViewport> WeakViewport = Context->Viewport;
            Menu->FindOrAddSection("Movement").AddMenuEntry(
                "KataResetCamera",
                NSLOCTEXT("Kata", "ResetCamera", "Reset Camera"),
                NSLOCTEXT("Kata", "ResetCameraTooltip", "Return the current view to its default position and zoom"),
                FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"),
                FUIAction(FExecuteAction::CreateLambda([WeakViewport]()
                {
                    // 이 메뉴는 SKataPreviewViewport만 생성하므로 컨텍스트의 뷰포트는 항상 이 타입이다.
                    if (const TSharedPtr<SEditorViewport> Viewport = WeakViewport.Pin())
                    {
                        StaticCastSharedPtr<SKataPreviewViewport>(Viewport)->ResetCamera();
                    }
                })));
        }));

        // 뷰 모드 메뉴는 기존 뷰포트 툴바와 호환되도록 공용 부모 메뉴 아래에 먼저 등록해야 한다.
        const FName ViewParentMenuName = "UnrealEd.ViewportToolbar.View";
        if (!Menus->IsMenuRegistered(ViewParentMenuName))
        {
            Menus->RegisterMenu(ViewParentMenuName);
        }
        Menus->RegisterMenu(UToolMenus::JoinMenuPaths(KataViewportToolbarName, "ViewModes"), ViewParentMenuName);
        RightSection.AddEntry(UE::UnrealEd::CreateViewModesSubmenu());

        RightSection.AddEntry(UE::UnrealEd::CreateDefaultShowSubmenu());
    }
}

TSharedPtr<SWidget> SKataPreviewViewport::BuildViewportToolbar()
{
    RegisterKataViewportToolbar();

    FToolMenuContext MenuContext;
    MenuContext.AppendCommandList(GetCommandList());

    UUnrealEdViewportToolbarContext* ContextObject = NewObject<UUnrealEdViewportToolbarContext>();
    ContextObject->Viewport = SharedThis(this);
    ContextObject->bShowCoordinateSystemControls = false;
    // 프리뷰 월드에 없는 환경 요소와 렌더링 개발용 플래그는 Show 메뉴에서 뺀다.
    ContextObject->ExcludedShowMenuFlags.Append({
        FEngineShowFlags::EShowFlag::SF_Atmosphere,
        FEngineShowFlags::EShowFlag::SF_BSP,
        FEngineShowFlags::EShowFlag::SF_Cloud,
        FEngineShowFlags::EShowFlag::SF_Fog,
        FEngineShowFlags::EShowFlag::SF_Landscape,
        FEngineShowFlags::EShowFlag::SF_MediaPlanes,
        FEngineShowFlags::EShowFlag::SF_Navigation });
    ContextObject->ExcludedShowMenuGroupFlags.Append({
        EShowFlagGroup::SFG_PostProcess,
        EShowFlagGroup::SFG_LightTypes,
        EShowFlagGroup::SFG_LightingComponents,
        EShowFlagGroup::SFG_LightingFeatures,
        EShowFlagGroup::SFG_Lumen,
        EShowFlagGroup::SFG_MegaLights,
        EShowFlagGroup::SFG_Nanite,
        EShowFlagGroup::SFG_Developer,
        EShowFlagGroup::SFG_Visualize,
        EShowFlagGroup::SFG_Advanced,
        EShowFlagGroup::SFG_Custom });
    ContextObject->IsViewModeSupported = UE::UnrealEd::IsViewModeSupportedDelegate::CreateStatic(&IsKataViewModeSupported);
    ContextObject->DoesViewModeMenuShowSection =
        UE::UnrealEd::DoesViewModeMenuShowSectionDelegate::CreateStatic(&DoesKataViewModeMenuShowSection);
    MenuContext.AddObject(ContextObject);

    return UToolMenus::Get()->GenerateWidget(KataViewportToolbarName, MenuContext);
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

void SKataPreviewViewport::ResetScene(UKataAction* Asset)
{
    Stop();
    UWorld* World = PreviewScene->GetWorld();
    if (PreviewClient.IsValid())
    {
        // 선택 집합에서 이전 조작 대상을 먼저 빼고 프리뷰 액터를 파괴한다. 자리는 유지한다.
        PreviewClient->SetManipulatedActor(nullptr, ManipulatedSlot);
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
    PlayheadTime = 0.0f;
    SimulatedTime = 0;
    bCompletedPlayback = false;
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
        // 클래스를 지정하지 않은 자리는 표시용 메시 없이 빈 액터만 둔다. 화면에는 보이지 않지만
        // Kata 실행 주체와 대상 위치, 트랜스폼 위젯 조작에는 계속 쓰인다.
        return Actor;
    };
    const FVector EnvironmentSize = Asset ? Asset->PreviewEnvironmentSize.ComponentMax(FVector(100.0))
        : FVector(10000.0, 10000.0, 1000.0);
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
    // 앞쪽 벽과 옆 벽이 같은 환경 크기를 사용해 하나의 코너를 이룬다. 에셋이 없으면 둘 다 표시한다.
    if (!Asset || Asset->bPreviewShowFrontWall)
    {
        AddSurface(FVector(-EnvironmentSize.X * 0.5 - SurfaceThickness * 0.5, 0, EnvironmentSize.Z * 0.5),
            FVector(SurfaceThickness, EnvironmentSize.Y, EnvironmentSize.Z));
    }
    if (!Asset || Asset->bPreviewShowSideWall)
    {
        AddSurface(FVector(0, EnvironmentSize.Y * 0.5 + SurfaceThickness * 0.5, EnvironmentSize.Z * 0.5),
            FVector(EnvironmentSize.X, SurfaceThickness, EnvironmentSize.Z));
    }
    // Static 루트는 등록 후 이동이 거부되므로 프리뷰 액터는 항상 Movable로 둔다.
    auto MakeMovable = [](AActor* Actor)
    {
        USceneComponent* Root = Actor ? Actor->GetRootComponent() : nullptr;
        if (Root && Root->Mobility != EComponentMobility::Movable)
        {
            Root->SetMobility(EComponentMobility::Movable);
        }
    };
    // 프리뷰 캐릭터에는 Controller가 없다. UCharacterMovementComponent는 Controller가 없으면
    // 걷기 이동을 중단하고 속도와 가속을 0으로 만들어 중력도 적용하지 않는다.
    //
    // ACharacter::PostInitializeComponents는 Controller가 없을 때 이 플래그가 켜져 있어야만
    // SetDefaultMovementMode를 호출한다. 스폰이 끝난 뒤에 플래그를 켜면 그 시점을 이미 지나쳐
    // MovementMode가 기본값 MOVE_None에 머물고 StartNewPhysics가 아무 일도 하지 않는다.
    // 그래서 플래그를 켠 뒤 이동 모드도 직접 지정한다.
    auto AllowMovementWithoutController = [](AActor* Actor)
    {
        ACharacter* Character = Cast<ACharacter>(Actor);
        UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
        if (Movement)
        {
            Movement->bRunPhysicsWithNoController = true;
            Movement->SetDefaultMovementMode();
        }
    };
    if (Asset)
    {
        PreviewActor = Spawn(Asset->PreviewActorClass, Asset->PreviewActorTransform);
        TargetActor = Spawn(Asset->PreviewTargetClass, Asset->PreviewTargetTransform);
        MakeMovable(PreviewActor);
        MakeMovable(TargetActor);
        AllowMovementWithoutController(PreviewActor);
        AllowMovementWithoutController(TargetActor);
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
    bStatusError = false;
    Invalidate();
}

bool SKataPreviewViewport::Start(UKataAction* Asset)
{
    ResetScene(Asset);
    if (!Component)
    {
        Status = TEXT("Preview actor could not be created");
        bStatusError = true;
        return false;
    }
    FKataContext Context;
    Context.OwnerActor = PreviewActor;
    Context.AvatarActor = PreviewActor;
    Context.TargetActor = TargetActor;
    Context.AbilitySystem = PrepareAbilitySystem(PreviewActor);
    UKataActionInstance* Started = nullptr;
    const EKataStartResult Result = Component->PlayKataAction(Asset, Context, Started);
    Instance = Started;
    if (Result != EKataStartResult::Started)
    {
        Status = FString::Printf(TEXT("Start failed: %s"), *StaticEnum<EKataStartResult>()->GetNameStringByValue(static_cast<int64>(Result)));
        bStatusError = true;
        return false;
    }
    bPlaying = Instance && Instance->IsRunning();
    SimulatedTime = Instance ? Instance->GetCurrentTime() : 0.0f;
    PlayheadTime = SimulatedTime;
    Status = bPlaying ? TEXT("Playing") : TEXT("Completed");
    return true;
}

void SKataPreviewViewport::Play(UKataAction* Asset)
{
    bCompletedPlayback = false;
    if (!bPoseScrubbing && Instance && Instance->IsRunning())
    {
        bPlaying = true;
        Status = TEXT("Playing");
        return;
    }
    // 포즈 탐색은 액션을 실행하지 않으므로 재생 헤드 시각의 실행 상태가 없다.
    // 처음부터 다시 실행해 그 시각까지 진행한 뒤 재생을 이어 가도록 예약한다.
    const float ResumeTime = bPoseScrubbing ? PlayheadTime : 0.0f;
    if (Start(Asset) && ResumeTime > UE_KINDA_SMALL_NUMBER && Instance && Instance->IsRunning())
    {
        PendingResumeTime = ResumeTime;
        PlayheadTime = ResumeTime;
    }
}

void SKataPreviewViewport::Pause()
{
    bPlaying = false;
    bCompletedPlayback = false;
    Status = TEXT("Paused");
}

void SKataPreviewViewport::Stop()
{
    bPlaying = false;
    bCompletedPlayback = false;
    PendingResumeTime = -1.0f;
    RestorePosePreview();
    bPoseScrubbing = false;
    if (Component)
    {
        Component->StopKata(EKataEndReason::Cancelled);
    }
    Instance = nullptr;
    PlayheadTime = 0.0f;
    SimulatedTime = 0.0f;
    Status = TEXT("Stopped");
}

void SKataPreviewViewport::Seek(UKataAction* Asset, const UKataResolvedAction* EditingAction, float Time)
{
    const float RequestedTime = FMath::Max(0.0f, Time);
    if (!bPoseScrubbing)
    {
        // 실행 중 생성된 액터·몽타주를 한 번만 정리한다. 마우스 이동마다 장면을 재생성하지 않는다.
        ResetScene(Asset);
        bPoseScrubbing = true;
    }

    PlayheadTime = RequestedTime;
    bPlaying = false;
    bCompletedPlayback = false;
    Status = TEXT("Paused");

    // 같은 시각에 여러 몽타주가 겹치면 가장 늦게 시작한 태스크가 표시를 소유한다.
    // 시작 시각도 같으면 해석된 태스크 배열에서 나중 항목이 우선한다.
    const UKataTask_PlayMontage* Chosen = nullptr;
    if (EditingAction)
    {
        for (const UKataTask* Task : EditingAction->Tasks)
        {
            const UKataTask_PlayMontage* Candidate = Cast<UKataTask_PlayMontage>(Task);
            if (!Candidate || !Candidate->bEnabled || !Candidate->Montage
                || !FMath::IsFinite(Candidate->StartTime) || Candidate->StartTime < 0.0f
                || !FMath::IsFinite(Candidate->Duration) || Candidate->Duration < 0.0f
                || !FMath::IsFinite(Candidate->PlayRate) || Candidate->PlayRate <= 0.0f
                || RequestedTime < Candidate->StartTime - UE_KINDA_SMALL_NUMBER
                || RequestedTime > Candidate->GetEndTime() + UE_KINDA_SMALL_NUMBER)
            {
                continue;
            }
            if (!Chosen || Candidate->StartTime >= Chosen->StartTime)
            {
                Chosen = Candidate;
            }
        }
    }

    if (Chosen)
    {
        float StartPosition = 0.0f;
        if (!Chosen->StartSection.IsNone())
        {
            const int32 SectionIndex = Chosen->Montage->GetSectionIndex(Chosen->StartSection);
            if (SectionIndex != INDEX_NONE)
            {
                float SectionEnd = 0.0f;
                Chosen->Montage->GetSectionStartAndEndTime(SectionIndex, StartPosition, SectionEnd);
            }
        }
        const float MontagePosition = FMath::Clamp(StartPosition
            + FMath::Max(0.0f, RequestedTime - Chosen->StartTime) * Chosen->PlayRate * Chosen->Montage->RateScale,
            0.0f, Chosen->Montage->GetPlayLength());
        ShowMontagePose(Chosen->Montage, StartPosition, MontagePosition);
    }
    else
    {
        RestorePosePreview();
    }
    Invalidate();
}

void SKataPreviewViewport::TickSimulation(float DeltaTime)
{
    if (bPoseScrubbing)
    {
        // 멈춘 프리뷰 포즈를 Idle 애니메이션이나 월드 Tick이 덮지 않는다.
        Invalidate();
        return;
    }
    if (PendingResumeTime >= 0.0f)
    {
        const float ResumeTime = PendingResumeTime;
        PendingResumeTime = -1.0f;
        SimulateTo(ResumeTime);
        Invalidate();
        return;
    }
    UWorld* World = PreviewScene->GetWorld();
    if (!Instance || !Instance->IsRunning())
    {
        // FEditorViewportClient는 프리뷰 월드를 진행시키지 않으므로 여기서 직접 진행한다.
        // 실행 중인 Kata가 없어도 월드가 흘러야 Idle 애니메이션과 중력 안정화가 이어진다.
        // 이 경로에는 진행할 인스턴스가 없어 실행 Subsystem이 Kata를 앞당길 수 없다.
        // 일시정지는 인스턴스가 살아 있어 이 경로를 타지 않으므로 장면이 그대로 멈춘다.
        World->Tick(LEVELTICK_All, FMath::Min(DeltaTime, 1.0f / 15.0f));
        Invalidate();
        return;
    }
    if (bPlaying)
    {
        StepWorld(FMath::Min(DeltaTime, 1.0f / 15.0f));
        SimulatedTime = Instance->GetCurrentTime();
        PlayheadTime = SimulatedTime;
        if (!Instance->IsRunning())
        {
            bPlaying = false;
            // 실제로 진행하던 인스턴스가 끝난 경우에만 표시해 반복 재생이 이 시점만 이어받게 한다.
            bCompletedPlayback = Instance->GetCurrentTime() > UE_KINDA_SMALL_NUMBER;
            Status = TEXT("Completed");
        }
    }
    Invalidate();
}

void SKataPreviewViewport::RestorePosePreview()
{
    if (USkeletalMeshComponent* Mesh = PosePreviewMesh.Get())
    {
        Mesh->SetRelativeTransform(OriginalMeshRelativeTransform);
        if (AActor* Owner = Mesh->GetOwner())
        {
            Owner->SetActorTransform(OriginalActorTransform, false, nullptr, ETeleportType::TeleportPhysics);
            if (PreviewClient.IsValid())
            {
                PreviewClient->SyncCommitBaseline();
            }
        }
        const EAnimationMode::Type Mode = static_cast<EAnimationMode::Type>(OriginalAnimationMode);
        Mesh->SetAnimInstanceClass(nullptr);
        if (Mode == EAnimationMode::AnimationBlueprint && OriginalAnimClass)
        {
            Mesh->SetAnimInstanceClass(OriginalAnimClass);
        }
        else
        {
            Mesh->SetAnimationMode(Mode);
            if (Mode == EAnimationMode::AnimationSingleNode)
            {
                if (UAnimSingleNodeInstance* SingleNode = Mesh->GetSingleNodeInstance())
                {
                    SingleNode->SetAnimationAsset(OriginalAnimationAsset, false);
                }
            }
        }
        Mesh->TickAnimation(0.0f, false);
        Mesh->RefreshBoneTransforms();
    }
    PosePreviewMesh.Reset();
    OriginalAnimClass = nullptr;
    OriginalAnimationAsset = nullptr;
}

void SKataPreviewViewport::ShowMontagePose(UAnimMontage* Montage, float StartPosition, float MontagePosition)
{
    USkeletalMeshComponent* Mesh = PreviewActor ? PreviewActor->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
    if (!Mesh || !Mesh->GetSkeletalMeshAsset() || !Montage)
    {
        return;
    }
    if (PosePreviewMesh.Get() != Mesh)
    {
        RestorePosePreview();
        PosePreviewMesh = Mesh;
        OriginalMeshRelativeTransform = Mesh->GetRelativeTransform();
        OriginalActorTransform = PreviewActor->GetActorTransform();
        OriginalAnimationMode = static_cast<int32>(Mesh->GetAnimationMode());
        OriginalAnimClass = Mesh->GetAnimClass();
        if (UAnimSingleNodeInstance* SingleNode = Mesh->GetSingleNodeInstance())
        {
            OriginalAnimationAsset = SingleNode->GetAnimationAsset();
        }
        Mesh->SetAnimInstanceClass(UAnimPreviewInstance::StaticClass());
    }
    UAnimPreviewInstance* PreviewInstance = Cast<UAnimPreviewInstance>(Mesh->GetAnimInstance());
    if (!PreviewInstance)
    {
        RestorePosePreview();
        return;
    }
    if (PreviewInstance->GetAnimationAsset() != Montage)
    {
        // 엔진 몽타주 에디터와 같은 단일 에셋·전체 섹션 프리뷰를 한 번만 준비한다.
        PreviewInstance->SetAnimationAsset(Montage, false, 1.0f);
        PreviewInstance->MontagePreview_PreviewAllSections(false);
    }
    PreviewInstance->MontagePreview_JumpToPosition(MontagePosition);
    PreviewInstance->MontagePreview_SetPlaying(false);
    Mesh->TickAnimation(0.0f, false);
    // 엔진 애니메이션 에디터처럼 정지 상태의 루트 모션도 직접 추출한다.
    // 이전 커서 위치에 더하지 않고 시작 위치부터 다시 구해 왕복 탐색·태스크 전환에도 같은 결과를 낸다.
    const FTransform RootMotion = Montage->HasRootMotion()
        ? UE::Anim::ExtractRootMotionFromAnimationAsset(Montage, PreviewInstance->GetMirrorDataTable(),
            StartPosition, MontagePosition)
        : FTransform::Identity;
    // 루트 모션은 메시 컴포넌트 공간의 변화량이다. CharacterMovement가 재생 중에 하듯 메시가 루트 모션만큼 움직였을 때의
    // 액터 트랜스폼을 역산해 캡슐째 옮긴다. 메시 상대 트랜스폼은 그대로 두며, 충돌·중력은 계산하지 않는다.
    // 월드 트랜스폼 = 상대 × 액터이므로 새 액터 = 상대⁻¹ × 루트 모션 × 상대 × 원래 액터다.
    const FTransform NewActorTransform = OriginalMeshRelativeTransform.Inverse() * RootMotion
        * OriginalMeshRelativeTransform * OriginalActorTransform;
    PreviewActor->SetActorTransform(NewActorTransform, false, nullptr, ETeleportType::TeleportPhysics);
    if (PreviewClient.IsValid())
    {
        PreviewClient->SyncCommitBaseline();
    }
    Mesh->RefreshBoneTransforms();
}

void SKataPreviewViewport::StepWorld(float Delta)
{
    UWorld* World = PreviewScene->GetWorld();
    const uint64 PreviousTickSerial = Instance ? Instance->GetTickSerial() : 0;
    World->Tick(LEVELTICK_All, Delta);
    // 일부 EditorPreview 월드는 전역 실행 Subsystem의 월드 콜백을 호출하지 않는다.
    // 루프 경계에서는 액션 시각이 되돌아가므로 시각 대신 갱신 횟수로 중복 실행을 막는다.
    if (Instance && Instance->IsRunning() && Instance->GetTickSerial() == PreviousTickSerial)
    {
        Instance->TickInstance(Delta);
    }
}

void SKataPreviewViewport::SimulateTo(float TargetTime)
{
    if (!Instance || !Instance->IsRunning())
    {
        return;
    }
    // 액션 전체 길이까지 허용한다. 끝에 닿으면 인스턴스가 끝나며 일반 재생의 종료 처리로 이어진다.
    const float Target = FMath::Clamp(TargetTime, 0.0f, Instance->GetTimelineDuration());
    constexpr float StepSeconds = 1.0f / 60.0f;
    // 부동소수 오차나 루프 경계로 시각이 제자리에 머물러도 끝나도록 단계 수에 상한을 둔다.
    const int32 MaxSteps = FMath::CeilToInt(Target / StepSeconds) + 2;
    SetPreviewMeshesMultiTickPose(true);
    for (int32 Step = 0; Step < MaxSteps && Instance->IsRunning(); ++Step)
    {
        const float Remaining = Target - Instance->GetCurrentTime();
        if (Remaining <= UE_KINDA_SMALL_NUMBER)
        {
            break;
        }
        StepWorld(FMath::Min(StepSeconds, Remaining));
    }
    SetPreviewMeshesMultiTickPose(false);

    SimulatedTime = Instance->GetCurrentTime();
    PlayheadTime = SimulatedTime;
    if (!Instance->IsRunning())
    {
        bPlaying = false;
        bCompletedPlayback = SimulatedTime > UE_KINDA_SMALL_NUMBER;
        Status = TEXT("Completed");
    }
}

void SKataPreviewViewport::SetPreviewMeshesMultiTickPose(bool bEnable)
{
    // bIsAutonomousTickPose는 엔진이 한 프레임 여러 번 포즈 진행을 허용하는 공식 예외다.
    // CharacterMovement의 TickCharacterPose도 이 플래그를 켰다가 끄므로 루트 모션 경로에서 포즈가 두 번 진행되지 않는다.
    for (AActor* Actor : { PreviewActor.Get(), TargetActor.Get() })
    {
        if (!Actor)
        {
            continue;
        }
        TInlineComponentArray<USkeletalMeshComponent*> Meshes(Actor);
        for (USkeletalMeshComponent* Mesh : Meshes)
        {
            if (Mesh)
            {
                Mesh->bIsAutonomousTickPose = bEnable;
            }
        }
    }
}

float SKataPreviewViewport::GetTime() const
{
    return PlayheadTime;
}

void SKataPreviewViewport::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(PreviewActor);
    Collector.AddReferencedObject(TargetActor);
    Collector.AddReferencedObject(Component);
    Collector.AddReferencedObject(Instance);
    Collector.AddReferencedObject(OriginalAnimClass);
    Collector.AddReferencedObject(OriginalAnimationAsset);
}
