#pragma once

#include "CoreMinimal.h"
#include "Spawning/KataSpawnerComponent.h"
#include "Engine/EngineTypes.h"
#include "KataSpawnerComponent_SpawnArea.generated.h"

/** 스폰 영역의 모양. */
UENUM(BlueprintType)
enum class EKataSpawnAreaShape : uint8
{
    /** 반지름 SphereRadius인 구 내부. */
    Sphere,
    /** 반경 BoxExtent인 상자 내부. */
    Box
};

/** 생성 개체 수와 스포너 기준 영역(구 또는 상자)을 함께 설정하는 인라인 객체. */
UCLASS(EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Spawn Area"))
class KATAFRAMEWORK_API UKataSpawnerComponent_SpawnArea : public UKataSpawnerComponent
{
    GENERATED_BODY()

public:
    /**
     * 한 번의 생성 요청에서 만들 최소 수. 1 이상이며 MaxSpawnCount보다 클 수 없다.
     * Details에서 MaxSpawnCount보다 크게 바꾸면 MaxSpawnCount를 같은 값으로 올린다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Count", meta = (ClampMin = "1", UIMin = "1"))
    int32 MinSpawnCount = 1;

    /**
     * 한 번의 생성 요청에서 만들 최대 수. 1 이상이며 MinSpawnCount보다 작을 수 없다.
     * 두 값이 다르면 요청마다 그 사이(양 끝 포함)에서 무작위로 정한다. Details에서 MinSpawnCount보다 작게 바꾸면 MinSpawnCount를 같은 값으로 내린다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Count", meta = (ClampMin = "1", UIMin = "1"))
    int32 MaxSpawnCount = 1;

    /** 스포너 액터를 기준으로 한 영역의 상대 위치·회전·스케일. 캐릭터의 스케일을 바꾸지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
    FTransform SpawnAreaTransform = FTransform::Identity;

    /** 영역의 모양. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
    EKataSpawnAreaShape AreaShape = EKataSpawnAreaShape::Sphere;

    /** 구 영역의 반지름, cm. 구 내부의 부피에서 균일하게 위치를 고르므로 원점 아래쪽도 후보가 된다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area",
        meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "AreaShape == EKataSpawnAreaShape::Sphere", EditConditionHides))
    float SphereRadius = 200.f;

    /** 상자 영역의 로컬 반경, cm. Z가 0이면 영역 원점을 지나는 평면에서 위치를 고른다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area",
        meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "AreaShape == EKataSpawnAreaShape::Box", EditConditionHides))
    FVector BoxExtent = FVector(200.0, 200.0, 0.0);

    /** 후보 위치가 막혔을 때 엔진의 생성 충돌 처리 방식. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision")
    ESpawnActorCollisionHandlingMethod CollisionHandling = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    /**
     * 이번 요청의 수량을 계산한다. 설정을 변경하지 않으며 음수는 설정 오류다.
     * 기본 구현은 MinSpawnCount와 MaxSpawnCount 사이에서 무작위로 고르고, 두 값이 1 미만이거나 최소가 최대보다 크면 경고 후 -1을 반환한다.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kata|Spawning")
    int32 CalculateSpawnCount() const;
    virtual int32 CalculateSpawnCount_Implementation() const;

    /**
     * 요청 당시 스포너 Transform과 설정의 상대 영역에서 후보 위치를 선택한다.
     * SpawnIndex는 이번 요청 안에서 0부터 시작한다. 지면·NavMesh·개체 간격은 보장하지 않는다.
     * 위치를 제공할 수 없으면 false다. 기본 구현은 구 또는 상자 내부를 균일하게 표본 추출한다.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kata|Spawning")
    bool GetSpawnTransform(const FTransform& SpawnerTransform, int32 SpawnIndex, FTransform& OutTransform) const;
    virtual bool GetSpawnTransform_Implementation(const FTransform& SpawnerTransform, int32 SpawnIndex, FTransform& OutTransform) const;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
