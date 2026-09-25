#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "KataTargetingComponent.generated.h"

class UTargetingPreset;

/**
 * PC와 몬스터가 함께 쓰는 타게팅 기반 컴포넌트.
 *
 * 팩션 값을 보관하고, 현재 대상 조회와 액션 시작 때의 대상 결정을 가상 함수로 제공한다.
 * 역할별 동작은 파생 클래스가 구현한다. PC용은 UKataPlayerTargetingComponent, 몬스터용은 KataAI가 제공한다.
 * 대상 결정 Command와 팩션 판정은 이 클래스만 알면 된다.
 *
 * 엔진 팀 인터페이스는 컴포넌트가 아니라 액터나 컨트롤러가 구현해야 한다. 이 컴포넌트를 가진 액터는
 * GetFactionTeamId()를 IGenericTeamAgentInterface::GetGenericTeamId에서 돌려준다.
 */
UCLASS(Blueprintable, ClassGroup = (Kata), meta = (BlueprintSpawnableComponent))
class KATATARGETING_API UKataTargetingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKataTargetingComponent();

    /** 이 액터의 팩션. Kata Factions 설정에 등록한 태그여야 팀 번호를 얻는다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Faction")
    FGameplayTag Faction;

    /** Faction의 팀 번호. 등록되지 않은 팩션이면 NoTeam이다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Faction")
    FGenericTeamId GetFactionTeamId() const;

    /** 지금 대상으로 삼고 있는 액터. 기반 구현은 대상이 없어 nullptr을 돌려준다. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kata|Targeting")
    AActor* GetCurrentTarget() const;

    /**
     * 액션을 시작할 때 쓸 대상을 정해 돌려준다. 대상 결정 Command가 액션의 PreCommands에서 호출한다.
     * 파생 클래스는 이 호출에서 자신의 대상 상태를 갱신할 수 있다. 기반 구현은 GetCurrentTarget()을 돌려준다.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kata|Targeting")
    AActor* ResolveActionTarget();

protected:
    /**
     * Preset을 즉시 실행해 후보를 순서대로 채운다. 첫 항목이 가장 우선하는 후보다.
     * 실행 주체는 소유 액터이며 소유 액터와 무효한 항목은 결과에서 뺀다. 요청 핸들은 반환 전에 해제한다.
     */
    void FindTargets(const UTargetingPreset* Preset, TArray<AActor*>& OutTargets) const;

    /** Preset의 가장 우선하는 후보. 후보가 없거나 Preset이 비어 있으면 nullptr이다. */
    AActor* FindBestTarget(const UTargetingPreset* Preset) const;
};
