#pragma once

#include "CoreMinimal.h"
#include "Spawning/KataDummyComponent.h"
#include "UObject/Object.h"
#include "UObject/SoftObjectPath.h"
#include "KataRuntimeSpawnerUserSettings.generated.h"

/** 여러 개체를 놓는 방식. */
UENUM()
enum class EKataRuntimeSpawnPattern : uint8
{
    /** 정면 기준점을 중심으로 플레이어 정면에 수직인 선 위에 놓는다. */
    Line,
    /** 거리를 반지름으로 플레이어를 둘러싸며, 개체 수만큼 원 전체에 균등하게 놓는다. 간격은 쓰지 않는다. */
    Circle
};

/** 생성한 NPC의 AI 동작. */
UENUM()
enum class EKataRuntimeSpawnAIMode : uint8
{
    /** 행의 AI Data를 그대로 쓴다. */
    Default,
    /** 행 사본의 AI Data를 비워 아무것도 하지 않는다. */
    Idle,
    /** AI를 끄고 선택한 Action을 반복한다. */
    RepeatAction,
    /** AI를 끄고 선택한 Graph를 반복한다. */
    RepeatGraph
};

/**
 * 런타임 스포너 패널의 마지막 입력값이다. 사용자별 설정(EditorPerProjectUserSettings)에 저장해 에디터를 다시 열어도 유지한다.
 * 에셋은 경로로만 기억하며 도구가 필요할 때 로드한다.
 */
UCLASS(config = EditorPerProjectUserSettings)
class UKataRuntimeSpawnerUserSettings : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(config)
    FSoftObjectPath Table;

    UPROPERTY(config)
    FName RowName;

    /** 플레이어에서 정면 기준점까지의 거리, cm. 원형 패턴에서는 반지름이다. */
    UPROPERTY(config)
    float Distance = 300.f;

    /** 플레이어를 마주보는 방향에 더할 Yaw, 도. */
    UPROPERTY(config)
    float YawOffset = 0.f;

    UPROPERTY(config)
    int32 Count = 1;

    UPROPERTY(config)
    EKataRuntimeSpawnPattern Pattern = EKataRuntimeSpawnPattern::Line;

    /** 일렬 패턴의 개체 사이 간격, cm. 원형 패턴은 쓰지 않는다. */
    UPROPERTY(config)
    float Spacing = 150.f;

    UPROPERTY(config)
    EKataRuntimeSpawnAIMode AIMode = EKataRuntimeSpawnAIMode::Default;

    /** Action·Graph 목록을 행의 Character Class 폴더와 그 하위 폴더의 에셋으로 좁힌다. */
    UPROPERTY(config)
    bool bFilterByCharacterFolder = true;

    UPROPERTY(config)
    FSoftObjectPath ActionAsset;

    UPROPERTY(config)
    FSoftObjectPath GraphAsset;

    /** 자동 진입이 없는 Graph에 보낼 Trigger 태그 이름. 비우면 보내지 않는다. */
    UPROPERTY(config)
    FName EntryTrigger;

    /** 한 번의 실행이 끝난 뒤(복귀를 켜면 복귀한 뒤) 다시 시작할 때까지 기다리는 시간, 초. */
    UPROPERTY(config)
    float RepeatInterval = 1.f;

    /** 반복 모드에서 한 번의 실행이 끝나면 생성 위치·회전으로 순간이동한다. */
    UPROPERTY(config)
    bool bReturnToStart = true;

    /**
     * 생성 직후 NPC에 걸 더미 보호. 게임플레이 태그로 걸리므로 치트 패널의 무적·대미지 무시 토글로는 풀리지 않는다.
     * 예전 치트 보호 enum과 값 이름이 같아 이전에 저장한 값을 그대로 읽는다.
     */
    UPROPERTY(config)
    EKataDummyProtection Protection = EKataDummyProtection::None;
};
