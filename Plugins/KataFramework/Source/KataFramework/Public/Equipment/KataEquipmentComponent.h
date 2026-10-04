#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/KataRowId.h"
#include "Equipment/KataEquipmentRow.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "KataEquipmentComponent.generated.h"

class UAbilitySystemComponent;
class UKataEquipmentSetup;
class UMeshComponent;
class USceneComponent;
struct FKataEquipmentRow;
struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataEquipmentChangedSignature, const FKataEquipmentId&, EquipmentId, const FGameplayTagContainer&, Slots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKataEquipFailedSignature, const FKataEquipmentId&, EquipmentId);

/** 장착된 장비 하나의 실행 상태. 해제할 때 되돌릴 자원을 모두 보관한다. 공유 데이터인 장비 행에는 이 상태를 두지 않는다. */
USTRUCT()
struct FKataEquippedItem
{
    GENERATED_BODY()

    UPROPERTY()
    FKataEquipmentId EquipmentId;

    /** 장착 대상 슬롯. */
    UPROPERTY()
    FGameplayTag TargetSlot;

    /** 이 장비가 점유한 슬롯. 대상 슬롯과 부품 슬롯의 합이다. */
    UPROPERTY()
    FGameplayTagContainer OccupiedSlots;

    /** 장착하며 만든 부품 메시. 해제할 때 파괴한다. */
    UPROPERTY()
    TArray<TObjectPtr<UMeshComponent>> MeshComponents;

    /** 장착하며 적용한 Gameplay Effect. 지속형만 핸들이 남으며 해제할 때 제거한다. */
    UPROPERTY()
    TArray<FActiveGameplayEffectHandle> EffectHandles;

    /** 장착하며 더한 루즈 태그. 해제할 때 같은 수만큼 뺀다. */
    UPROPERTY()
    FGameplayTagContainer GrantedTags;
};

/**
 * 장비를 부위 슬롯에 장착·해제하는 컴포넌트.
 *
 * 장비 ID로 데이터 컬렉션의 장비 행을 찾아 복사하고, 부품 메시와 Gameplay Effect를 비동기로 로드한 뒤 장착한다.
 * 장착하면 점유할 슬롯의 기존 장비를 해제하고, 부품 메시를 Equipment Setup이 정한 슬롯 소켓에 붙이고, 소유자 ASC에 GE와 루즈 태그를 적용한다.
 * 캐릭터 행이 Equipment Setup과 시작 장비를 지정하면 행 적용 때 받아 두고, 시작 장비는 BeginPlay에서 장착한다.
 * 해제·EndPlay 때는 장착하며 받은 핸들로 모두 되돌린다. 언제 교체할지는 호출자(입력, AI 조건)가 정한다.
 * 부품 메시는 캐릭터면 캐릭터 Mesh, 아니면 소유자 루트 컴포넌트에 붙는다. 메시에는 충돌을 켜지 않는다.
 */
