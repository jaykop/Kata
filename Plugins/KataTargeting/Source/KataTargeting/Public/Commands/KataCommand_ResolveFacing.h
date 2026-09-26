#pragma once

#include "Action/KataCommand.h"
#include "CoreMinimal.h"
#include "KataCommand_ResolveFacing.generated.h"

/**
 * 액션을 시작할 때 실행 주체를 공격 방향으로 즉시 돌리는 Command. 액션의 PreCommands에서 Resolve Target 뒤에 넣는다.
 *
 * 방향은 실행 주체의 UKataTargetingComponent::ResolveFacingDirection이 이번 실행의 대상을 받아 정한다.
 * PC는 락온 대상 → 이동 입력 방향 → 대상 순서이고, 기반 컴포넌트는 대상 쪽이다.
 * 방향이 없거나 컴포넌트가 없으면 돌리지 않는다. Yaw만 바꾸며 Pitch·Roll은 유지한다.
 *
 * 한 번에 돌리는 동작만 담당한다. 구간 동안 일정한 속도로 돌리려면 Kata Task: Rotate To Facing을 쓴다.
 * 컨트롤러 회전을 따르는 캐릭터(bUseControllerRotationYaw)는 컨트롤러가 다음 프레임에 회전을 덮어쓴다.
 */
UCLASS(meta = (DisplayName = "Resolve Facing"))
class KATATARGETING_API UKataCommand_ResolveFacing : public UKataCommand
{
    GENERATED_BODY()

protected:
    virtual void Execute_Implementation(UKataActionInstance* Instance) override;
};
