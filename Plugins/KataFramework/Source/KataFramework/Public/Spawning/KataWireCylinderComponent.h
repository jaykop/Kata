#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "KataWireCylinderComponent.generated.h"

/**
 * 바닥면 중심이 컴포넌트 원점에 놓인 원기둥을 선으로 그리는 표시 전용 컴포넌트.
 *
 * 스포너의 원기둥 영역 미리보기에 사용한다. 충돌과 내비게이션에 영향을 주지 않는다.
 * 반지름에는 컴포넌트 스케일의 X·Y 중 큰 절댓값, 높이에는 Z 절댓값을 곱한다. 비균등 스케일의 타원은 그리지 않는다.
 */
UCLASS(ClassGroup = (Kata), meta = (DisplayName = "Kata Wire Cylinder"),
    hidecategories = (Object, LOD, Lighting, TextureStreaming, Physics, Collision, Navigation))
class KATAFRAMEWORK_API UKataWireCylinderComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UKataWireCylinderComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /** 바닥 원의 반지름, cm. 음수는 0으로 그린다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float Radius = 100.f;

    /** 높이의 절반, cm. 바닥면에서 위로 이 값의 두 배까지 그린다. 0이면 바닥 원만 그린다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float HalfHeight = 0.f;

    /** 선 색. 선택·호버 상태에서는 엔진 규칙에 따라 표시 색과 밝기가 달라진다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
    FColor ShapeColor = FColor(255, 170, 0);

    /** 선 두께. 0이면 1픽셀 선이다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float LineThickness = 0.f;

    /** 크기를 바꾸고, 값이 달라졌으면 렌더 상태와 경계를 갱신한다. 음수와 유한하지 않은 값은 0으로 바꾼다. */
    void SetCylinderSize(float InRadius, float InHalfHeight);

    //~ Begin UPrimitiveComponent Interface
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    //~ End UPrimitiveComponent Interface

    //~ Begin USceneComponent Interface
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
    //~ End USceneComponent Interface
};
