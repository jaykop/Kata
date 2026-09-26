#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "GameplayTagContainer.h"
#include "HitTrace/KataHitTraceTypes.h"
#include "KataHurtBoxComponent.generated.h"

class UBodySetup;

/**
 * Hit Trace가 맞힐 수 있는 피격 영역.
 *
 * UKataTask_HitTrace의 판정은 이 컴포넌트만 대상으로 삼는다. 캐릭터 메시의 본·소켓에 붙여 머리·몸통 같은 부위를 만들고,
 * 맞았을 때 HitResult.BoneName에는 이 컴포넌트가 붙은 소켓(또는 본) 이름이 들어간다.
 *
 * 도형은 Sphere·Capsule·Box 중 하나이며 크기에는 컴포넌트 스케일이 적용된다(엔진 Shape 컴포넌트와 같은 규칙).
 * 기본 콜리전은 프로젝트 설정 Kata Hit Trace의 HurtBoxCollisionProfile이다. 판정은 그 프로필의 Object Type으로 HurtBox를 찾으므로,
 * 컴포넌트에서 Object Type을 다르게 바꾸면 판정에 걸리지 않는다. 프로필이 없으면 질의 전용·모든 채널 무시로 시작한다.
 * 판정에서 잠시 빼려면 SetCollisionEnabled로 콜리전을 끄거나, 태그를 붙이고 프리셋의 HurtBoxTagQuery로 거른다.
 */
UCLASS(ClassGroup = (Kata), meta = (BlueprintSpawnableComponent, DisplayName = "Kata Hurt Box"),
    hidecategories = (Object, LOD, Lighting, TextureStreaming, Rendering, Physics, Navigation))
class KATAFRAMEWORK_API UKataHurtBoxComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UKataHurtBoxComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /** 피격 영역의 도형. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hurt Box")
    EKataHurtBoxShape Shape = EKataHurtBoxShape::Capsule;

    /** Sphere의 반지름. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hurt Box", meta = (ClampMin = "0.1", Units = "cm", EditCondition = "Shape == EKataHurtBoxShape::Sphere", EditConditionHides))
    float SphereRadius = 16.0f;

    /** Capsule의 반지름. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hurt Box", meta = (ClampMin = "0.1", Units = "cm", EditCondition = "Shape == EKataHurtBoxShape::Capsule", EditConditionHides))
    float CapsuleRadius = 12.0f;

    /** 반구를 포함한 Capsule의 반높이. 로컬 Z축을 따른다. 반지름보다 작으면 반지름으로 올려 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hurt Box", meta = (ClampMin = "0.1", Units = "cm", EditCondition = "Shape == EKataHurtBoxShape::Capsule", EditConditionHides))
    float CapsuleHalfHeight = 30.0f;

    /** Box의 반 크기. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hurt Box", meta = (EditCondition = "Shape == EKataHurtBoxShape::Box", EditConditionHides))
    FVector BoxExtent = FVector(16.0f);

    /** 에디터 뷰포트에 그리는 선 색. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hurt Box")
    FColor ShapeColor = FColor(255, 160, 40);

    /** 이 피격 영역의 속성(예: 약점, 판정 제외). 판정 필터와 처리기가 읽는다. 태그의 정의와 의미는 프로젝트가 정한다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Hurt Box")
    const FGameplayTagContainer& GetHurtBoxTags() const { return HurtBoxTags; }

    UFUNCTION(BlueprintCallable, Category = "Kata|Hurt Box")
    void AddHurtBoxTag(FGameplayTag Tag);

    UFUNCTION(BlueprintCallable, Category = "Kata|Hurt Box")
    void RemoveHurtBoxTag(FGameplayTag Tag);

    // 스케일 규칙은 이 컴포넌트의 물리 바디가 BodySetup 도형에 스케일을 적용하는 방식과 같다.
    // 넓은 단계(물리 질의)와 좁은 단계(직접 교차 계산)가 같은 크기를 보도록 맞춘다.

    /** 스케일을 적용한 Sphere 반지름. 가장 작은 축 스케일을 쓴다. */
    float GetScaledSphereRadius() const;

    /** 스케일을 적용한 Capsule 반지름. X·Y 중 큰 스케일을 쓰고 반높이를 넘지 않는다. */
    float GetScaledCapsuleRadius() const;

    /** 스케일을 적용한 Capsule 반높이(반구 포함). Z 스케일을 쓴다. */
    float GetScaledCapsuleHalfHeight() const;

    /** 스케일을 적용한 Box 반 크기. */
    FVector GetScaledBoxExtent() const;

    //~ UPrimitiveComponent
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual UBodySetup* GetBodySetup() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
    virtual bool IsZeroExtent() const override;
    virtual FCollisionShape GetCollisionShape(float Inflation = 0.0f) const override;
    virtual bool ShouldCollideWhenPlacing() const override { return false; }

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hurt Box")
    FGameplayTagContainer HurtBoxTags;

private:
    /** 현재 도형 값으로 BodySetup을 만들거나 고친다. 물리 상태는 건드리지 않으므로 이미 만든 바디에 반영하려면 호출한 쪽이 다시 만든다. */
    void RebuildBodySetup();

    /** 도형 하나만 담은 충돌 정의. 컴포넌트마다 따로 만들며 저장하지 않는다. */
    UPROPERTY(Transient, DuplicateTransient)
    TObjectPtr<UBodySetup> HurtBoxBodySetup;
};
