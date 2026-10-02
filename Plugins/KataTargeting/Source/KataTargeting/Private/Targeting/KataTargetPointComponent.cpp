#include "Targeting/KataTargetPointComponent.h"

#include "Engine/CollisionProfile.h"

UKataTargetPointComponent::UKataTargetPointComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;

    // 위치 표시용이다. 충돌·오버랩·그림자·내비게이션에 관여하지 않게 해 물리 상태가 생기지 않도록 한다.
    SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    SetGenerateOverlapEvents(false);
    SetCanEverAffectNavigation(false);
    CanCharacterStepUpOn = ECB_No;
    CastShadow = false;
    bHiddenInGame = true;

    // 에디터에서 부위 위치를 알아볼 수 있는 작은 와이어 구체로 보인다. 반지름과 색은 Shape 항목에서 바꿀 수 있다.
    InitSphereRadius(12.0f);
    ShapeColor = FColor(255, 140, 0);
}

void UKataTargetPointComponent::SetTargetPointEnabled(bool bEnabled)
{
    if (bTargetPointEnabled == bEnabled)
    {
        return;
    }
    bTargetPointEnabled = bEnabled;
    OnEnabledChanged.Broadcast(this, bEnabled);
}
