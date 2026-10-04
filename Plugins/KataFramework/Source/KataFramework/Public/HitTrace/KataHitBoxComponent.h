#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "HitTrace/KataHitTraceTypes.h"
#include "KataHitBoxComponent.generated.h"

class UAnimMontage;
class UMeshComponent;
class USkeletalMeshComponent;

/**
 * 공격 판정의 기준 메시를 제공하고, 판정이 시작되는 프레임을 위해 직전 프레임 포즈를 기록하는 컴포넌트.
 *
 * Character 기준 메시는 지정하지 않으면 소유 ACharacter의 Mesh를 쓴다. 맨몸 공격은 이 메시의 손·발 소켓으로 판정한다.
 * Weapon 기준 메시는 장비 슬롯 태그를 키로 등록한다. UKataEquipmentComponent가 장비를 장착·해제할 때 자동으로 등록·해제하며,
 * 장비 시스템을 쓰지 않는 액터는 RegisterEquipmentMesh를 직접 호출한다. 무기가 없어도 Character 기준 판정은 그대로 쓸 수 있다.
 *
 * 포즈 기록은 UKataHitSubsystem이 매 프레임 포즈 확정 뒤에 호출한다. 판정 태스크는 구간 시작 시각에 처음 생기므로
 * 그 직전 프레임을 스스로 기록할 수 없다. 액터와 수명이 같은 이 컴포넌트가 계속 기록해 두어 첫 프레임 구간도 쓸어 판정한다.
 */
UCLASS(ClassGroup = (Kata), meta = (BlueprintSpawnableComponent, DisplayName = "Kata Hit Box"))
class KATAFRAMEWORK_API UKataHitBoxComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKataHitBoxComponent();

    /**
     * 기준 종류에 해당하는 메시를 반환한다.
     * Weapon이면 WeaponSlot에 등록한 장비 메시를 돌려준다. WeaponSlot을 비우면 등록한 장비 메시가 하나일 때만 그 메시를, 없거나 둘 이상이면 nullptr을 돌려준다.
     */
    UFUNCTION(BlueprintPure, Category = "Kata|Hit Box")
    UMeshComponent* GetHitBoxMesh(EKataHitBoxMeshSource Source, UPARAM(meta = (Categories = "Equipment.Slot")) FGameplayTag WeaponSlot) const;

    /** 등록한 장비 메시 수. 이미 파괴된 메시는 세지 않는다. */
    int32 GetEquipmentMeshCount() const;

    /**
     * Character 기준 메시를 지정한다. nullptr이면 소유 ACharacter의 Mesh로 되돌린다.
     * 바꾼 직후 한 프레임은 직전 포즈가 없어 판정 첫 구간 대신 시작 시점 판정만 한다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Hit Box")
    void SetCharacterMesh(UMeshComponent* InMesh);

    /**
     * 장비 슬롯에 Weapon 기준 메시를 등록한다. 같은 슬롯에 이미 있으면 바꾼다. 무기 소켓은 이 메시에서 찾는다.
     * 등록하거나 바꾼 직후 한 프레임은 직전 포즈가 없어 판정 첫 구간 대신 시작 시점 판정만 한다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Hit Box")
    void RegisterEquipmentMesh(UPARAM(meta = (Categories = "Equipment.Slot")) FGameplayTag Slot, UMeshComponent* Mesh);

    /** 슬롯에 등록한 메시가 Mesh이면 등록을 지운다. 다른 메시로 이미 바뀌었으면 아무 일도 하지 않는다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Hit Box")
    void UnregisterEquipmentMesh(UPARAM(meta = (Categories = "Equipment.Slot")) FGameplayTag Slot, UMeshComponent* Mesh);

    /**
     * 기록해 둔 직전 프레임 포즈로 소켓의 월드 트랜스폼을 계산한다.
     *
     * 스켈레탈 메시는 엔진이 보관하는 직전 프레임 본 트랜스폼과 기록한 메시 트랜스폼을 조합한다.
     * 그 버퍼는 이번 프레임에 애니메이션 갱신이 정확히 한 번 있었을 때만 직전 프레임 값이므로
     * 본 트랜스폼 리비전이 기록 시점보다 1만큼 늘었을 때만 유효로 본다. 스폰·텔레포트 직후나 갱신을 건너뛴 프레임은 실패한다.
     *
     * @param CurrentTick UKataHitSubsystem의 현재 Tick 번호. 기록이 바로 앞 Tick의 것이어야 한다.
     * @return 계산에 성공하면 true. 실패하면 호출자는 첫 구간을 쓸지 않고 시작 시점 판정으로 대신한다.
     */
    bool GetPreviousSocketTransform(const UMeshComponent* Mesh, FName SocketName, uint64 CurrentTick, FTransform& OutTransform) const;

    /**
     * 기록해 둔 직전 프레임의 활성 몽타주 재생 상태와 메시 월드 트랜스폼을 돌려준다. 프레임 사이 포즈 재샘플링의 첫 프레임에 쓴다.
     * 기록이 바로 앞 Tick의 것이 아니거나 그때 몽타주가 없었으면 false다.
     */
    bool GetPreviousAnimationState(const USkeletalMeshComponent* Mesh, uint64 CurrentTick,
        const UAnimMontage*& OutMontage, float& OutPosition, FTransform& OutComponentToWorld) const;

    /** UKataHitSubsystem이 포즈 확정 뒤 호출한다. Character 메시와 등록한 장비 메시의 현재 상태를 기록한다. */
    void RecordPoses(uint64 TickIndex);

protected:
    // BeginPlay가 아니라 등록 시점에 Subsystem에 붙는다. 액션 에디터 프리뷰 월드의 액터는 BeginPlay를 받지 않아 포즈 기록이 빠졌다.
    virtual void OnRegister() override;
    virtual void OnUnregister() override;

private:
    /** 메시 하나의 직전 프레임 기록. */
    struct FPoseRecord
    {
        TWeakObjectPtr<const UMeshComponent> Mesh;
        FTransform ComponentToWorld;
        uint64 TickIndex = 0;
        uint32 BoneTransformRevision = 0;
        bool bValid = false;
        /** 스켈레탈 메시의 활성 몽타주와 재생 위치. */
        TWeakObjectPtr<const UAnimMontage> Montage;
        float MontagePosition = 0.0f;
    };

    /** 장비 슬롯 하나에 등록한 메시와 그 직전 프레임 기록. */
    struct FEquipmentMeshEntry
    {
        TWeakObjectPtr<UMeshComponent> Mesh;
        FPoseRecord Record;
    };

    void RecordPose(FPoseRecord& Record, const UMeshComponent* Mesh, uint64 TickIndex) const;

    /** Mesh의 기록을 찾는다. Character 기록과 장비 기록을 모두 본다. */
    const FPoseRecord* FindRecord(const UMeshComponent* Mesh) const;

    /** 지정하지 않으면 소유 ACharacter의 Mesh를 쓴다. */
    UPROPERTY(Transient)
    TWeakObjectPtr<UMeshComponent> CharacterMesh;

    FPoseRecord CharacterRecord;

    /** 장비 슬롯별 Weapon 기준 메시. 약참조라 장비 메시가 파괴되면 저절로 무효가 된다. */
    TMap<FGameplayTag, FEquipmentMeshEntry> EquipmentMeshes;
};
