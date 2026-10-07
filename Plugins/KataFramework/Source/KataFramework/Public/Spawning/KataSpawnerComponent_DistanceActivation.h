#pragma once

#include "CoreMinimal.h"
#include "Spawning/KataSpawnerComponent.h"
#include "KataSpawnerComponent_DistanceActivation.generated.h"

/**
 * 사용 중단된 거리 설정 항목. 기존 레벨에 저장된 항목을 읽기 위해서만 남긴다.
 *
 * 거리 기반 생성은 스포너의 Activation을 Player Distance로 고르고 스포너의 SpawnDistance·DespawnDistance를 쓴다.
 * 스포너 PostLoad가 활성 항목의 거리 값을 옮기고 배열에서 제거하므로, 이 항목 자체는 생성·제거에 관여하지 않는다.
 * 사용하는 레벨을 모두 다시 저장한 뒤 클래스를 제거할 수 있다.
 */
UCLASS(EditInlineNew, DefaultToInstanced, CollapseCategories, HideDropdown, meta = (DisplayName = "Distance Activation (Deprecated)"))
class KATAFRAMEWORK_API UKataSpawnerComponent_DistanceActivation : public UKataSpawnerComponent
{
    GENERATED_BODY()

public:
    /** 이전 SpawnDistance. 스포너 PostLoad가 스포너의 SpawnDistance로 옮긴다(cm). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Distance", meta = (Units = "cm"))
    float SpawnDistance = 3000.f;

    /** 이전 DespawnDistance. 스포너 PostLoad가 스포너의 DespawnDistance로 옮긴다(cm). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Distance", meta = (Units = "cm"))
    float DespawnDistance = 4000.f;
};
