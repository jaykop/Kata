#pragma once

#include "CoreMinimal.h"
#include "Spawning/KataSpawnerComponent.h"
#include "KataSpawnerComponent_NavMeshProjection.generated.h"

/**
 * 생성 후보 위치를 NavMesh 위의 가장 가까운 위치로 옮기는 인라인 설정 객체.
 *
 * 탐색 범위는 같은 스포너의 Spawn Area 영역 크기를 따른다. Sphere면 반지름, Box면 가장 긴 Extent를 쓰고,
 * 영역 Transform과 배치 시작 시 스포너의 가장 큰 축 스케일을 곱한다. 배치 밖의 직접 호출은 현재 Transform을 쓴다.
 * 탐색 범위는 축마다 기본 NavMesh 데이터의 Default Query Extent보다 작아지지 않는다. Spawn Area가 없으면 그 기본 범위를 그대로 쓴다.
 * 기본 NavMesh 데이터가 없으면 후보를 거절한다.
 * 탐색 범위 안에 NavMesh가 없으면 후보를 거절하고, 스포너가 Spawn Area에서 후보를 다시 뽑는다.
 * MaxAttempts번 모두 실패한 개체는 생성하지 않고 실패 이벤트로 알린다.
 */
UCLASS(EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Nav Mesh Projection"))
class KATAFRAMEWORK_API UKataSpawnerComponent_NavMeshProjection : public UKataSpawnerComponent
{
    GENERATED_BODY()

public:
    /** 개체 하나의 위치를 정하려고 후보를 뽑아 투영하는 최대 횟수. 1이면 다시 뽑지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation", meta = (ClampMin = "1", UIMin = "1", UIMax = "20"))
    int32 MaxAttempts = 5;

    /**
     * 투영한 NavMesh 위치에서 월드 Z 방향으로 더할 높이(cm). 결과는 발 위치이며, 스포너가 생성할 때 캡슐 절반 높이를 따로 더한다.
     * 0이면 NavMesh 표면을 발 위치 기준으로 쓴다. 캡슐 절반 높이를 넣으면 그만큼 높은 위치를 요청한다.
     * 최종 위치와 생성 성공 여부는 Collision Handling과 주변 충돌에도 영향을 받는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float HeightOffset = 0.f;

    virtual bool AdjustSpawnTransform_Implementation(AKataCharacterSpawner* Spawner, const UKataSpawnerComponent_SpawnArea* SpawnArea,
        int32 SpawnIndex, const FTransform& CandidateTransform, FTransform& OutTransform) const override;
    virtual int32 GetPlacementAttempts_Implementation() const override;
};