UCLASS(ClassGroup = (Kata), meta = (BlueprintSpawnableComponent, DisplayName = "Kata Equipment"))
class KATAFRAMEWORK_API UKataEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKataEquipmentComponent();

    /**
     * 슬롯→소켓 매핑과 기본 슬롯을 담은 장비 설정. 캐릭터 행이 지정하면 그 값으로 바뀐다.
     * 비어 있으면 부품은 부착 대상의 원점에 붙고, 대상 슬롯을 비운 장착은 장비의 첫 허용 슬롯을 쓴다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Equipment")
    TObjectPtr<UKataEquipmentSetup> EquipmentSetup;

    /** 장비 설정을 바꾼다. 이미 장착한 장비의 부착 위치는 바뀌지 않으며 다음 장착부터 적용된다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Equipment")
    void SetEquipmentSetup(UKataEquipmentSetup* InSetup);

    /**
     * BeginPlay에서 장착할 시작 장비를 지정한다. 캐릭터 행 적용이 호출한다.
     * BeginPlay 이후에 호출하면 저장만 하고 장착하지 않는다.
     */
    void SetStartingEquipment(const TArray<FKataStartingEquipment>& InStartingEquipment);

    /**
     * 장비를 장착하도록 요청한다. 에셋 로드가 끝나면 장착하고 OnEquipped를 알린다.
     * 에셋이 모두 로드되어 있으면 이 함수 안에서 바로 장착될 수 있다. 점유 슬롯이 겹치는 대기 요청은 취소한다.
     *
     * @param EquipmentId 데이터 컬렉션의 장비 행 ID.
     * @param TargetSlot 장착 대상 슬롯. 비우면 Equipment Setup의 기본 슬롯을 쓰고, 없거나 장비가 허용하지 않으면 첫 허용 슬롯을 쓴다.
     * @return 요청을 받으면 true. 행이 없거나 대상 슬롯을 허용하지 않으면 경고 로그와 함께 false이며 OnEquipFailed는 부르지 않는다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Equipment")
    bool Equip(FKataEquipmentId EquipmentId, UPARAM(meta = (Categories = "Equipment.Slot")) FGameplayTag TargetSlot);

    /** Slot을 점유한 장비를 해제하고 OnUnequipped를 알린다. 그 장비가 점유한 다른 슬롯도 함께 비워진다. 장비가 없으면 false다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Equipment")
    bool Unequip(UPARAM(meta = (Categories = "Equipment.Slot")) FGameplayTag Slot);

    /** 모든 장비를 해제하고 대기 중인 장착 요청을 취소한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Equipment")
    void UnequipAll();

    /** Slot을 점유한 장비의 ID. 비어 있으면 빈 ID다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Equipment")
    FKataEquipmentId GetEquipmentInSlot(UPARAM(meta = (Categories = "Equipment.Slot")) FGameplayTag Slot) const;

    /** 로드를 기다리는 장착 요청이 있으면 true다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Equipment")
    bool IsEquipPending() const { return PendingEquips.Num() > 0; }

    /** 장비를 장착했을 때 점유 슬롯과 함께 알린다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Equipment")
    FKataEquipmentChangedSignature OnEquipped;

    /** 장비를 해제했을 때 비운 슬롯과 함께 알린다. 다른 장비에 밀려 해제될 때도 알린다. EndPlay에서는 알리지 않는다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Equipment")
    FKataEquipmentChangedSignature OnUnequipped;

    /** 요청을 받은 뒤 에셋 로드에 실패해 장착하지 못했을 때 알린다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Equipment")
    FKataEquipFailedSignature OnEquipFailed;

protected:
    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    //~ End UActorComponent Interface

private:
    struct FPendingEquip
    {
        FKataEquipmentId EquipmentId;
        FGameplayTag TargetSlot;
        FGameplayTagContainer OccupiedSlots;
        /** 요청 시점에 복사한 행. 로드 중 테이블이 바뀌어도 요청은 영향을 받지 않는다. */
        FInstancedStruct RowData;
        TSharedPtr<FStreamableHandle> LoadHandle;
    };

    void HandleAssetsLoaded(uint32 RequestId);
    void ApplyEquip(const FPendingEquip& Pending);
    void RemoveEquippedItem(int32 Index, bool bBroadcast);
    void CancelPendingEquips(const FGameplayTagContainer& Slots);

    USceneComponent* GetAttachParent() const;
    UAbilitySystemComponent* GetOwnerAbilitySystem() const;

    TMap<uint32, FPendingEquip> PendingEquips;
    uint32 LastRequestId = 0;

    UPROPERTY(Transient)
    TArray<FKataEquippedItem> EquippedItems;

    /** 행이 지정한 시작 장비. BeginPlay에서 장착한다. ASC Actor Info가 준비된 뒤라야 GE를 적용할 수 있기 때문이다. */
    UPROPERTY(Transient)
    TArray<FKataStartingEquipment> StartingEquipment;
};
