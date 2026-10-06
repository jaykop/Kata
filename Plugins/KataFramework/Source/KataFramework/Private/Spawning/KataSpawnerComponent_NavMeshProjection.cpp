#include "Spawning/KataSpawnerComponent_NavMeshProjection.h"

#include "AI/Navigation/NavigationTypes.h"
#include "NavigationSystem.h"
#include "Spawning/KataCharacterSpawner.h"
#include "Spawning/KataSpawnerComponent_SpawnArea.h"

namespace KataNavMeshProjection
{
    /** Spawn Area 영역 크기에서 탐색 범위를 구한다. 영역이 없거나 크기가 0이면 NavMesh 기본 범위를 쓰도록 INVALID_NAVEXTENT를 돌려준다. */
    FVector GetQueryExtent(const AKataCharacterSpawner* Spawner, const UKataSpawnerComponent_SpawnArea* SpawnArea)
    {
        if (SpawnArea == nullptr || Spawner == nullptr)
        {
            return INVALID_NAVEXTENT;
        }

        const double AreaSize = SpawnArea->AreaShape == EKataSpawnAreaShape::Sphere
            ? static_cast<double>(SpawnArea->SphereRadius)
            : SpawnArea->BoxExtent.GetMax();
        // 후보 위치는 영역 Transform과 스포너 Transform의 스케일까지 곱해 정해지므로 탐색 범위도 같은 비율로 넓힌다.
        const FKataSpawnBatchContext* Context = Spawner->GetSpawnBatchContext();
        const FTransform SpawnerTransform = Context != nullptr && Context->SpawnArea.Get() == SpawnArea
            ? Context->SpawnerTransform : Spawner->GetActorTransform();
        const double Scale = (SpawnArea->SpawnAreaTransform * SpawnerTransform).GetMaximumAxisScale();
        const double Extent = AreaSize * Scale;
        return Extent > UE_KINDA_SMALL_NUMBER ? FVector(Extent) : INVALID_NAVEXTENT;
    }
}

bool UKataSpawnerComponent_NavMeshProjection::AdjustSpawnTransform_Implementation(AKataCharacterSpawner* Spawner,
    const UKataSpawnerComponent_SpawnArea* SpawnArea, int32 SpawnIndex, const FTransform& CandidateTransform, FTransform& OutTransform) const
{
    UWorld* World = Spawner != nullptr ? Spawner->GetWorld() : nullptr;
    UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (NavigationSystem == nullptr)
    {
        return false;
    }

    FNavLocation NavLocation;
    if (!NavigationSystem->ProjectPointToNavigation(CandidateTransform.GetLocation(), NavLocation,
        KataNavMeshProjection::GetQueryExtent(Spawner, SpawnArea)))
    {
        return false;
    }

    OutTransform = CandidateTransform;
    OutTransform.SetLocation(NavLocation.Location + FVector(0.0, 0.0, HeightOffset));
    return true;
}

int32 UKataSpawnerComponent_NavMeshProjection::GetPlacementAttempts_Implementation() const
{
    return FMath::Max(MaxAttempts, 1);
}
