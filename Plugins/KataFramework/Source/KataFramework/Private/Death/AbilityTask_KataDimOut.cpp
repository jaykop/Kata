#include "Death/AbilityTask_KataDimOut.h"

#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UAbilityTask_KataDimOut::UAbilityTask_KataDimOut(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bTickingTask = true;
}

UAbilityTask_KataDimOut* UAbilityTask_KataDimOut::KataDimOut(UGameplayAbility* OwningAbility, FName ParameterName, float Duration)
{
    UAbilityTask_KataDimOut* Task = NewAbilityTask<UAbilityTask_KataDimOut>(OwningAbility);
    Task->ParameterName = ParameterName;
    Task->Duration = Duration;
    return Task;
}

void UAbilityTask_KataDimOut::Activate()
{
    AActor* Avatar = GetAvatarActor();
    if (Avatar != nullptr && !ParameterName.IsNone())
    {
        const FHashedMaterialParameterInfo ParameterInfo(ParameterName);
        TInlineComponentArray<UMeshComponent*> Meshes(Avatar);
        for (UMeshComponent* Mesh : Meshes)
        {
            for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
            {
                UMaterialInterface* Material = Mesh->GetMaterial(Index);
                float CurrentValue = 0.0f;
                // 파라미터가 없는 슬롯까지 Dynamic Material Instance로 바꾸면 배칭만 깨지고 보이는 변화는 없다.
                if (Material == nullptr || !Material->GetScalarParameterValue(ParameterInfo, CurrentValue))
                {
                    continue;
                }
                if (UMaterialInstanceDynamic* Dynamic = Mesh->CreateDynamicMaterialInstance(Index, Material))
                {
                    Materials.Add(Dynamic);
                }
            }
        }
    }

    ApplyAlpha(0.0f);
    if (Duration <= 0.0f)
    {
        ApplyAlpha(1.0f);
        if (ShouldBroadcastAbilityTaskDelegates())
        {
            OnFinished.Broadcast();
        }
        EndTask();
    }
}

void UAbilityTask_KataDimOut::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);

    Elapsed += DeltaTime;
    const float Alpha = FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f);
    ApplyAlpha(Alpha);
    if (Alpha >= 1.0f)
    {
        if (ShouldBroadcastAbilityTaskDelegates())
        {
            OnFinished.Broadcast();
        }
        EndTask();
    }
}

void UAbilityTask_KataDimOut::ApplyAlpha(float Alpha)
{
    for (UMaterialInstanceDynamic* Material : Materials)
    {
        if (Material != nullptr)
        {
            Material->SetScalarParameterValue(ParameterName, Alpha);
        }
    }
}

void UAbilityTask_KataDimOut::OnDestroy(bool bInOwnerFinished)
{
    Materials.Reset();
    Super::OnDestroy(bInOwnerFinished);
}
