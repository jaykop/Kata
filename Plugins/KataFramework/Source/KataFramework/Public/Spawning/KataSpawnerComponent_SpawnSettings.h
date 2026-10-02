#pragma once

#include "CoreMinimal.h"
#include "Spawning/KataSpawnerComponent.h"
#include "Engine/EngineTypes.h"
#include "KataSpawnerComponent_SpawnSettings.generated.h"

/** 생성 개체 수와 스포너 기준 Box 영역을 함께 설정하는 인라인 객체. */
UCLASS(EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Spawn Settings"))
class KATAFRAMEWORK_API UKataSpawnerComponent_SpawnSettings : public UKataSpawnerComponent
{
    GENERATED_BODY()

public:
    /** 한 번의 명시적 생성 요청에서 만들 수. 0이면 생성 없이 완료한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Count", meta = (ClampMin = "0", UIMin = "0"))
    int32 SpawnCount = 1;

    /** 스포너 액터를 기준으로 한 영역의 상대 위치·회전·스케일. 캐릭터의 스케일을 바꾸지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
    FTransform SpawnAreaTransform = FTransform::Identity;

    /** 영역의 로컬 반경, cm. Z가 0이면 영역 원점을 지나는 평면에서 위치를 고른다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "0.0", UIMin = "0.0"))
    FVector BoxExtent = FVector(200.0, 200.0, 0.0);

    /** 후보 위치가 막혔을 때 엔진의 생성 충돌 처리 방식. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision")
    ESpawnActorCollisionHandlingMethod CollisionHandling = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    /** 이번 요청의 수량을 계산한다. 설정을 변경하지 않으며 음수는 설정 오류다. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kata|Spawning")
    int32 CalculateSpawnCount() const;
    virtual int32 CalculateSpawnCount_Implementation() const;

    /**
     * 요청 당시 스포너 Transform과 설정의 상대 영역에서 후보 위치를 선택한다.
     * SpawnIndex는 이번 요청 안에서 0부터 시작한다. 지면·NavMesh·개체 간격은 보장하지 않는다.
     * 위치를 제공할 수 없으면 false다. 기본 구현은 Box 내부를 균일하게 표본 추출한다.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kata|Spawning")
    bool GetSpawnTransform(const FTransform& SpawnerTransform, int32 SpawnIndex, FTransform& OutTransform) const;
    virtual bool GetSpawnTransform_Implementation(const FTransform& SpawnerTransform, int32 SpawnIndex, FTransform& OutTransform) const;
};
