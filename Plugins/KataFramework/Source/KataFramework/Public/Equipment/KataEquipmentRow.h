#pragma once

#include "CoreMinimal.h"
#include "Data/KataRowBase.h"
#include "Data/KataRowId.h"
#include "GameplayTagContainer.h"
#include "KataEquipmentRow.generated.h"

class UGameplayEffect;

/**
 * 장비가 장착될 때 만드는 메시 부품 하나.
 *
 * Slot이 비어 있으면 장착 대상 슬롯에 붙는다. Slot을 지정하면 그 슬롯에 붙고 장비가 그 슬롯도 점유한다.
 * 쌍검처럼 한 장비가 여러 슬롯을 쓰는 경우 부품마다 슬롯을 지정한다.
 */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataEquipmentPart
{
    GENERATED_BODY()

    /** 부품 메시. Static Mesh 또는 Skeletal Mesh다. 비어 있으면 이 부품은 메시를 만들지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Equipment", meta = (AllowedClasses = "/Script/Engine.StaticMesh,/Script/Engine.SkeletalMesh"))
    TSoftObjectPtr<UObject> Mesh;

    /** 부착할 슬롯. 비우면 장착 대상 슬롯에 붙는다. 선택기는 Equipment.Slot 아래 태그만 보여 준다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Equipment", meta = (Categories = "Equipment.Slot"))
    FGameplayTag Slot;

    /** 슬롯 소켓을 기준으로 한 부품의 상대 Transform. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Equipment")
    FTransform RelativeTransform = FTransform::Identity;
};

/** 캐릭터가 시작할 때 장착할 장비 하나. 캐릭터 행의 Starting Equipment 항목이다. */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataStartingEquipment
{
    GENERATED_BODY()

    /** 장착할 장비. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Equipment")
    FKataEquipmentId EquipmentId;

    /** 장착 대상 슬롯. 비우면 Equipment Setup의 기본 슬롯, 없으면 장비의 첫 허용 슬롯을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Equipment", meta = (Categories = "Equipment.Slot"))
    FGameplayTag Slot;
};

/**
 * 장비 데이터 테이블의 행. 행 하나가 장비 하나의 설정이며 행 이름이 장비 ID다.
 *
 * 장비는 AllowedSlots 중 하나(장착 대상 슬롯)와 부품이 지정한 슬롯을 함께 점유한다.
 * 같은 장비를 오른손과 왼손 어디에나 장착하려면 AllowedSlots에 두 슬롯을 넣고, 장착할 때 대상 슬롯을 고른다.
 * 모든 에셋 참조는 소프트 참조이며 UKataEquipmentComponent가 장착할 때 비동기로 로드한다.
 * 슬롯 태그는 플러그인이 정의하지 않는다. 프로젝트가 Equipment.Slot 아래에 정의한 태그를 쓴다(선택기가 이 루트로 거른다).
 */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataEquipmentRow : public FKataRowBase
{
    GENERATED_BODY()

    /** 장착 대상으로 고를 수 있는 슬롯. 비어 있으면 장착할 수 없다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Equipment", meta = (Categories = "Equipment.Slot"))
    FGameplayTagContainer AllowedSlots;

    /**
     * 장비의 종류(예: Equipment.Type.Sword). 아이템 분류이므로 소유자 ASC에는 넣지 않는다.
     * 캐릭터의 Anim Layer Setup이 이 태그로 무기 레이어를 고른다. 선택기는 Equipment.Type 아래 태그만 보여 준다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Equipment", meta = (Categories = "Equipment.Type"))
    FGameplayTag EquipmentType;

    /** 장착할 때 만드는 메시 부품. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Equipment")
    TArray<FKataEquipmentPart> Parts;

    /** 장착하는 동안 소유자 ASC에 적용하는 Gameplay Effect. 스탯 보정 등에 쓴다. 해제하면 제거한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Equipment")
    TArray<TSoftClassPtr<UGameplayEffect>> GrantedEffects;

    /**
     * 장착하는 동안 소유자 ASC에 더하는 루즈 태그. 장착으로 생기는 캐릭터 상태(예: Status.Wielding.Sword)를 나타내며
     * 그래프 전이 분기에 쓴다. 해제하면 뺀다. ASC에는 Status·Identity 루트만 두므로 선택기는 Status 아래 태그만 보여 준다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Equipment", meta = (Categories = "Status"))
    FGameplayTagContainer GrantedTags;

    /**
     * 장착 전에 비동기로 로드할 에셋 경로를 모은다. 비어 있는 참조는 넣지 않는다.
     * 파생 행은 Super를 호출한 뒤 자기 항목을 덧붙인다.
     */
    virtual void GatherAssetsToLoad(TArray<FSoftObjectPath>& OutPaths) const;

    /**
     * 대상 슬롯으로 장착할 때 점유하는 슬롯을 돌려준다. 대상 슬롯과 부품이 지정한 슬롯의 합이다.
     * 대상 슬롯이 AllowedSlots에 없으면 빈 컨테이너를 돌려준다.
     */
    FGameplayTagContainer GetOccupiedSlots(const FGameplayTag& TargetSlot) const;
};
