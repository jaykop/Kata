#include "Spawning/KataSpawnerComponent_NavMeshProjection.h"

#include "AI/Navigation/NavigationTypes.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "Spawning/KataCharacterSpawner.h"
#include "Spawning/KataSpawnerComponent_SpawnArea.h"

namespace KataNavMeshProjection
{
    /** 영역 크기에서 구한 탐색 범위와, 바닥면 후보에서 질의 위치를 올릴 월드 Z 높이. */
    struct FAreaQuery
    {
        FVector Extent = INVALID_NAVEXTENT;
        double RaiseZ = 0.0;
    };

    /**
     * Spawn Area 크기에서 탐색 범위를 구한다. 수평 범위는 영역의 수평 반경에 합성 Transform의 X·Y 스케일 중 큰 절댓값을 곱한다.
     * 위아래 범위와 질의 위치를 올릴 높이는 영역 높이의 절반에 합성 Transform의 Z 스케일 절댓값을 곱한다. 질의는 월드 축을 따르며 영역 회전을 반영하지 않는다.
     * 스포너나 영역이 없으면 0 범위와 0 높이를 돌려준다.
     */
    FAreaQuery GetAreaQuery(const AKataCharacterSpawner* Spawner, const UKataSpawnerComponent_SpawnArea* SpawnArea)
    {
        FAreaQuery Query;
        if (SpawnArea == nullptr || Spawner == nullptr)
        {
            return Query;
        }

        // 후보 위치는 영역 Transform과 스포너 Transform의 스케일까지 곱해 정해지므로 탐색 범위도 같은 비율로 넓힌다.
        const FKataSpawnBatchContext* Context = Spawner->GetSpawnBatchContext();
        const FTransform SpawnerTransform = Context != nullptr && Context->SpawnArea.Get() == SpawnArea
            ? Context->SpawnerTransform : Spawner->GetActorTransform();
        const FVector Scale = (SpawnArea->SpawnAreaTransform * SpawnerTransform).GetScale3D().GetAbs();
        const double Horizontal = SpawnArea->GetAreaHorizontalRadius() * FMath::Max(Scale.X, Scale.Y);
        const double HalfHeight = SpawnArea->GetAreaHalfHeight() * Scale.Z;
        Query.Extent = FVector(Horizontal, Horizontal, HalfHeight);
        // NavMesh 질의 범위는 월드 축 상자이므로 영역 회전이 기울어 있어도 월드 Z로 올린다.
        Query.RaiseZ = HalfHeight;
        return Query;
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
    const KataNavMeshProjection::FAreaQuery AreaQuery = KataNavMeshProjection::GetAreaQuery(Spawner, SpawnArea);
    const FVector QueryExtent = AreaQuery.Extent.ComponentMax(NavData->GetDefaultQueryExtent());
    const FVector QueryLocation = CandidateTransform.GetLocation() + FVector(0.0, 0.0, AreaQuery.RaiseZ);
    FNavLocation NavLocation;
    if (!NavigationSystem->ProjectPointToNavigation(QueryLocation, NavLocation, QueryExtent, NavData))
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
