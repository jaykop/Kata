#pragma once

#include "CoreMinimal.h"
#include "Spawning/KataSpawnerComponent.h"
#include "Engine/EngineTypes.h"
#include "KataSpawnerComponent_SpawnArea.generated.h"

// 값 순서는 직렬화 기본값과 맞물려 있으므로 바꾸지 않는다. 이전 이름 Sphere는 플러그인 설정의 CoreRedirects가 Cylinder로 옮긴다.
/**
 * 스폰 영역의 모양. 두 모양 모두 영역 원점이 바닥면 중심이며, 영역은 바닥면에서 위로만 쌓인다.
 * 후보 위치는 영역 바닥면에서 고른다.
 */
UENUM(BlueprintType)
enum class EKataSpawnAreaShape : uint8
{
    /** 바닥 원의 반지름이 CylinderRadius이고, 바닥에서 위로 CylinderHalfHeight의 두 배까지인 원기둥. */
    Cylinder,
    /** 원점에서 X·Y 각 면까지 거리가 BoxExtent이고, 바닥에서 위로 BoxExtent.Z의 두 배까지인 상자. */
    Box
};

/** 생성 개체 수와 스포너 기준 영역(원기둥 또는 상자)을 함께 설정하는 인라인 객체. */
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

    /** 스포너 액터를 기준으로 한 영역의 상대 위치·회전·스케일. 위치는 영역 바닥면의 중심이다. 캐릭터의 스케일을 바꾸지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
    FTransform SpawnAreaTransform = FTransform::Identity;

    /** 영역의 모양. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
    EKataSpawnAreaShape AreaShape = EKataSpawnAreaShape::Cylinder;

    // 이전 이름 SphereRadius는 CoreRedirects가 CylinderRadius로 옮긴다.
    /** 원기둥 바닥 원의 반지름, cm. 후보 위치는 이 원판 안에서 균일하게 고른다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area",
        meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "AreaShape == EKataSpawnAreaShape::Cylinder", EditConditionHides))
    float CylinderRadius = 200.f;

    /**
     * 원기둥 높이의 절반, cm. 영역은 바닥면에서 위로 이 값의 두 배까지다. 후보 위치는 항상 바닥면에 있으며,
     * 이 값은 미리보기 높이와 Nav Mesh Projection의 수직 탐색 범위에 쓰인다. NavMesh 질의 위치도 스케일을 적용한 이 값만큼 월드 Z로 올린다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area",
        meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "AreaShape == EKataSpawnAreaShape::Cylinder", EditConditionHides))
    float CylinderHalfHeight = 0.f;

    /**
     * 상자 크기, cm. X·Y는 원점에서 각 로컬 축의 면까지 거리로 전체 너비는 두 배다. Z는 높이의 절반이며 상자는 바닥면에서 위로 Z의 두 배까지다.
     * 후보 위치는 항상 바닥면에서 X·Y만 균일하게 고른다. Z는 미리보기 높이와 Nav Mesh Projection의 수직 탐색 범위에 쓰인다.
     */
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
     * 요청 당시 스포너 Transform과 설정의 상대 영역에서 후보 위치를 선택한다. 고른 위치는 캐릭터의 발 위치다.
     * SpawnIndex는 이번 요청 안에서 0부터 시작한다. 지면·NavMesh·개체 간격은 보장하지 않는다.
     * 위치를 제공할 수 없으면 false다. 기본 구현은 영역 바닥면(원기둥은 원판, 상자는 사각형)에서 균일하게 표본 추출한다.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kata|Spawning")
    bool GetSpawnTransform(const FTransform& SpawnerTransform, int32 SpawnIndex, FTransform& OutTransform) const;
    virtual bool GetSpawnTransform_Implementation(const FTransform& SpawnerTransform, int32 SpawnIndex, FTransform& OutTransform) const;

    /** 현재 모양의 높이 절반(cm, 영역 스케일 적용 전). 원기둥은 CylinderHalfHeight, 상자는 BoxExtent.Z다. 음수와 유한하지 않은 값은 0으로 본다. */
    float GetAreaHalfHeight() const;

    /** 현재 모양의 수평 반경(cm, 영역 스케일 적용 전). 원기둥은 CylinderRadius, 상자는 BoxExtent의 X·Y 중 큰 값이다. 구한 값이 음수이거나 유한하지 않으면 0으로 본다. */
    float GetAreaHorizontalRadius() const;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
