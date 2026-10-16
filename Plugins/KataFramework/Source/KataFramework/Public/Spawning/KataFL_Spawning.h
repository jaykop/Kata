#pragma once

#include "CoreMinimal.h"

class UClass;
struct FKataCharacterSpawnOwnership;

namespace KataFL
{
    /** 기록의 NPC에 제거를 요청한다. 이미 파괴됐으면 true, 살아 있는 NPC가 Destroy를 거절하면 false다. */
    KATAFRAMEWORK_API bool DestroySpawnedCharacter(FKataCharacterSpawnOwnership& Ownership);

    /** 기록한 AIController 하나를 정리한다. 다른 Pawn으로 이전한 Controller는 유지하며 성공으로 처리한다. */
    KATAFRAMEWORK_API bool DestroySpawnOwnedController(const FKataCharacterSpawnOwnership& Ownership, int32 ControllerIndex);

    /**
     * Character 클래스 기본값의 캡슐 절반 높이(cm)에 캡슐의 상대 Z 스케일을 곱해 돌려준다.
     * 생성 위치의 높이 보정에 쓴다. 클래스가 없거나 ACharacter 파생이 아니거나 캡슐이 없으면 0이다.
     */
    KATAFRAMEWORK_API float GetCharacterClassHalfHeight(const UClass* CharacterClass);
}
