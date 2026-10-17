#include "Spawning/KataCharacterSpawner.h"

#include "Animation/AnimSequence.h"
#include "Character/KataCharacter.h"
#include "Character/KataCharacterRow.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Equipment/KataEquipmentComponent.h"
#include "Equipment/KataEquipmentRow.h"
#include "Equipment/KataEquipmentSetup.h"
#include "Spawning/KataFL_Spawning.h"
#include "Spawning/KataSpawnerComponent.h"
#include "Spawning/KataSpawnerComponent_SpawnArea.h"
#include "Spawning/KataWireCylinderComponent.h"

// 스포너의 에디터 미리보기 구현이다. 생성 로직과 분리해 KataCharacterSpawner.cpp가 생성·제거 흐름만 담게 한다.

#if WITH_EDITORONLY_DATA
namespace KataCharacterSpawnerPreviewPrivate
{
    /** 장비 미리보기 부품을 다른 자식 컴포넌트와 구분하는 태그. 갱신할 때 이 태그로 이전 부품을 찾아 지운다. */
    const FName EquipmentPartTag(TEXT("KataSpawnerEquipmentPreview"));

    /**
     * 캐릭터 미리보기에 붙은 장비 부품을 모두 제거한다.
     * 부품을 멤버 배열로 기억하지 않고 태그로 찾으므로 Undo·복제·Construction 재실행 뒤에도 남은 부품을 지울 수 있다.
     */
    void ClearEquipmentPreview(USkeletalMeshComponent& CharacterPreview)
    {
        // 제거가 부착 목록을 바꾸므로 사본을 순회한다.
        const TArray<TObjectPtr<USceneComponent>> Children = CharacterPreview.GetAttachChildren();
        for (USceneComponent* Child : Children)
        {
            if (Child != nullptr && Child->ComponentHasTag(EquipmentPartTag))
            {
                Child->DestroyComponent();
            }
        }
    }

    /** 부품 메시 하나의 컴포넌트를 만든다. 메시가 Static Mesh나 Skeletal Mesh가 아니면 nullptr다. */
    UMeshComponent* CreateEquipmentPart(AActor& Owner, UObject* MeshAsset)
    {
        // 저장되지 않도록 Transient로 만든다. Construction이 다시 실행되면 이전 부품을 지우고 새로 만든다.
        if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(MeshAsset))
        {
            UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(&Owner, NAME_None, RF_Transient);
            Component->SetStaticMesh(StaticMesh);
            return Component;
        }
        if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(MeshAsset))
        {
            USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(&Owner, NAME_None, RF_Transient);
            Component->SetSkeletalMeshAsset(SkeletalMesh);
            return Component;
        }
        return nullptr;
    }

    /**
     * 캐릭터 행의 시작 장비 부품을 캐릭터 미리보기의 슬롯 소켓에 붙인다.
     * 대상 슬롯, 점유 슬롯, 소켓은 UKataEquipmentComponent의 장착과 같은 규칙으로 정하지만 GE·태그·HitBox는 다루지 않는다.
     */
    void BuildEquipmentPreview(AActor& Owner, USkeletalMeshComponent& CharacterPreview, const FKataCharacterRow& Row,
        const AKataCharacter& CharacterDefaults)
    {
        // 행 적용과 같이 행의 Equipment Setup이 비어 있으면 캐릭터 Blueprint의 장착 컴포넌트 기본값을 쓴다.
        const UKataEquipmentSetup* Setup = Row.EquipmentSetup.LoadSynchronous();
        if (Setup == nullptr)
        {
            const UKataEquipmentComponent* DefaultEquipment = CharacterDefaults.GetEquipmentComponent();
            Setup = DefaultEquipment != nullptr ? DefaultEquipment->EquipmentSetup.Get() : nullptr;
        }

        struct FPreviewItem
        {
            const FKataEquipmentRow* EquipmentRow = nullptr;
            FGameplayTag TargetSlot;
            FGameplayTagContainer OccupiedSlots;
        };
        TArray<FPreviewItem> Items;
        for (const FKataStartingEquipment& Entry : Row.StartingEquipment)
        {
            const FKataEquipmentRow* EquipmentRow = Entry.EquipmentId.Find();
            if (EquipmentRow == nullptr)
            {
                continue;
            }
            const FGameplayTag TargetSlot = EquipmentRow->ResolveTargetSlot(Entry.Slot, Setup);
            const FGameplayTagContainer OccupiedSlots = EquipmentRow->GetOccupiedSlots(TargetSlot);
            if (OccupiedSlots.IsEmpty())
            {
                continue;
            }
            // 실제 장착은 점유 슬롯이 겹치는 이전 장비를 해제하므로 미리보기에서도 뒤 항목이 앞 항목을 대체한다.
            Items.RemoveAll([&OccupiedSlots](const FPreviewItem& Item)
            {
                return Item.OccupiedSlots.HasAnyExact(OccupiedSlots);
            });
            Items.Add({ EquipmentRow, TargetSlot, OccupiedSlots });
        }

        for (const FPreviewItem& Item : Items)
        {
            for (const FKataEquipmentPart& Part : Item.EquipmentRow->Parts)
            {
                UMeshComponent* PartComponent = CreateEquipmentPart(Owner, Part.Mesh.LoadSynchronous());
                if (PartComponent == nullptr)
                {
                    continue;
                }
                PartComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                PartComponent->SetGenerateOverlapEvents(false);
                PartComponent->SetCanEverAffectNavigation(false);
                PartComponent->SetHiddenInGame(true);
                PartComponent->bIsEditorOnly = true;
                PartComponent->ComponentTags.Add(EquipmentPartTag);
                PartComponent->RegisterComponent();

                // 소켓이 없으면 실제 장착처럼 원점에 붙는다. Construction마다 반복되므로 경고 로그는 남기지 않는다.
                const FGameplayTag PartSlot = Part.Slot.IsValid() ? Part.Slot : Item.TargetSlot;
                const FName Socket = Setup != nullptr ? Setup->SlotSockets.FindRef(PartSlot) : NAME_None;
                PartComponent->AttachToComponent(&CharacterPreview, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
                PartComponent->SetRelativeTransform(Part.RelativeTransform);
            }
        }
    }
}
#endif

