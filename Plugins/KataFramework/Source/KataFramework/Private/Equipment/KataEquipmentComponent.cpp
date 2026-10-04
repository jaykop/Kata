#include "Equipment/KataEquipmentComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Equipment/KataEquipmentRow.h"
#include "Equipment/KataEquipmentSetup.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "KataFrameworkLog.h"

UKataEquipmentComponent::UKataEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UKataEquipmentComponent::Equip(FKataEquipmentId EquipmentId, FGameplayTag TargetSlot)
{
    const UDataTable* Table = nullptr;
    const FKataEquipmentRow* FoundRow = EquipmentId.Find(&Table);
    if (FoundRow == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Equip failed: equipment %s not found in the data collection."), *EquipmentId.ToString());
        return false;
    }

    // 대상 슬롯을 비우면 기본 슬롯을, 기본 슬롯이 없거나 장비가 허용하지 않으면 첫 허용 슬롯을 쓴다.
    if (!TargetSlot.IsValid())
    {
        const FGameplayTag DefaultSlot = EquipmentSetup != nullptr ? EquipmentSetup->DefaultSlot : FGameplayTag();
        TargetSlot = DefaultSlot.IsValid() && FoundRow->AllowedSlots.HasTagExact(DefaultSlot) ? DefaultSlot : FoundRow->AllowedSlots.First();
    }
    const FGameplayTagContainer OccupiedSlots = FoundRow->GetOccupiedSlots(TargetSlot);
    if (OccupiedSlots.IsEmpty())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Equip failed: equipment %s does not allow slot %s."),
            *EquipmentId.ToString(), *TargetSlot.ToString());
        return false;
    }

    // 같은 슬롯에 걸린 이전 요청은 결과가 곧바로 덮이므로 로드를 기다리지 않는다.
    CancelPendingEquips(OccupiedSlots);

    const uint32 RequestId = ++LastRequestId == 0 ? ++LastRequestId : LastRequestId;
    FPendingEquip& Pending = PendingEquips.Add(RequestId);
    Pending.EquipmentId = EquipmentId;
    Pending.TargetSlot = TargetSlot;
    Pending.OccupiedSlots = OccupiedSlots;
    Pending.RowData.InitializeAs(Table->GetRowStruct(), reinterpret_cast<const uint8*>(FoundRow));

    TArray<FSoftObjectPath> AssetsToLoad;
    Pending.RowData.Get<FKataEquipmentRow>().GatherAssetsToLoad(AssetsToLoad);
    if (AssetsToLoad.IsEmpty())
    {
        HandleAssetsLoaded(RequestId);
        return true;
    }

    TSharedPtr<FStreamableHandle> Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(AssetsToLoad,
        FStreamableDelegate::CreateWeakLambda(this, [this, RequestId]()
        {
            HandleAssetsLoaded(RequestId);
        }),
        FStreamableManager::DefaultAsyncLoadPriority, false, false,
        FString::Printf(TEXT("KataEquip %s"), *EquipmentId.ToString()));

    // 이미 로드된 에셋이면 RequestAsyncLoad 안에서 콜백이 끝나 요청이 사라졌을 수 있다.
    if (FPendingEquip* StillPending = PendingEquips.Find(RequestId))
    {
        if (!Handle.IsValid())
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Equip failed: could not start loading assets for equipment %s."), *EquipmentId.ToString());
            PendingEquips.Remove(RequestId);
            return false;
        }
        StillPending->LoadHandle = Handle;
    }
    return true;
}

void UKataEquipmentComponent::HandleAssetsLoaded(uint32 RequestId)
{
    FPendingEquip Pending;
    if (!PendingEquips.RemoveAndCopyValue(RequestId, Pending))
    {
        return;
    }

    ApplyEquip(Pending);

    // 적용된 에셋은 부품 메시 컴포넌트와 활성 GE가 참조하므로 로드 핸들을 놓는다.
    if (Pending.LoadHandle.IsValid())
    {
        Pending.LoadHandle->ReleaseHandle();
    }
}

