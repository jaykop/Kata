#pragma once

#include "Action/KataCommand.h"
#include "CoreMinimal.h"
#include "KataCommand_FaceMoveDirection.generated.h"

/**
 * 액션을 시작할 때 실행 주체를 이동 입력 방향으로 즉시 돌리는 Command. 회피 같은 액션의 PreCommands에 넣는다.
 *
 * 방향은 실행 주체의 UKataTargetingComponent::ResolveMoveDirection이 정한다. 입력이 없거나 컴포넌트가 없으면 돌리지 않는다.
 * 기본값은 락온 중에는 돌리지 않는다. 락온 중 방향 동작은 몸 방향을 유지한 채 Move Direction 조건으로 고른 액션이 맡는다.
 * Yaw만 바꾸며 Pitch·Roll은 유지한다. 컨트롤러 회전을 따르는 캐릭터(bUseControllerRotationYaw)는 컨트롤러가 다음 프레임에 회전을 덮어쓴다.
 */
UCLASS(meta = (DisplayName = "Face Move Direction"))
class KATATARGETING_API UKataCommand_FaceMoveDirection : public UKataCommand
{
    GENERATED_BODY()

public:
    /** 켜면 락온 중에는 돌리지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Facing")
    bool bOnlyWhenUnlocked = true;

protected:
    virtual void Execute_Implementation(UKataActionInstance* Instance) override;
};