void AKataCharacterSpawner::UpdateDistancePreview()
{
#if WITH_EDITORONLY_DATA
    if (SpawnDistancePreview == nullptr || DespawnDistancePreview == nullptr)
    {
        return;
    }
    const bool bVisible = Activation == EKataSpawnerActivation::PlayerDistance;
    // 판정은 액터 위치 기준 3D 거리이므로 액터 스케일과 무관한 절대 반지름으로 그린다.
    SpawnDistancePreview->SetSphereRadius(FMath::Max(SpawnDistance, 0.f), false);
    DespawnDistancePreview->SetSphereRadius(FMath::Max(DespawnDistance, 0.f), false);
    SpawnDistancePreview->SetVisibility(bVisible);
    DespawnDistancePreview->SetVisibility(bVisible);
#endif
}

const UKataSpawnerComponent_SpawnArea* AKataCharacterSpawner::FindEnabledSpawnArea() const
{
    for (const TObjectPtr<UKataSpawnerComponent>& Component : SpawnerComponents)
    {
        const UKataSpawnerComponent_SpawnArea* Settings = Cast<UKataSpawnerComponent_SpawnArea>(Component);
        if (IsValid(Settings) && Settings->bEnabled)
        {
            return Settings;
        }
    }
    return nullptr;
}

