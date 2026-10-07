#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "KataSpawnerSettings.generated.h"

/** 스포너의 생성·제거가 월드마다 공유하는 예산. 초기값은 성능 보장이 아니며 프로젝트 부하에 맞춰 조정한다. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Kata Spawner"))
class KATAFRAMEWORK_API UKataSpawnerSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UKataSpawnerSettings();

    /** 작업 사이에서 확인하는 시간 상한(ms). 개별 Blueprint 훅과 캐릭터 생성은 중간에 끊지 않는다. */
    UPROPERTY(Config, EditAnywhere, Category = "Scheduling", meta = (ClampMin = "0.01", Units = "ms"))
    double TimeBudgetMs = 1.0;

    /** 위치 준비·요청 제출의 방문 상한. 준비 큐 확인에도 별도로 같은 상한을 적용하며 취소 항목 확인을 포함한다. */
    UPROPERTY(Config, EditAnywhere, Category = "Scheduling", meta = (ClampMin = "1"))
    int32 MaxPreparationStepsPerFrame = 64;

    /** 로드 완료 요청의 실제 생성 시도 상한. 생성 실패도 한 번으로 센다. */
    UPROPERTY(Config, EditAnywhere, Category = "Scheduling", meta = (ClampMin = "1"))
    int32 MaxSpawnAttemptsPerFrame = 2;

    /** NPC 또는 기록한 Controller 하나를 처리하는 제거 단계 상한. 무효한 기록 확인도 한 번으로 센다. */
    UPROPERTY(Config, EditAnywhere, Category = "Scheduling", meta = (ClampMin = "1"))
    int32 MaxDespawnStepsPerFrame = 2;

    /** 모든 분산 그룹의 로드 중·생성 대기 요청 상한. 일반 단일 생성 요청은 제외한다. */
    UPROPERTY(Config, EditAnywhere, Category = "Scheduling", meta = (ClampMin = "1"))
    int32 MaxOutstandingRequests = 64;

    /** 한 스포너가 동시에 유지하는 로드 중·생성 대기 요청 상한. */
    UPROPERTY(Config, EditAnywhere, Category = "Scheduling", meta = (ClampMin = "1"))
    int32 MaxOutstandingRequestsPerSpawner = 8;

    /** 거리 관리 스포너 전체를 한 번 평가하는 최소 간격(초). 한 평가가 여러 프레임에 걸치면 끝난 뒤 다음 간격을 센다. */
    UPROPERTY(Config, EditAnywhere, Category = "Distance", meta = (ClampMin = "0.0", Units = "s"))
    float DistanceEvaluationInterval = 0.25f;

    /** 한 프레임에 거리를 평가하는 스포너 수 상한. 스포너 하나의 평가에는 원점과 그 스포너의 모든 NPC가 포함된다. */
    UPROPERTY(Config, EditAnywhere, Category = "Distance", meta = (ClampMin = "1"))
    int32 MaxDistanceChecksPerFrame = 16;

    /**
     * 거리 관리 스포너를 찾는 그리드 셀 한 변의 길이(cm). 월드 XY를 이 크기의 정사각형으로 나누며 생성·제거 거리와는 독립이다.
     * 월드 관리자 초기화 때 읽으므로 변경은 다음 월드부터 적용한다.
     */
    UPROPERTY(Config, EditAnywhere, Category = "Distance", meta = (ClampMin = "100.0", Units = "cm"))
    float GridCellSize = 5000.f;
};
