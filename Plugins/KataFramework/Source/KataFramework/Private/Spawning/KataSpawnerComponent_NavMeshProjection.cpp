#include "Spawning/KataSpawnerComponent_NavMeshProjection.h"

#include "AI/Navigation/NavigationTypes.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "Spawning/KataCharacterSpawner.h"
#include "Spawning/KataSpawnerComponent_SpawnArea.h"

namespace KataNavMeshProjection
{
    /** Spawn Area 영역 크기에서 탐색 범위를 구한다. 스포너나 영역이 없거나, 스케일을 적용한 크기가 UE_KINDA_SMALL_NUMBER 이하이면 INVALID_NAVEXTENT(0 벡터)를 돌려준다. */
    FVector GetAreaQueryExtent(const AKataCharacterSpawner* Spawner, const UKataSpawnerComponent_SpawnArea* SpawnArea)
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
    ANavigationData* NavData = NavigationSystem != nullptr ? NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) : nullptr;
    if (NavData == nullptr)
    {
        return false;
    }

    // NavMesh 표면과 후보 위치 사이의 높이 차이가 영역 크기보다 크면 투영이 실패할 수 있다.
    // 작은 영역에서도 이 차이를 탐색할 수 있도록 축마다 NavMesh 기본 탐색 범위를 최소값으로 둔다.
    const FVector QueryExtent = KataNavMeshProjection::GetAreaQueryExtent(Spawner, SpawnArea).ComponentMax(NavData->GetDefaultQueryExtent());
    FNavLocation NavLocation;
    if (!NavigationSystem->ProjectPointToNavigation(CandidateTransform.GetLocation(), NavLocation, QueryExtent, NavData))
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
