#pragma once

#include "CoreMinimal.h"
#include "Character/KataCharacterSpawnOwnership.h"

class AKataCharacterSpawner;

/** 제거 커서와 결과 집계. 스포너가 종료돼도 월드 관리자가 남은 기록을 이어서 처리한다. */
struct KATAFRAMEWORK_API FKataDespawnBatchState
{
    TWeakObjectPtr<AKataCharacterSpawner> Spawner;
    TArray<TSharedPtr<FKataCharacterSpawnOwnership>> Records;
    TArray<TSharedPtr<FKataCharacterSpawnOwnership>> FailedRecords;
    int32 RecordIndex = 0;
    int32 ControllerIndex = 0;
    int32 RemovedCharacterCount = 0;
    int32 FailedActorCount = 0;
    /** NPC가 Destroy를 거절한 기록 수. 거리 제거에서는 살아 남은 NPC를 재생성 수에서 빼는 데 쓴다. */
    int32 RefusedCharacterCount = 0;
    /** 거리 관리가 제출한 개체별 제거면 true다. 수동 제거 상태와 완료 이벤트에 영향을 주지 않는다. */
    bool bDistanceDespawn = false;
    /** 사망한 NPC의 시체 제거면 true다. 거리 제거처럼 수동 제거 상태와 완료 이벤트에 영향을 주지 않고 재생성 수에도 더하지 않는다. */
    bool bDeathRemoval = false;
    bool bCharacterProcessed = false;
    bool bRecordFailed = false;
};
