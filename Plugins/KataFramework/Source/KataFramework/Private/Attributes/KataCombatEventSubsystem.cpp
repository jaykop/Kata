#include "Attributes/KataCombatEventSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

UKataCombatEventSubsystem* UKataCombatEventSubsystem::Get(const UObject* WorldContext)
{
    const UWorld* World = GEngine != nullptr ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
    return World != nullptr ? World->GetSubsystem<UKataCombatEventSubsystem>() : nullptr;
}

void UKataCombatEventSubsystem::BroadcastDamageApplied(AActor* Target, AActor* Instigator, float Damage)
{
    if (Target != nullptr && Damage > 0.0f)
    {
        OnDamageApplied.Broadcast(Target, Instigator, Damage);
    }
}
