#pragma once

#include "CoreMinimal.h"

struct FKataCharacterSpawnOwnership;

namespace KataFL
{
    /** 기록의 NPC에 제거를 요청한다. 이미 파괴됐으면 true, 살아 있는 NPC가 Destroy를 거절하면 false다. */
    KATAFRAMEWORK_API bool DestroySpawnedCharacter(FKataCharacterSpawnOwnership& Ownership);

    /** 기록한 AIController 하나를 정리한다. 다른 Pawn으로 이전한 Controller는 유지하며 성공으로 처리한다. */
    KATAFRAMEWORK_API bool DestroySpawnOwnedController(const FKataCharacterSpawnOwnership& Ownership, int32 ControllerIndex);
}
