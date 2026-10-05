#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameplayTagContainer.h"
#include "KataTargetPointComponent.generated.h"

class UKataTargetPointComponent;
class UDataAsset;

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
UCLASS(ClassGroup = (Kata), PrioritizeCategories = "Kata|TargetPoint", meta = (BlueprintSpawnableComponent, DisplayName = "Kata Target Point"),
    HideCategories = (Collision, Physics, Rendering, Lighting, Navigation, HLOD, Mobile, RayTracing, TextureStreaming, VirtualTexture))
class KATATARGETING_API UKataTargetPointComponent : public USphereComponent
{
    GENERATED_BODY()

public:
    /** 끄면 카메라 매니저의 기본 락온 설정을 쓰고, 켜면 LockOnCameraData를 그대로 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Target Point", meta = (InlineEditConditionToggle))
    bool bUseLockOnCameraData = false;

    /** 부위별 락온 카메라 데이터. 타입 선택만 제한하고 KataCamera 모듈을 직접 참조하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Target Point",
        meta = (EditCondition = "bUseLockOnCameraData", AllowedClasses = "/Script/KataCamera.KataLockOnData"))
    TObjectPtr<UDataAsset> LockOnCameraData;

    /** 이 지점에 쓸 락온 카메라 데이터. 토글이 꺼졌거나 비어 있으면 nullptr이며, 카메라는 매니저 기본값을 쓴다. */
    UDataAsset* GetLockOnCameraData() const { return bUseLockOnCameraData ? LockOnCameraData.Get() : nullptr; }

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
