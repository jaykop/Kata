#include "UI/KataVM_EnemyHealth.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Actor.h"
#include "UI/KataHUDSettings.h"

float UKataVM_EnemyHealth::GetHealthRatio() const
{
    return MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f;
}

bool UKataVM_EnemyHealth::GetBarWorldLocation(FVector& OutLocation) const
{
    OutLocation = FVector::ZeroVector;
    const AActor* Target = TargetActor.Get();
    if (Target == nullptr)
    {
        return false;
    }

    FVector Top;
    if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Target->GetRootComponent()))
    {
        Top = Capsule->GetComponentLocation() + Capsule->GetUpVector() * Capsule->GetScaledCapsuleHalfHeight();
    }
    else
    {
        FVector Origin;
        FVector Extent;
        Target->GetActorBounds(true, Origin, Extent);
        Top = Origin + FVector(0.0, 0.0, Extent.Z);
    }
    OutLocation = Top + FVector(0.0, 0.0, UKataHUDSettings::Get()->BarHeightOffset);
    return true;
}

void UKataVM_EnemyHealth::SetHealth(float NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(Health, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthRatio);
    }
}

void UKataVM_EnemyHealth::SetMaxHealth(float NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthRatio);
    }
}

void UKataVM_EnemyHealth::SetLockedOn(bool bNewValue)
{
    UE_MVVM_SET_PROPERTY_VALUE(bLockedOn, bNewValue);
}

void UKataVM_EnemyHealth::SetDead(bool bNewValue)
{
    UE_MVVM_SET_PROPERTY_VALUE(bDead, bNewValue);
}