void UKataEquipmentComponent::ApplyEquip(const FPendingEquip& Pending)
{
    const FKataEquipmentRow& Row = Pending.RowData.Get<FKataEquipmentRow>();

    // 지정했지만 로드되지 않은 에셋이 있으면 일부만 장착하지 않고 실패로 처리한다.
    for (const FKataEquipmentPart& Part : Row.Parts)
    {
        if (!Part.Mesh.IsNull() && Part.Mesh.Get() == nullptr)
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Equip failed: mesh %s of equipment %s failed to load."),
                *Part.Mesh.ToString(), *Pending.EquipmentId.ToString());
            OnEquipFailed.Broadcast(Pending.EquipmentId);
            return;
        }
    }
    for (const TSoftClassPtr<UGameplayEffect>& Effect : Row.GrantedEffects)
    {
        if (!Effect.IsNull() && Effect.Get() == nullptr)
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Equip failed: effect %s of equipment %s failed to load."),
                *Effect.ToString(), *Pending.EquipmentId.ToString());
            OnEquipFailed.Broadcast(Pending.EquipmentId);
            return;
        }
    }

    // 점유할 슬롯을 쓰던 장비를 먼저 해제한다. 해제 알림 안에서 다른 장비가 바뀌어도 인덱스가 어긋나지 않게 뒤에서부터 찾는다.
    for (int32 Index = EquippedItems.Num() - 1; Index >= 0; --Index)
    {
        if (EquippedItems.IsValidIndex(Index) && EquippedItems[Index].OccupiedSlots.HasAnyExact(Pending.OccupiedSlots))
        {
            RemoveEquippedItem(Index, true);
        }
    }

    FKataEquippedItem Item;
    Item.EquipmentId = Pending.EquipmentId;
    Item.TargetSlot = Pending.TargetSlot;
    Item.OccupiedSlots = Pending.OccupiedSlots;

    AActor* Owner = GetOwner();
    USceneComponent* AttachParent = GetAttachParent();
    for (const FKataEquipmentPart& Part : Row.Parts)
    {
        UObject* MeshAsset = Part.Mesh.Get();
        if (MeshAsset == nullptr || Owner == nullptr || AttachParent == nullptr)
        {
            continue;
        }

        UMeshComponent* MeshComponent = nullptr;
        if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(MeshAsset))
        {
            UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
            StaticMeshComponent->SetStaticMesh(StaticMesh);
            MeshComponent = StaticMeshComponent;
        }
        else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(MeshAsset))
        {
            USkeletalMeshComponent* SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(Owner, NAME_None, RF_Transient);
            SkeletalMeshComponent->SetSkeletalMeshAsset(SkeletalMesh);
            MeshComponent = SkeletalMeshComponent;
        }
        if (MeshComponent == nullptr)
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Equipment %s has a part whose mesh %s is neither a Static Mesh nor a Skeletal Mesh."),
                *Pending.EquipmentId.ToString(), *GetNameSafe(MeshAsset));
            continue;
        }

        // 판정은 Hit Trace가 맡으므로 장비 메시에는 충돌과 내비게이션 영향을 켜지 않는다.
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        MeshComponent->SetGenerateOverlapEvents(false);
        MeshComponent->SetCanEverAffectNavigation(false);
        MeshComponent->RegisterComponent();

        const FGameplayTag PartSlot = Part.Slot.IsValid() ? Part.Slot : Pending.TargetSlot;
        const FName Socket = EquipmentSetup != nullptr ? EquipmentSetup->SlotSockets.FindRef(PartSlot) : NAME_None;
        if (Socket != NAME_None && !AttachParent->DoesSocketExist(Socket))
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Equipment %s: socket %s for slot %s does not exist on %s. Attaching to its origin."),
                *Pending.EquipmentId.ToString(), *Socket.ToString(), *PartSlot.ToString(), *GetNameSafe(AttachParent));
        }
        MeshComponent->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
        MeshComponent->SetRelativeTransform(Part.RelativeTransform);
        Item.MeshComponents.Add(MeshComponent);
    }

    UAbilitySystemComponent* AbilitySystem = GetOwnerAbilitySystem();
    if (AbilitySystem != nullptr)
    {
        for (const TSoftClassPtr<UGameplayEffect>& Effect : Row.GrantedEffects)
        {
            UClass* EffectClass = Effect.Get();
            if (EffectClass == nullptr)
            {
                continue;
            }
            const FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(EffectClass, 1.f, AbilitySystem->MakeEffectContext());
            if (Spec.IsValid())
            {
                // 즉시형 GE는 핸들이 남지 않는다. 해제할 때 되돌릴 대상은 지속형뿐이다.
                const FActiveGameplayEffectHandle Handle = AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
                if (Handle.IsValid())
                {
                    Item.EffectHandles.Add(Handle);
                }
            }
        }
        if (!Row.GrantedTags.IsEmpty())
        {
            AbilitySystem->AddLooseGameplayTags(Row.GrantedTags);
            Item.GrantedTags = Row.GrantedTags;
        }
    }
    else if (!Row.GrantedEffects.IsEmpty() || !Row.GrantedTags.IsEmpty())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Equipment %s grants effects or tags, but %s has no Ability System Component."),
            *Pending.EquipmentId.ToString(), *GetNameSafe(Owner));
    }

    EquippedItems.Add(MoveTemp(Item));
    OnEquipped.Broadcast(Pending.EquipmentId, Pending.OccupiedSlots);
}