void AKataCharacterSpawner::UpdateCharacterPreview()
{
#if WITH_EDITORONLY_DATA
    if (CharacterPreview == nullptr)
    {
        return;
    }

    // 게임 월드에서는 미리보기가 숨겨져 있으므로 동기 로드를 하지 않는다.
    const UWorld* World = GetWorld();
    if (World == nullptr || World->IsGameWorld())
    {
        return;
    }

    // 행이나 메시가 바뀌어 아래에서 일찍 끝나도 이전 장비 부품이 남지 않게 먼저 지운다.
    KataCharacterSpawnerPreviewPrivate::ClearEquipmentPreview(*CharacterPreview);

    const FKataCharacterRow* Row = CharacterId.Find();
    UClass* CharacterClass = Row != nullptr ? Row->CharacterClass.LoadSynchronous() : nullptr;
    const AKataCharacter* CharacterDefaults = CharacterClass != nullptr ? CharacterClass->GetDefaultObject<AKataCharacter>() : nullptr;
    const USkeletalMeshComponent* DefaultMesh = CharacterDefaults != nullptr ? CharacterDefaults->GetMesh() : nullptr;
    if (DefaultMesh == nullptr)
    {
        CharacterPreview->SetSkinnedAssetAndUpdate(nullptr);
        CharacterPreview->SetVisibility(false);
        return;
    }

    // 행의 메시가 비어 있으면 실제 생성과 같이 캐릭터 Blueprint의 기본 메시를 쓴다.
    USkeletalMesh* PreviewMesh = Row->SkeletalMesh.LoadSynchronous();
    if (PreviewMesh == nullptr)
    {
        PreviewMesh = DefaultMesh->GetSkeletalMeshAsset();
    }
    CharacterPreview->SetSkeletalMeshAsset(PreviewMesh);
    CharacterPreview->EmptyOverrideMaterials();
    for (int32 Index = 0; Index < DefaultMesh->OverrideMaterials.Num(); ++Index)
    {
        CharacterPreview->SetMaterial(Index, DefaultMesh->OverrideMaterials[Index]);
    }

    // GetSpawnTransform_Implementation의 기준을 따른다. 영역 원점(바닥면 중심)에 영역 회전으로 놓고 스케일은 1로 둔다.
    FTransform SpawnTransform(GetActorRotation(), GetActorLocation());
    if (const UKataSpawnerComponent_SpawnArea* Settings = FindEnabledSpawnArea())
    {
        const FTransform AreaTransform = Settings->SpawnAreaTransform * GetActorTransform();
        SpawnTransform = FTransform(AreaTransform.GetRotation(), AreaTransform.GetLocation());
    }
    // 고른 위치는 발 위치이므로 실제 생성과 같이 캡슐 절반 높이만큼 올려 캡슐 중심을 구한다.
    SpawnTransform.AddToTranslation(FVector(0.0, 0.0, KataFL::GetCharacterClassHalfHeight(CharacterClass)));

    // 캐릭터의 메시는 캡슐 기준으로 내려가 있고 회전되어 있으므로 Blueprint의 상대 Transform을 곱해야 실제 모습과 겹친다.
    CharacterPreview->SetWorldTransform(DefaultMesh->GetRelativeTransform() * SpawnTransform);

    // 캐릭터의 Anim Blueprint는 소유자 캐릭터와 Linked Anim Layer를 전제로 하므로 스포너에서 실행하지 않는다.
    // 대신 OverrideAnimationData로 단일 노드 모드에서 행의 Preview Pose 첫 프레임을 정지 상태로 평가한다.
    // 이 값은 컴포넌트에 직렬화되므로 Construction 중에도 안전하다. 비어 있으면 재생할 애니메이션이 없어 기본 포즈가 된다.
    // 메시가 같으면 SetSkeletalMeshAsset이 애니메이션을 다시 초기화하지 않으므로, Tick 없이 포즈를 갱신하도록 직접 초기화한다.
    CharacterPreview->OverrideAnimationData(Row->PreviewPose.LoadSynchronous(), false, false, 0.f);
    CharacterPreview->InitAnim(true);
    CharacterPreview->SetVisibility(PreviewMesh != nullptr);

    if (PreviewMesh != nullptr)
    {
        KataCharacterSpawnerPreviewPrivate::BuildEquipmentPreview(*this, *CharacterPreview, *Row, *CharacterDefaults);
    }
#endif
}

void AKataCharacterSpawner::UpdateSpawnAreaPreview()
{
#if WITH_EDITORONLY_DATA
    if (SpawnAreaPreview == nullptr || CylinderAreaPreview == nullptr)
    {
        return;
    }

    const UKataSpawnerComponent_SpawnArea* Settings = FindEnabledSpawnArea();
    if (Settings == nullptr)
    {
        // 설정이 없으면 액터 위치 한 곳에서만 생성하므로 보여 줄 영역이 없다.
        SpawnAreaPreview->SetVisibility(false);
        CylinderAreaPreview->SetVisibility(false);
        return;
    }

    // GetSpawnTransform_Implementation과 같은 기준을 쓴다. 상대 Transform의 스케일은 영역 크기에 곱해진다.
    const bool bCylinder = Settings->AreaShape == EKataSpawnAreaShape::Cylinder;
    const FVector BoxExtent = Settings->BoxExtent.ComponentMax(FVector::ZeroVector);
    // 엔진 상자 컴포넌트는 중심 기준이므로 영역 로컬 Z로 절반 높이만큼 올려 바닥면을 영역 원점에 맞춘다.
    SpawnAreaPreview->SetRelativeTransform(FTransform(FVector(0.0, 0.0, BoxExtent.Z)) * Settings->SpawnAreaTransform);
    SpawnAreaPreview->SetBoxExtent(BoxExtent, false);
    SpawnAreaPreview->SetVisibility(!bCylinder);
    CylinderAreaPreview->SetRelativeTransform(Settings->SpawnAreaTransform);
    CylinderAreaPreview->SetCylinderSize(Settings->CylinderRadius, Settings->CylinderHalfHeight);
    CylinderAreaPreview->SetVisibility(bCylinder);
#endif
}
