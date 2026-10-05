#include "Targeting/Tasks/KataTargetingTask_SelectPerceivedActors.h"

#include "GameFramework/Actor.h"
#include "Targeting/KataAITargetingComponent.h"
#include "Types/TargetingSystemTypes.h"

void UKataTargetingTask_SelectPerceivedActors::Execute(const FTargetingRequestHandle& TargetingHandle) const
{
    Super::Execute(TargetingHandle);
    SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Executing);
    if (TargetingHandle.IsValid())
    {
        FTargetingDefaultResultsSet& Results = FTargetingDefaultResultsSet::FindOrAdd(TargetingHandle);
        Results.TargetResults.Reset();
        const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
        AActor* Owner = Source != nullptr ? Source->SourceActor.Get() : nullptr;
        const UKataAITargetingComponent* Targeting = Owner != nullptr
            ? Owner->FindComponentByClass<UKataAITargetingComponent>() : nullptr;
        if (Targeting != nullptr)
        {
            TArray<AActor*> Actors;
            Targeting->GetVisibleActors(Actors);
            for (AActor* Actor : Actors)
            {
                FTargetingDefaultResultData& Result = Results.TargetResults.AddDefaulted_GetRef();
                Result.HitResult.HitObjectHandle = FActorInstanceHandle(Actor);
                Result.HitResult.Location = Actor->GetActorLocation();
                Result.HitResult.ImpactPoint = Result.HitResult.Location;
                Result.HitResult.Distance = FVector::Distance(Owner->GetActorLocation(), Result.HitResult.Location);
            }
        }
    }
    SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
}
