#include "Targeting/KataTargetingComponent.h"

#include "Faction/KataFactionSettings.h"
#include "GameFramework/Actor.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Types/TargetingSystemTypes.h"

UKataTargetingComponent::UKataTargetingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

FGenericTeamId UKataTargetingComponent::GetFactionTeamId() const
{
    const UKataFactionSettings* Settings = UKataFactionSettings::Get();
    return Settings != nullptr ? Settings->GetTeamId(Faction) : FGenericTeamId::NoTeam;
}

AActor* UKataTargetingComponent::GetCurrentTarget_Implementation() const
{
    return nullptr;
}

AActor* UKataTargetingComponent::ResolveActionTarget_Implementation()
{
    return GetCurrentTarget();
}

void UKataTargetingComponent::FindTargets(const UTargetingPreset* Preset, TArray<AActor*>& OutTargets) const
{
    OutTargets.Reset();

    AActor* Owner = GetOwner();
    UTargetingSubsystem* Subsystem = UTargetingSubsystem::Get(GetWorld());
    if (Preset == nullptr || Owner == nullptr || Subsystem == nullptr)
    {
        return;
    }

    FTargetingSourceContext SourceContext;
    SourceContext.SourceActor = Owner;
    SourceContext.InstigatorActor = Owner;
    SourceContext.SourceLocation = Owner->GetActorLocation();

    // 즉시 실행 요청은 호출 안에서 모든 태스크를 끝낸다. 핸들은 암묵적으로 해제되지 않으므로 결과를 읽은 뒤 직접 해제한다.
    FTargetingRequestHandle Handle = UTargetingSubsystem::MakeTargetRequestHandle(Preset, SourceContext);
    Subsystem->ExecuteTargetingRequestWithHandle(Handle);
    Subsystem->GetTargetingResultsActors(Handle, OutTargets);
    UTargetingSubsystem::ReleaseTargetRequestHandle(Handle);

    // Preset이 자신을 제외하지 않았어도 자기 자신을 대상으로 삼지 않게 한다. 정렬 순서는 유지한다.
    OutTargets.RemoveAll([Owner](const AActor* Target)
    {
        return !IsValid(Target) || Target == Owner;
    });
}

AActor* UKataTargetingComponent::FindBestTarget(const UTargetingPreset* Preset) const
{
    TArray<AActor*> Targets;
    FindTargets(Preset, Targets);
    return Targets.Num() > 0 ? Targets[0] : nullptr;
}
