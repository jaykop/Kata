#include "Time/KataWorldTimeSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "KataWorldLog.h"
#include "KataWorldSettings.h"
#include "Time/KataLevelTimeSettings.h"

namespace
{
    constexpr double HoursPerDay = 24.0;
    constexpr float MinDayLengthMinutes = 0.1f;

    /** float로 표현할 수 있는 24 미만의 가장 큰 값(24 - 2^-19). */
    constexpr float MaxFloatHour = 23.999998f;

    /** 시각을 0 이상 24 미만으로 바꾼다. 24와 음수의 경계에서 부동소수 오차로 24가 나오면 0으로 둔다. */
    double NormalizeHour(double Hour)
    {
        double Result = FMath::Fmod(Hour, HoursPerDay);
        if (Result < 0.0)
        {
            Result += HoursPerDay;
        }
        return Result >= HoursPerDay ? 0.0 : Result;
    }

    float SanitizeDayLength(float Minutes)
    {
        return FMath::Max(Minutes, MinDayLengthMinutes);
    }
}

UKataWorldTimeSubsystem* UKataWorldTimeSubsystem::Get(const UObject* WorldContext)
{
    const UWorld* World = GEngine != nullptr ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
    return World != nullptr ? World->GetSubsystem<UKataWorldTimeSubsystem>() : nullptr;
}

float UKataWorldTimeSubsystem::GetTimeOfDay() const
{
    // 내부 시계는 double이라 24 바로 아래 값이 float로 바뀌면서 24.0f로 반올림될 수 있다. 24 미만 계약을 지키려고 자른다.
    return FMath::Min(static_cast<float>(TimeOfDay), MaxFloatHour);
}

void UKataWorldTimeSubsystem::SetTimeOfDay(float Hour)
{
    if (!FMath::IsFinite(Hour))
    {
        UE_LOG(LogKataWorld, Warning, TEXT("SetTimeOfDay ignored: invalid hour %f"), Hour);
        return;
    }

    TimeOfDay = NormalizeHour(Hour);
    UpdatePhase();
}

void UKataWorldTimeSubsystem::SetTimeMode(EKataWorldTimeMode NewMode)
{
    // Tick 여부는 IsTickable이 매 프레임 모드를 읽어 정하므로 값만 바꾼다.
    TimeMode = NewMode;
}

void UKataWorldTimeSubsystem::SetDayLengthMinutes(float Minutes)
{
    if (!FMath::IsFinite(Minutes))
    {
        UE_LOG(LogKataWorld, Warning, TEXT("SetDayLengthMinutes ignored: invalid length %f"), Minutes);
        return;
    }

    DayLengthMinutes = SanitizeDayLength(Minutes);
}

bool UKataWorldTimeSubsystem::RegisterLevelSettings(AKataLevelTimeSettings* Settings)
{
    if (!IsValid(Settings))
    {
        return false;
    }
    if (LevelSettings.IsValid() && LevelSettings.Get() != Settings)
    {
        UE_LOG(LogKataWorld, Warning, TEXT("Level time settings %s ignored: %s is already active in this world"),
            *GetNameSafe(Settings), *GetNameSafe(LevelSettings.Get()));
        return false;
    }

    LevelSettings = Settings;
    TimeMode = Settings->TimeMode;
    DayLengthMinutes = SanitizeDayLength(Settings->bOverrideDayLength ? Settings->DayLengthMinutes : UKataWorldSettings::Get()->DayLengthMinutes);
    TimeOfDay = NormalizeHour(Settings->InitialHour);
    UpdatePhase();
    return true;
}

void UKataWorldTimeSubsystem::UnregisterLevelSettings(AKataLevelTimeSettings* Settings)
{
    if (Settings == nullptr || LevelSettings.Get() != Settings)
    {
        return;
    }

    LevelSettings.Reset();
    TimeMode = EKataWorldTimeMode::Flowing;
    DayLengthMinutes = SanitizeDayLength(UKataWorldSettings::Get()->DayLengthMinutes);
}

void UKataWorldTimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const UKataWorldSettings* Settings = UKataWorldSettings::Get();
    TimeMode = EKataWorldTimeMode::Flowing;
    DayLengthMinutes = SanitizeDayLength(Settings->DayLengthMinutes);
    TimeOfDay = NormalizeHour(Settings->DefaultHour);
    DayCount = 0;
    CachePhases();

    // 이 시점에는 구독자가 없으므로 알리지 않고 정한다.
    CurrentPhase = FindPhase(TimeOfDay);
}

void UKataWorldTimeSubsystem::Deinitialize()
{
    OnTimePhaseChanged.Clear();
    OnTimePhaseChangedNative.Clear();
    LevelSettings.Reset();

    Super::Deinitialize();
}

void UKataWorldTimeSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Tick이 받는 시간에는 월드 배율이 곱해져 있다. 하루의 흐름은 배율과 무관해야 하므로 되돌린다.
    const AWorldSettings* WorldSettings = GetWorld() != nullptr ? GetWorld()->GetWorldSettings(false, false) : nullptr;
    const float WorldTimeDilation = WorldSettings != nullptr ? WorldSettings->GetEffectiveTimeDilation() : 1.0f;
    if (WorldTimeDilation <= UE_SMALL_NUMBER)
    {
        return;
    }

    const double HoursPerSecond = HoursPerDay / (static_cast<double>(DayLengthMinutes) * 60.0);
    const double AdvancedHour = TimeOfDay + static_cast<double>(DeltaTime / WorldTimeDilation) * HoursPerSecond;

    // 긴 히치로 한 Tick에 하루 이상 지나도 지난 날짜를 모두 센다.
    DayCount += FMath::FloorToInt32(AdvancedHour / HoursPerDay);
    TimeOfDay = NormalizeHour(AdvancedHour);
    UpdatePhase();
}

bool UKataWorldTimeSubsystem::IsTickable() const
{
    return TimeMode == EKataWorldTimeMode::Flowing;
}

TStatId UKataWorldTimeSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UKataWorldTimeSubsystem, STATGROUP_Tickables);
}

bool UKataWorldTimeSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UKataWorldTimeSubsystem::CachePhases()
{
    Phases.Reset();
    for (const FKataTimePhase& Phase : UKataWorldSettings::Get()->TimePhases)
    {
        if (!Phase.PhaseTag.IsValid())
        {
            UE_LOG(LogKataWorld, Warning, TEXT("Kata World time phase at %.2f h has no tag and is ignored"), Phase.StartHour);
            continue;
        }

        FKataTimePhase& Cached = Phases.Add_GetRef(Phase);
        Cached.StartHour = static_cast<float>(NormalizeHour(Phase.StartHour));
    }

    Phases.StableSort([](const FKataTimePhase& A, const FKataTimePhase& B)
    {
        return A.StartHour < B.StartHour;
    });
}

FGameplayTag UKataWorldTimeSubsystem::FindPhase(double Hour) const
{
    if (Phases.IsEmpty())
    {
        return FGameplayTag();
    }

    // 시각보다 늦게 시작하는 구간만 있으면 전날 마지막 구간이 이어지고 있는 것이다.
    const FKataTimePhase* Found = &Phases.Last();
    for (const FKataTimePhase& Phase : Phases)
    {
        if (Phase.StartHour > Hour)
        {
            break;
        }
        Found = &Phase;
    }
    return Found->PhaseTag;
}

void UKataWorldTimeSubsystem::UpdatePhase()
{
    const FGameplayTag NewPhase = FindPhase(TimeOfDay);
    if (NewPhase == CurrentPhase)
    {
        return;
    }

    const FGameplayTag OldPhase = CurrentPhase;
    CurrentPhase = NewPhase;
    OnTimePhaseChangedNative.Broadcast(OldPhase, NewPhase);
    OnTimePhaseChanged.Broadcast(OldPhase, NewPhase);
}