void UKataEquipmentComponent::SetEquipmentSetup(UKataEquipmentSetup* InSetup)
{
    EquipmentSetup = InSetup;
}

void UKataEquipmentComponent::SetStartingEquipment(const TArray<FKataStartingEquipment>& InStartingEquipment)
{
    StartingEquipment = InStartingEquipment;
}

void UKataEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();

    for (const FKataStartingEquipment& Entry : StartingEquipment)
    {
        if (Entry.EquipmentId.IsValid())
        {
            Equip(Entry.EquipmentId, Entry.Slot);
        }
    }
}

bool UKataEquipmentComponent::Unequip(FGameplayTag Slot)
{
    for (int32 Index = 0; Index < EquippedItems.Num(); ++Index)
    {
        if (EquippedItems[Index].OccupiedSlots.HasTagExact(Slot))
        {
            RemoveEquippedItem(Index, true);
            return true;
        }
    }
    return false;
}

void UKataEquipmentComponent::UnequipAll()
{
    for (TPair<uint32, FPendingEquip>& Pair : PendingEquips)
    {
        if (Pair.Value.LoadHandle.IsValid())
        {
            Pair.Value.LoadHandle->CancelHandle();
        }
    }
    PendingEquips.Empty();

    for (int32 Index = EquippedItems.Num() - 1; Index >= 0; --Index)
    {
        if (EquippedItems.IsValidIndex(Index))
        {
            RemoveEquippedItem(Index, true);
        }
    }
}

FKataEquipmentId UKataEquipmentComponent::GetEquipmentInSlot(FGameplayTag Slot) const
{
    for (const FKataEquippedItem& Item : EquippedItems)
    {
        if (Item.OccupiedSlots.HasTagExact(Slot))
        {
            return Item.EquipmentId;
        }
    }
    return FKataEquipmentId();
}

void UKataEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 소유자가 사라지는 중이므로 알림 없이 자원만 되돌린다.
    for (TPair<uint32, FPendingEquip>& Pair : PendingEquips)
    {
        if (Pair.Value.LoadHandle.IsValid())
        {
            Pair.Value.LoadHandle->CancelHandle();
        }
    }
    PendingEquips.Empty();

    for (int32 Index = EquippedItems.Num() - 1; Index >= 0; --Index)
    {
        RemoveEquippedItem(Index, false);
    }

    Super::EndPlay(EndPlayReason);
}

void UKataEquipmentComponent::RemoveEquippedItem(int32 Index, bool bBroadcast)
{
    if (!EquippedItems.IsValidIndex(Index))
    {
        return;
    }

    // 알림 안에서 배열이 바뀔 수 있으므로 먼저 꺼내 둔다.
    FKataEquippedItem Item = MoveTemp(EquippedItems[Index]);
    EquippedItems.RemoveAt(Index);

    for (const TObjectPtr<UMeshComponent>& MeshComponent : Item.MeshComponents)
    {
        if (IsValid(MeshComponent))
        {
            MeshComponent->DestroyComponent();
        }
    }

    if (UAbilitySystemComponent* AbilitySystem = GetOwnerAbilitySystem())
    {
        for (const FActiveGameplayEffectHandle& Handle : Item.EffectHandles)
        {
            AbilitySystem->RemoveActiveGameplayEffect(Handle);
        }
        if (!Item.GrantedTags.IsEmpty())
        {
            AbilitySystem->RemoveLooseGameplayTags(Item.GrantedTags);
        }
    }

    if (bBroadcast)
    {
        OnUnequipped.Broadcast(Item.EquipmentId, Item.OccupiedSlots);
    }
}

void UKataEquipmentComponent::CancelPendingEquips(const FGameplayTagContainer& Slots)
{
    for (auto It = PendingEquips.CreateIterator(); It; ++It)
    {
        if (It->Value.OccupiedSlots.HasAnyExact(Slots))
        {
            if (It->Value.LoadHandle.IsValid())
            {
                It->Value.LoadHandle->CancelHandle();
            }
            It.RemoveCurrent();
        }
    }
}

USceneComponent* UKataEquipmentComponent::GetAttachParent() const
{
    const AActor* Owner = GetOwner();
    if (const ACharacter* Character = Cast<ACharacter>(Owner))
    {
        return Character->GetMesh();
    }
    return Owner != nullptr ? Owner->GetRootComponent() : nullptr;
}

UAbilitySystemComponent* UKataEquipmentComponent::GetOwnerAbilitySystem() const
{
    return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
}
