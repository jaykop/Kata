#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Time/KataWorldTimeTypes.h"
#include "KataWorldSettings.generated.h"

/**
 * KataWorld의 프로젝트 기본값. 프로젝트 설정의 Kata World에서 편집하고 DefaultGame.ini에 저장한다.
 *
 * 레벨에 AKataLevelTimeSettings가 없으면 시각은 DefaultHour에서 시작해 DayLengthMinutes에 맞춰 흐른다.
 * 시간대 태그의 정의는 프로젝트가 소유한다. 이 객체는 공유 설정이므로 실행 중 상태를 저장하지 않는다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Kata World"))
class KATAWORLD_API UKataWorldSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UKataWorldSettings();

    /** 레벨 설정이 없을 때 게임을 시작하는 시각(시). 24는 0시와 같다. */
    UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "0.0", ClampMax = "24.0", UIMax = "24.0", ForceUnits = "Hours"))
    float DefaultHour = 9.0f;

    /**
     * 게임 하루(24시간)가 지나는 데 걸리는 실제 시간(분). 레벨 설정이 덮어쓰지 않으면 이 값을 쓴다.
     * 시계는 시간 배율의 영향을 받지 않고 일시정지 중에는 멈춘다.
     */
    UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "0.1", ForceUnits = "Minutes"))
    float DayLengthMinutes = 24.0f;

    /** 시간대 구간. 순서는 상관없으며 시작 시각 순으로 정렬해 쓴다. 비어 있으면 시간대를 알리지 않는다. */
    UPROPERTY(Config, EditAnywhere, Category = "Time")
    TArray<FKataTimePhase> TimePhases;

    static const UKataWorldSettings* Get();
};
