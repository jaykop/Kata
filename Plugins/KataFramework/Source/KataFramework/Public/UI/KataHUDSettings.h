#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "KataHUDSettings.generated.h"

/** 적 체력바를 띄울 피해의 출처. */
UENUM(BlueprintType)
enum class EKataEnemyBarDamageSource : uint8
{
    /** 로컬 플레이어가 조종하는 폰이 가해자인 피해만. */
    PlayerOnly,
    /** 가해자와 관계없이 모든 피해. 적끼리의 피해나 환경 피해도 포함한다. */
    Any
};

/** MainHUD 적 체력바의 표시 규칙. 프로젝트 설정 Plugins > Kata HUD에서 바꾼다. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Kata HUD"))
class KATAFRAMEWORK_API UKataHUDSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UKataHUDSettings();

    static const UKataHUDSettings* Get();

    /** 어떤 피해에 적 체력바를 띄울지. 락온 대상은 이 값과 관계없이 표시한다. */
    UPROPERTY(Config, EditAnywhere, Category = "Enemy Health Bar")
    EKataEnemyBarDamageSource DamageSource = EKataEnemyBarDamageSource::PlayerOnly;

    /** 마지막 피해 또는 락온 해제 뒤 바를 유지하는 시간(초). 락온 중에는 사라지지 않는다. */
    UPROPERTY(Config, EditAnywhere, Category = "Enemy Health Bar", meta = (ClampMin = "0.0", Units = "s"))
    float DisplayDuration = 5.0f;

    /** 동시에 표시할 최대 개수. 넘으면 락온하지 않은 항목 중 가장 먼저 사라질 항목을 지운다. */
    UPROPERTY(Config, EditAnywhere, Category = "Enemy Health Bar", meta = (ClampMin = "1"))
    int32 MaxEnemyBars = 8;

    /** 관찰 폰에서 이 거리보다 먼 대상은 새로 띄우지 않고, 표시 중이면 지운다. 락온 대상은 제외한다. */
    UPROPERTY(Config, EditAnywhere, Category = "Enemy Health Bar", meta = (ClampMin = "0.0", Units = "cm"))
    float MaxDistance = 3000.0f;

    /** 대상이 죽은 뒤 바를 지울 때까지의 시간(초). 락온 중이어도 이 시간 뒤 지운다. */
    UPROPERTY(Config, EditAnywhere, Category = "Enemy Health Bar", meta = (ClampMin = "0.0", Units = "s"))
    float DeadHideDelay = 1.0f;

    /** 바 위치. 대상 캡슐 꼭대기에서 위로 띄우는 높이. 캡슐이 없으면 액터 바운드 꼭대기를 쓴다. */
    UPROPERTY(Config, EditAnywhere, Category = "Enemy Health Bar", meta = (Units = "cm"))
    float BarHeightOffset = 30.0f;
};
