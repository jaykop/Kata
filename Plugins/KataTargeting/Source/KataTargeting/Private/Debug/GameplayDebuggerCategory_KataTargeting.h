#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

class AActor;
class UKataPlayerTargetingComponent;

/**
 * GameplayDebugger의 "KataTargeting" 카테고리.
 *
 * 디버그 대상 액터와 무관하게 로컬 플레이어 폰의 UKataPlayerTargetingComponent를 읽는다.
 * 락온 지점과 Lock On Preset이 지금 고를 수 있는 후보 지점을 우선순위와 함께 글로 보이고, 지점 위치에 구를 그린다.
 * Soft Target Preset의 AOE 수집 범위와 지금 고를 수 있는 소프트 타겟 후보도 노랑으로 함께 그린다.
 * 후보를 구하려고 수집 주기마다 Preset을 실행하므로 이 카테고리를 켠 동안에만 비용이 든다.
 */
class FGameplayDebuggerCategory_KataTargeting final : public FGameplayDebuggerCategory
{
public:
    FGameplayDebuggerCategory_KataTargeting();

    virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;

    static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

private:
    /**
     * Soft Target Preset의 AOE 선택 태스크마다 수집 범위를 도형으로 추가한다. AOE가 아닌 선택 태스크는 그리지 않는다.
     * Kata Filter Forward Angle이 있으면 AOE 수평 도달 거리만큼의 부채꼴도 추가한다.
     */
    void CollectSoftTargetRange(const UKataPlayerTargetingComponent& Targeting);

    /** Owner 정면 기준 좌우 MaxAngle의 수평 부채꼴을 Length 길이로 추가한다. 180도 이상이면 거르지 않으므로 글로만 알린다. */
    void CollectForwardAngleFan(const AActor& Owner, float MaxAngle, double Length, const FString& Label);

    /** Soft Target Preset을 실행해 후보 액터를 우선순위와 함께 글과 점으로 추가한다. 필터를 통과한 후보만 나온다. */
    void CollectSoftTargetCandidates(const UKataPlayerTargetingComponent& Targeting);
};

#endif // WITH_GAMEPLAY_DEBUGGER
