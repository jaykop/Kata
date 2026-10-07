#pragma once

#include "CoreMinimal.h"
#include "Spawning/KataSpawnerComponent.h"
#include "KataSpawnerComponent_DistanceActivation.generated.h"

/**
 * 플레이어 Pawn과의 거리로 스포너의 생성·제거를 관리하게 하는 인라인 설정 객체.
 *
 * 스포너 원점이 SpawnDistance 안으로 들어오면 생성을 시작하고, 각 NPC는 자기 현재 위치가 DespawnDistance 밖이면 개별로 제거한다.
 * 원점은 DespawnDistance 밖으로 나가야 이탈로 보며, 다시 SpawnDistance 안에 들어올 때 거리로 제거한 수만큼 원점에서 새로 생성한다.
 * 생성 실패·외부 제거·사망으로 사라진 개체는 다시 생성하지 않는다. 상태는 보존하지 않으며 재생성 개체는 초기 상태다.
 *
 * 설정은 스포너 BeginPlay에서 한 번 고정하며 이후 변경은 반영하지 않는다. 활성 항목이 둘 이상이면 첫 항목만 사용한다.
 * 거리 관리 스포너는 bSpawnOnBeginPlay·bUseTimeSlicing과 무관하게 공용 분산 경로를 쓰며 수동 생성·취소·제거 호출을 거절한다.
 * 판정은 월드 관리자가 수행하며 이 객체는 값만 보관한다.
 */
UCLASS(EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Distance Activation"))
class KATAFRAMEWORK_API UKataSpawnerComponent_DistanceActivation : public UKataSpawnerComponent
{
    GENERATED_BODY()

public:
    /** 스포너 원점과 플레이어 Pawn 사이 3D 거리가 이 값 이하이면 진입으로 본다(cm). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Distance", meta = (ClampMin = "1.0", Units = "cm"))
    float SpawnDistance = 3000.f;

    /**
     * NPC 또는 스포너 원점과 플레이어 Pawn 사이 3D 거리가 이 값을 넘으면 이탈로 본다(cm).
     * 경계에서 생성·제거가 반복되지 않도록 SpawnDistance보다 커야 하며, 그렇지 않으면 스포너가 거리 관리를 시작하지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Distance", meta = (ClampMin = "1.0", Units = "cm"))
    float DespawnDistance = 4000.f;

    /** 두 거리가 유한한 양수이고 DespawnDistance가 SpawnDistance보다 크면 true다. */
    bool HasValidDistances() const;
};
