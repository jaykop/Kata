#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Time/KataWorldTimeTypes.h"
#include "KataLevelTimeSettings.generated.h"

/**
 * 레벨 단위 시간 설정. 레벨에 배치하면 BeginPlay에서 UKataWorldTimeSubsystem에 등록해 모드와 시작 시각을 적용한다.
 * 하루 길이는 bOverrideDayLength를 켰을 때만 프로젝트 기본값 대신 이 액터의 값을 쓴다.
 *
 * 한 월드에서는 먼저 등록된 설정 하나만 쓰고, 다른 설정 액터의 등록은 경고를 남기고 무시한다. 스트리밍 레벨에 둔 설정도 같은 규칙을 따른다.
 * 등록된 액터의 EndPlay에서 등록을 해제한다. 그때 모드는 Flowing, 하루 길이는 프로젝트 기본값으로 돌아가고 현재 시각은 유지된다.
 * 값은 등록할 때 한 번 읽으므로 실행 중에 바꾸려면 UKataWorldTimeSubsystem의 함수를 쓴다.
 */
UCLASS(meta = (DisplayName = "Kata Level Time Settings"))
class KATAWORLD_API AKataLevelTimeSettings : public AInfo
{
    GENERATED_BODY()

public:
    /** 이 레벨에서 시각이 흐르는 방식. Fixed면 InitialHour에 멈춰 있다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Time")
    EKataWorldTimeMode TimeMode = EKataWorldTimeMode::Flowing;

    /** 레벨을 시작하는 시각(시). Fixed 모드에서는 고정 시각이다. 24는 0시와 같다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Time", meta = (ClampMin = "0.0", ClampMax = "24.0", UIMax = "24.0", ForceUnits = "Hours"))
    float InitialHour = 9.0f;

    /** 켜면 이 레벨에서는 프로젝트 설정 대신 DayLengthMinutes를 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Time", meta = (InlineEditConditionToggle))
    bool bOverrideDayLength = false;

    /** 이 레벨에서 게임 하루가 지나는 데 걸리는 실제 시간(분). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Time", meta = (EditCondition = "bOverrideDayLength", ClampMin = "0.1", ForceUnits = "Minutes"))
    float DayLengthMinutes = 24.0f;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
