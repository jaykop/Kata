#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameplayTagContainer.h"
#include "KataTargetPointComponent.generated.h"

class UKataTargetPointComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKataTargetPointEnabledChangedSignature, UKataTargetPointComponent*, Point, bool, bEnabled);

/**
 * 액터 몸의 특정 지점을 타게팅 대상으로 표시하는 컴포넌트. 메시 소켓에 붙여 부위(머리, 팔, 꼬리)마다 둔다.
 *
 * 역할은 RoleTags로 구분한다. 락온은 Kata Expand Target Points 태스크가 필요 태그를 가진 지점만 후보로 펼친다.
 * Targeting 결과의 FHitResult::Component가 UPrimitiveComponent만 담을 수 있어 Primitive 계열이어야 한다.
 * 엔진 구체 컴포넌트를 상속해 에디터에서 와이어 구체로 위치를 보이고, 게임에서는 숨긴다.
 * 충돌을 끄므로 물리 바디를 만들지 않는다(엔진은 충돌이 꺼진 컴포넌트의 물리 상태를 생성하지 않는다). 반지름은 표시용이다.
 * 꺼진 지점은 모든 용도에서 후보가 아니다.
 */
UCLASS(ClassGroup = (Kata), meta = (BlueprintSpawnableComponent, DisplayName = "Kata Target Point"),
    HideCategories = (Collision, Physics, Rendering, Lighting, Navigation, HLOD, Mobile, RayTracing, TextureStreaming, VirtualTexture))
class KATATARGETING_API UKataTargetPointComponent : public USphereComponent
{
    GENERATED_BODY()

public:
    UKataTargetPointComponent(const FObjectInitializer& ObjectInitializer);

    /** 이 지점의 역할. 태그 정의는 프로젝트가 정하며 TargetPoint 하위 태그만 고를 수 있다. 예: TargetPoint.LockOn. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Target Point", meta = (Categories = "TargetPoint"))
    FGameplayTagContainer RoleTags;

    UFUNCTION(BlueprintPure, Category = "Kata|Target Point")
    bool IsTargetPointEnabled() const { return bTargetPointEnabled; }

    /** 지점을 켜거나 끈다(부위 파괴 등). 값이 바뀔 때만 OnEnabledChanged를 알린다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Target Point")
    void SetTargetPointEnabled(bool bEnabled);

    /** 지점이 켜지거나 꺼질 때 알린다. 락온 중인 타게팅 컴포넌트가 구독한다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|Target Point")
    FKataTargetPointEnabledChangedSignature OnEnabledChanged;

private:
    /** 시작 시 활성 여부. 실행 중 변경은 SetTargetPointEnabled()로 한다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Target Point", meta = (DisplayName = "Enabled"))
    bool bTargetPointEnabled = true;
};
