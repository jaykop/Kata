#pragma once

#include "CoreMinimal.h"
#include "Character/KataCharacterSpawnSubsystem.h"
#include "Character/KataCharacterSpawnOwnership.h"
#include "UObject/Object.h"
#include "KataSpawnBatchTypes.generated.h"

class UKataSpawnerComponent;
class UKataSpawnerComponent_SpawnArea;
class UKataSpawnerSubsystem;

/** 배치 시작 시 고정한 생성 정보. 설정 사본과 행의 UObject 참조는 배치 실행 객체가 GC 추적한다. */
USTRUCT()
struct KATAFRAMEWORK_API FKataSpawnBatchContext
{
    GENERATED_BODY()

    UPROPERTY(Transient)
    FKataCharacterId CharacterId;

    UPROPERTY(Transient)
    FInstancedStruct RowData;

    UPROPERTY(Transient)
    FTransform SpawnerTransform = FTransform::Identity;

    UPROPERTY(Transient)
    TObjectPtr<UKataSpawnerComponent_SpawnArea> SpawnArea;

    ESpawnActorCollisionHandlingMethod CollisionHandling = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    int32 RequestedCount = 0;
    int32 MaxPlacementAttempts = 1;
};

/** 후보 선택·보정·확정을 각각 한 작업으로 진행하기 위한 단계. */
enum class EKataSpawnPlacementPhase : uint8
{
    SelectCandidate,
    AdjustCandidate,
    CompleteCandidate
};

/** 위치 계산과 제출의 중단 지점. 설정 객체를 변경하지 않고 다음 작업에서 이어간다. */
struct KATAFRAMEWORK_API FKataSpawnBatchProgress
{
    int32 PlacementIndex = 0;
    int32 AttemptIndex = 0;
    int32 ComponentIndex = 0;
    int32 SubmissionIndex = 0;
    EKataSpawnPlacementPhase PlacementPhase = EKataSpawnPlacementPhase::SelectCandidate;
    FTransform CandidateTransform = FTransform::Identity;
    bool bPlacementAccepted = false;
};

/**
 * 스포너 한 배치의 실행 데이터. 설정 에셋에 실행 상태를 저장하지 않는다.
 * 콜백이 스포너의 참조를 해제해도 호출 중에는 강한 참조로 행·설정 사본을 유지한다.
 */
UCLASS(Transient)
class KATAFRAMEWORK_API UKataSpawnBatchState : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(Transient)
    FKataSpawnBatchContext Context;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UKataSpawnerComponent>> Components;

    FKataSpawnBatchProgress Progress;
    TArray<FTransform> PreparedTransforms;
    TArray<bool> PlacementFailed;
    TMap<int32, FKataCharacterSpawnHandle> PendingRequests;
    TMap<int32, TSharedPtr<FKataCharacterSpawnOwnership>> PendingOwnership;
    TSharedPtr<FKataSpawnGeneration> Generation;
    TWeakObjectPtr<UKataCharacterSpawnSubsystem> Subsystem;
    TWeakObjectPtr<UKataSpawnerSubsystem> Scheduler;
    FKataCharacterSpawnGroupHandle SpawnGroup;
    uint32 RegistrationId = 0;
    FTransform ReadyTransform = FTransform::Identity;
    bool bTimeSliced = false;
    bool bCandidateReady = false;
    bool bReadyPlacementFailed = false;
    int32 RemainingCount = 0;
    int32 SucceededCount = 0;
    int32 FailedCount = 0;
};
