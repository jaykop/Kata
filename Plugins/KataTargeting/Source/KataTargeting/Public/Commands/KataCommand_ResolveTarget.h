#pragma once

#include "Action/KataCommand.h"
#include "CoreMinimal.h"
#include "KataCommand_ResolveTarget.generated.h"

/**
 * 액션을 시작할 때 실행 주체의 타게팅 컴포넌트로 이번 실행의 대상을 정하는 Command. 액션의 PreCommands에 넣는다.
 *
 * 실행 주체는 Context의 Avatar Actor이며, 그 액터의 UKataTargetingComponent에 대상을 묻는다.
 * 역할별 대상 규칙은 컴포넌트가 정하므로 PC와 몬스터가 같은 Command를 쓴다. 컴포넌트가 없으면 아무것도 하지 않는다.
 * 컴포넌트가 대상을 찾지 못하면 이번 실행의 대상을 비운다.
 *
 * 시작 조건은 PreCommands보다 먼저 평가되므로, 대상을 읽는 시작 조건은 이 Command가 정한 대상이 아니라 이어받은 대상으로 판정한다.
 */
UCLASS(meta = (DisplayName = "Resolve Target"))
class KATATARGETING_API UKataCommand_ResolveTarget : public UKataCommand
{
    GENERATED_BODY()

public:
    /**
     * 이어받은 대상이 유효하면 다시 구하지 않는다. 콤보 전이의 Keep Target과 함께 쓰는 옵션이다.
     * 켜져 있어도 컴포넌트의 CanKeepActionTarget이 false면 다시 구한다. PC는 락온 대상이나 이동 입력이 우선할 때 다시 구한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Targeting")
    bool bKeepValidTarget = true;

protected:
    virtual void Execute_Implementation(UKataActionInstance* Instance) override;
};
