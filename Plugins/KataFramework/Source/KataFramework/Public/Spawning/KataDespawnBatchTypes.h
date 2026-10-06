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
    bool bCharacterProcessed = false;
    bool bRecordFailed = false;
};
