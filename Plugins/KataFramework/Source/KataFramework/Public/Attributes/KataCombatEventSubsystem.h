#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "KataCombatEventSubsystem.generated.h"

/** 피해가 Health에 반영됐을 때 알린다. Target은 피해를 받은 ASC의 Avatar, Instigator는 Effect Context의 원 가해자다(없으면 nullptr). */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FKataDamageAppliedDelegate, AActor* /*Target*/, AActor* /*Instigator*/, float /*Damage*/);

/**
 * 월드 안의 전투 사건을 알리는 확장점. 범용 메시지 라우터 대신 타입이 정해진 델리게이트만 둔다.
 *
 * UKataAttributeSet_Base가 피해로 Health를 실제로 줄였을 때 OnDamageApplied를 부른다.
 * 대미지 무시 상태라 Health가 줄지 않았거나 이미 0이었으면 알리지 않는다.
 * 구독자는 월드가 바뀌면 새 월드의 서브시스템을 다시 구독해야 한다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataCombatEventSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** WorldContext의 월드에 있는 서브시스템. 월드가 없으면 nullptr이다. */
    static UKataCombatEventSubsystem* Get(const UObject* WorldContext);

    /** Damage는 실제로 줄어든 Health 양이다. 0 이하면 알리지 않는다. */
    void BroadcastDamageApplied(AActor* Target, AActor* Instigator, float Damage);

    FKataDamageAppliedDelegate OnDamageApplied;
};
