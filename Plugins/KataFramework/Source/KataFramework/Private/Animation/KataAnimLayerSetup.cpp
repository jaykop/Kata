#include "Animation/KataAnimLayerSetup.h"

#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "KataFrameworkLog.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "KataAnimLayerSetup"

TSubclassOf<UAnimInstance> UKataAnimLayerSetup::ResolveWeaponLayer(const FGameplayTag& EquipmentType) const
{
    if (!EquipmentType.IsValid())
    {
        return DefaultWeaponLayer;
    }
    if (const TSubclassOf<UAnimInstance>* Found = WeaponLayers.Find(EquipmentType))
    {
        return *Found;
    }

    UE_LOG(LogKataFramework, Warning, TEXT("Anim layer setup %s has no weapon layer for %s. Using the default weapon layer."),
        *GetName(), *EquipmentType.ToString());
    return DefaultWeaponLayer;
}

bool UKataAnimLayerSetup::IsCompatibleWith(const USkeletalMeshComponent* Mesh) const
{
    if (Skeleton == nullptr)
    {
        return true;
    }
    const USkeletalMesh* MeshAsset = Mesh != nullptr ? Mesh->GetSkeletalMeshAsset() : nullptr;
    return MeshAsset != nullptr && MeshAsset->GetSkeleton() == Skeleton;
}

TSubclassOf<UAnimInstance> UKataAnimLayerSetup::LinkLayers(USkeletalMeshComponent* Mesh, const FGameplayTag& EquipmentType, TSubclassOf<UAnimInstance> PreviousWeaponLayer) const
{
    if (Mesh == nullptr || Mesh->GetAnimInstance() == nullptr)
    {
        return nullptr;
    }
    if (!IsCompatibleWith(Mesh))
    {
        // 다른 스켈레톤용 레이어를 링크하면 포즈가 깨지므로 아무것도 바꾸지 않는다.
        UE_LOG(LogKataFramework, Warning, TEXT("Anim layer setup %s targets skeleton %s, but %s uses %s. Layers are not linked."),
            *GetName(), *GetNameSafe(Skeleton), *GetNameSafe(Mesh->GetOwner()),
            *GetNameSafe(Mesh->GetSkeletalMeshAsset() != nullptr ? Mesh->GetSkeletalMeshAsset()->GetSkeleton() : nullptr));
        return nullptr;
    }

    if (BodyLayer != nullptr)
    {
        Mesh->LinkAnimClassLayers(BodyLayer);
    }

    // 같은 인터페이스를 구현한 레이어를 링크하면 이전 레이어를 대체한다. 새 레이어가 없을 때만 직접 끊는다.
    const TSubclassOf<UAnimInstance> WeaponLayer = ResolveWeaponLayer(EquipmentType);
    if (WeaponLayer != nullptr)
    {
        Mesh->LinkAnimClassLayers(WeaponLayer);
    }
    else if (PreviousWeaponLayer != nullptr)
    {
        Mesh->UnlinkAnimClassLayers(PreviousWeaponLayer);
    }
    return WeaponLayer;
}

#if WITH_EDITOR
EDataValidationResult UKataAnimLayerSetup::IsDataValid(FDataValidationContext& Context) const
{
    Super::IsDataValid(Context);

    // Template 레이어 ABP는 Target Skeleton이 없어 어느 스켈레톤에나 링크할 수 있으므로 검사하지 않는다.
    auto ValidateLayer = [this, &Context](const TSubclassOf<UAnimInstance>& LayerClass, const FText& Label)
    {
        const UAnimBlueprintGeneratedClass* LayerAnimClass = Cast<UAnimBlueprintGeneratedClass>(LayerClass.Get());
        if (Skeleton == nullptr || LayerAnimClass == nullptr || LayerAnimClass->TargetSkeleton == nullptr)
        {
            return;
        }
        if (LayerAnimClass->TargetSkeleton != Skeleton)
        {
            Context.AddError(FText::Format(LOCTEXT("SkeletonMismatch", "{0} layer {1} targets skeleton {2}, but this setup targets {3}."),
                Label, FText::FromString(LayerAnimClass->GetName()),
                FText::FromString(LayerAnimClass->TargetSkeleton->GetName()), FText::FromString(Skeleton->GetName())));
        }
    };

    ValidateLayer(BodyLayer, LOCTEXT("BodyLabel", "Body"));
    ValidateLayer(DefaultWeaponLayer, LOCTEXT("DefaultWeaponLabel", "Default weapon"));
    for (const TPair<FGameplayTag, TSubclassOf<UAnimInstance>>& Pair : WeaponLayers)
    {
        ValidateLayer(Pair.Value, FText::FromString(Pair.Key.ToString()));
    }

    return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

#undef LOCTEXT_NAMESPACE
