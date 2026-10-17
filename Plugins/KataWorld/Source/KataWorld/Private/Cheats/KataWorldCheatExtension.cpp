#include "Cheats/KataWorldCheatExtension.h"

#include "GameFramework/PlayerController.h"
#include "KataWorldLog.h"
#include "Time/KataWorldTimeSubsystem.h"

namespace
{
#if UE_WITH_CHEAT_MANAGER
    /**
     * 토글 인자를 읽는다. 비우면 bCurrent를 반전한 값을 bOutValue에 쓴다. 알 수 없는 값이면 false를 돌려준다.
     * KataWorld는 KataTargeting을 참조할 수 없으므로 공용 파서와 같은 규칙으로 따로 구현한다.
     */
    bool ParseToggle(const FString& Value, bool bCurrent, bool& bOutValue)
    {
        const FString Trimmed = Value.TrimStartAndEnd();
        if (Trimmed.IsEmpty())
        {
            bOutValue = !bCurrent;
            return true;
        }
        if (Trimmed == TEXT("1") || Trimmed.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Trimmed.Equals(TEXT("on"), ESearchCase::IgnoreCase))
        {
            bOutValue = true;
            return true;
        }
        if (Trimmed == TEXT("0") || Trimmed.Equals(TEXT("false"), ESearchCase::IgnoreCase) || Trimmed.Equals(TEXT("off"), ESearchCase::IgnoreCase))
        {
            bOutValue = false;
            return true;
        }
        return false;
    }
#endif

    const TCHAR* GetTimeModeName(EKataWorldTimeMode Mode)
    {
        return Mode == EKataWorldTimeMode::Flowing ? TEXT("Flowing") : TEXT("Fixed");
    }
}

FDelegateHandle UKataWorldCheatExtension::RegisterForCheatManagers()
{
#if UE_WITH_CHEAT_MANAGER
    return UCheatManager::RegisterForOnCheatManagerCreated(FOnCheatManagerCreated::FDelegate::CreateLambda([](UCheatManager* CheatManager)
    {
        CheatManager->AddCheatManagerExtension(NewObject<UKataWorldCheatExtension>(CheatManager));
    }));
#else
    return FDelegateHandle();
#endif
}

void UKataWorldCheatExtension::UnregisterFromCheatManagers(FDelegateHandle Handle)
{
#if UE_WITH_CHEAT_MANAGER
    if (Handle.IsValid())
    {
        UCheatManager::UnregisterFromOnCheatManagerCreated(Handle);
    }
#endif
}

void UKataWorldCheatExtension::KataTime(const FString& Hour)
{
#if UE_WITH_CHEAT_MANAGER
    UKataWorldTimeSubsystem* TimeSubsystem = FindTimeSubsystem(TEXT("KataTime"));
    if (TimeSubsystem == nullptr)
    {
        return;
    }

    const FString Trimmed = Hour.TrimStartAndEnd();
    if (!Trimmed.IsEmpty())
    {
        float NewHour = 0.0f;
        if (!LexTryParseString(NewHour, *Trimmed) || !FMath::IsFinite(NewHour))
        {
            Report(FString::Printf(TEXT("KataTime: unknown hour '%s'. Use a number such as 18.5."), *Hour));
            return;
        }
        TimeSubsystem->SetTimeOfDay(NewHour);
    }
    Report(FString::Printf(TEXT("KataTime %s"), *DescribeTime(*TimeSubsystem)));
#endif
}

void UKataWorldCheatExtension::KataTimeFlow(const FString& Enable)
{
#if UE_WITH_CHEAT_MANAGER
    UKataWorldTimeSubsystem* TimeSubsystem = FindTimeSubsystem(TEXT("KataTimeFlow"));
    if (TimeSubsystem == nullptr)
    {
        return;
    }

    const bool bFlowing = TimeSubsystem->GetTimeMode() == EKataWorldTimeMode::Flowing;
    bool bNewFlowing = bFlowing;
    if (!ParseToggle(Enable, bFlowing, bNewFlowing))
    {
        Report(FString::Printf(TEXT("KataTimeFlow: unknown value '%s'. Use 0 or 1."), *Enable));
        return;
    }
    TimeSubsystem->SetTimeMode(bNewFlowing ? EKataWorldTimeMode::Flowing : EKataWorldTimeMode::Fixed);
    Report(FString::Printf(TEXT("KataTimeFlow %s"), *DescribeTime(*TimeSubsystem)));
#endif
}

void UKataWorldCheatExtension::KataDayLength(const FString& Minutes)
{
#if UE_WITH_CHEAT_MANAGER
    UKataWorldTimeSubsystem* TimeSubsystem = FindTimeSubsystem(TEXT("KataDayLength"));
    if (TimeSubsystem == nullptr)
    {
        return;
    }

    float NewMinutes = 0.0f;
    const FString Trimmed = Minutes.TrimStartAndEnd();
    if (Trimmed.IsEmpty() || !LexTryParseString(NewMinutes, *Trimmed) || !FMath::IsFinite(NewMinutes))
    {
        Report(FString::Printf(TEXT("KataDayLength: unknown minutes '%s'. Use a number such as 2."), *Minutes));
        return;
    }
    TimeSubsystem->SetDayLengthMinutes(NewMinutes);
    Report(FString::Printf(TEXT("KataDayLength %s"), *DescribeTime(*TimeSubsystem)));
#endif
}

UKataWorldTimeSubsystem* UKataWorldCheatExtension::FindTimeSubsystem(const TCHAR* CommandName) const
{
    UKataWorldTimeSubsystem* TimeSubsystem = UKataWorldTimeSubsystem::Get(GetPlayerController());
    if (TimeSubsystem == nullptr)
    {
        Report(FString::Printf(TEXT("%s: no Kata World time subsystem in this world"), CommandName));
    }
    return TimeSubsystem;
}

FString UKataWorldCheatExtension::DescribeTime(const UKataWorldTimeSubsystem& TimeSubsystem)
{
    const FGameplayTag Phase = TimeSubsystem.GetTimePhase();
    return FString::Printf(TEXT("%.2f h, phase %s, %s, day length %.1f min, day %d"),
        TimeSubsystem.GetTimeOfDay(),
        Phase.IsValid() ? *Phase.ToString() : TEXT("None"),
        GetTimeModeName(TimeSubsystem.GetTimeMode()),
        TimeSubsystem.GetDayLengthMinutes(),
        TimeSubsystem.GetDayCount());
}

void UKataWorldCheatExtension::Report(const FString& Message) const
{
#if UE_WITH_CHEAT_MANAGER
    UE_LOG(LogKataWorld, Display, TEXT("%s"), *Message);
    if (APlayerController* PlayerController = GetPlayerController())
    {
        PlayerController->ClientMessage(Message);
    }
#endif
}
