#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "KataAnimLayerSetup.generated.h"

class UAnimInstance;
class USkeletalMeshComponent;
class USkeleton;

/**
 * 스켈레톤에 따라 정해지는 Linked Anim Layer 설정. 같은 스켈레톤을 쓰는 캐릭터끼리 하나를 공유한다.
 *
 * Body 레이어는 발 IK처럼 무기와 무관한 처리를 담고 캐릭터가 존재하는 동안 유지된다.
 * 무기 레이어는 장비 행의 Equipment Type 태그로 고르며, 맞는 항목이 없거나 무기가 없으면 기본 무기 레이어를 쓴다.
 * 무기는 애니메이션을 직접 참조하지 않으므로 스켈레톤이 다른 캐릭터가 같은 무기를 들어도 각자 자기 설정의 레이어를 쓴다.
 * 캐릭터 행의 Anim Layer Setup이 지정하며, 비어 있으면 장착 컴포넌트의 Blueprint 기본값을 쓴다.
 * 레이어 클래스는 하드 참조이므로 이 에셋을 로드하면 모든 무기 레이어와 그 애니메이션이 함께 로드된다.
 */
UCLASS(BlueprintType)
class KATAFRAMEWORK_API UKataAnimLayerSetup : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** 이 설정이 대상으로 하는 스켈레톤. 링크 직전과 데이터 검증에서 비교한다. 비우면 검사하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Layers")
    TObjectPtr<USkeleton> Skeleton;

    /** 캐릭터가 존재하는 동안 링크해 두는 Body 레이어. 무기와 무관한 Anim Layer Interface를 구현한 레이어 ABP다. 비워도 된다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Layers")
    TSubclassOf<UAnimInstance> BodyLayer;

    /** 무기가 없거나 무기 종류에 맞는 항목이 없을 때 쓰는 무기 레이어. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Layers")
    TSubclassOf<UAnimInstance> DefaultWeaponLayer;

    /** 장비 행의 Equipment Type 태그별 무기 레이어. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Layers", meta = (Categories = "Equipment.Type"))
    TMap<FGameplayTag, TSubclassOf<UAnimInstance>> WeaponLayers;

    /**
     * 무기 종류에 맞는 무기 레이어를 돌려준다.
     * 태그가 비어 있으면 기본 무기 레이어다. 매핑에 없으면 경고 로그를 남기고 기본 무기 레이어를 돌려준다.
     */
    TSubclassOf<UAnimInstance> ResolveWeaponLayer(const FGameplayTag& EquipmentType) const;

    /** Mesh의 스켈레톤이 Skeleton과 같은지 확인한다. Skeleton이 비어 있으면 항상 true다. */
    bool IsCompatibleWith(const USkeletalMeshComponent* Mesh) const;

    /**
     * Body 레이어와 무기 레이어를 Mesh에 링크한다.
     * 스켈레톤이 맞지 않으면 경고 로그만 남기고 아무것도 링크하지 않는다. Mesh에 Anim Instance가 없어도 아무것도 하지 않는다.
     *
     * @param PreviousWeaponLayer 직전에 링크한 무기 레이어. 새 무기 레이어가 없을 때 이 레이어의 링크를 끊는다.
     * @return 실제로 링크한 무기 레이어. 링크하지 않았으면 nullptr다.
     */
    TSubclassOf<UAnimInstance> LinkLayers(USkeletalMeshComponent* Mesh, const FGameplayTag& EquipmentType, TSubclassOf<UAnimInstance> PreviousWeaponLayer) const;

#if WITH_EDITOR
    //~ Begin UObject Interface
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
    //~ End UObject Interface
#endif
};
