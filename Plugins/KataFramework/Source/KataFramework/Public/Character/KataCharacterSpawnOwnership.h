#pragma once

#include "CoreMinimal.h"

class AKataCharacter;
class AController;

/** 배치의 생성 세대. 생성 중 디스폰 요청도 결과 콜백 전에 확인할 수 있도록 공유한다. */
struct KATAFRAMEWORK_API FKataSpawnGeneration
{
    uint32 Id = 0;
    bool bDespawnRequested = false;
};

/**
 * 생성한 NPC와 NPC가 직접 만든 Controller의 수명 기록. 외부에서 빙의한 Controller는 기록하지 않는다.
 * 약한 참조만 보관하며 스포너·생성 요청·캐릭터·제거 작업이 같은 기록을 공유한다.
 */
struct KATAFRAMEWORK_API FKataCharacterSpawnOwnership
{
    TSharedPtr<FKataSpawnGeneration> Generation;
    TWeakObjectPtr<AKataCharacter> Character;
    TArray<TWeakObjectPtr<AController>> OwnedControllers;
    bool bDespawnRequested = false;

    bool IsDespawnRequested() const
    {
        return bDespawnRequested || (Generation.IsValid() && Generation->bDespawnRequested);
    }
};
