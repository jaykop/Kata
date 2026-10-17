#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "Time/KataWorldTimeTypes.h"
#include "KataWorldTimeSubsystem.generated.h"

class AKataLevelTimeSettings;

/**
 * 월드의 현재 시각과 시간대를 관리하는 월드 Subsystem. 낮밤·날씨 연출과 이후 게임플레이 연결이 읽는 상태 계층이다.
 *
 * 시각은 0 이상 24 미만의 시(hour)로 표현한다. Flowing 모드에서는 하루 길이에 맞춰 흐르고, 24시를 넘으면 0시로 돌아가며 날짜 수가 늘어난다.
 * 시계는 시간 배율(슬로모션·히트스톱)의 영향을 받지 않고 일시정지 중에는 멈춘다. 월드 Tick이 받은 시간을 월드 유효 배율로 나눠 쓴다.
 * 시작 값은 프로젝트 설정 Kata World에서 가져오고, 레벨의 AKataLevelTimeSettings가 BeginPlay에서 등록되면 그 값으로 바꾼다.
 * 시간대는 설정의 구간 목록에서 현재 시각이 속한 구간의 태그다. 바뀌면 OnTimePhaseChangedNative, OnTimePhaseChanged 순서로 알린다.
 * 구독하기 전에 정해진 시간대는 알리지 않으므로 구독자는 GetTimePhase로 현재 값을 먼저 읽는다.
 * 연출처럼 매 프레임 시각이 필요한 쪽은 GetTimeOfDay를 읽는다.
 * 게임·PIE 월드에서만 만들어진다. 싱글플레이 전용이며 복제하지 않는다.
 */
UCLASS()
class KATAWORLD_API UKataWorldTimeSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    /** WorldContext의 월드에 있는 Subsystem. 월드가 없거나 이 Subsystem을 만들지 않는 월드 종류면 nullptr이다. */
    static UKataWorldTimeSubsystem* Get(const UObject* WorldContext);

    /** 현재 시각(시). 0 이상 24 미만이다. */
    UFUNCTION(BlueprintPure, Category = "Kata|World|Time")
    float GetTimeOfDay() const;

    /**
     * 시각을 바로 바꾼다. 24로 나눈 나머지가 0 이상 24 미만이 되도록 정규화한다(26은 2, -1은 23, 24는 0).
     * 두 모드 모두에서 쓸 수 있고 모드는 바꾸지 않는다.
     * 앞이나 뒤로 건너뛰어도 날짜 수는 바꾸지 않는다. 시간대가 달라지면 변경을 알린다. 유한하지 않은 값이면 경고를 남기고 무시한다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|World|Time")
    void SetTimeOfDay(float Hour);

    UFUNCTION(BlueprintPure, Category = "Kata|World|Time")
    EKataWorldTimeMode GetTimeMode() const { return TimeMode; }

    /** 시각이 흐르는 방식을 바꾼다. 현재 시각은 유지한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|World|Time")
    void SetTimeMode(EKataWorldTimeMode NewMode);

    /** 게임 하루가 지나는 데 걸리는 실제 시간(분). */
    UFUNCTION(BlueprintPure, Category = "Kata|World|Time")
    float GetDayLengthMinutes() const { return DayLengthMinutes; }

    /** 하루 길이를 바꾼다. 0.1분보다 짧은 값은 0.1분으로 올린다. 유한하지 않은 값이면 경고를 남기고 무시한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|World|Time")
    void SetDayLengthMinutes(float Minutes);

    /** 현재 시간대 태그. 구간 목록이 비어 있으면 빈 태그다. */
    UFUNCTION(BlueprintPure, Category = "Kata|World|Time")
    FGameplayTag GetTimePhase() const { return CurrentPhase; }

    /** 이 월드가 시작된 뒤 시각이 흘러 0시를 지난 횟수. 시작한 날이 0이다. */
    UFUNCTION(BlueprintPure, Category = "Kata|World|Time")
    int32 GetDayCount() const { return DayCount; }

    /**
     * 레벨 설정을 등록하고 그 값으로 모드, 시각, 하루 길이를 정한다. AKataLevelTimeSettings의 BeginPlay가 부른다.
     * @return 등록했으면 true. 설정이 유효하지 않으면 경고 없이 false를 돌려준다.
     * 다른 설정이 이미 등록돼 있으면 경고를 남기고 false를 돌려준다.
     */
    bool RegisterLevelSettings(AKataLevelTimeSettings* Settings);

    /**
     * 등록한 레벨 설정을 해제한다. 모드는 Flowing, 하루 길이는 프로젝트 기본값으로 돌아가고 현재 시각은 유지한다.
     * 등록한 설정이 아니면 아무것도 하지 않는다.
     */
    void UnregisterLevelSettings(AKataLevelTimeSettings* Settings);

    /** 시간대가 바뀌면 알린다. */
    UPROPERTY(BlueprintAssignable, Category = "Kata|World|Time")
    FKataTimePhaseChangedSignature OnTimePhaseChanged;

    /** OnTimePhaseChanged보다 먼저 알리는 C++ 델리게이트. */
    FKataTimePhaseChangedNativeSignature OnTimePhaseChangedNative;

    //~ Begin USubsystem Interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    //~ Begin FTickableGameObject Interface
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;
    //~ End FTickableGameObject Interface

protected:
    /** 게임·PIE 월드만 지원한다. 편집 중인 레벨과 에디터 프리뷰 월드에서는 시간이 흐르지 않는다. */
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
    /** 프로젝트 설정의 시간대 구간을 시작 시각 순으로 정렬해 Phases에 담는다. 태그가 없는 구간은 경고를 남기고 뺀다. */
    void CachePhases();

    /** 시각이 속한 구간의 태그. 첫 구간보다 이른 시각은 전날부터 이어진 마지막 구간에 속한다. */
    FGameplayTag FindPhase(double Hour) const;

    /** 현재 시각으로 시간대를 다시 정하고, 달라졌으면 알린다. */
    void UpdatePhase();

    /** 시계 정밀도를 위해 double로 누적한다. */
    double TimeOfDay = 0.0;

    float DayLengthMinutes = 24.0f;
    int32 DayCount = 0;
    EKataWorldTimeMode TimeMode = EKataWorldTimeMode::Flowing;
    FGameplayTag CurrentPhase;

    /** StartHour 순으로 정렬한 구간 사본. 공유 설정 객체에 실행 상태를 두지 않기 위해 Subsystem이 가진다. */
    TArray<FKataTimePhase> Phases;

    /** 등록된 레벨 설정. 액터 수명은 레벨이 가지므로 약한 참조로 둔다. */
    TWeakObjectPtr<AKataLevelTimeSettings> LevelSettings;
};
