#include "Cheats/KataFL_CheatTarget.h"

#if UE_WITH_CHEAT_MANAGER

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Targeting/KataTargetingComponent.h"

namespace KataFL
{
    bool ParseCheatTarget(const FString& Text, EKataCheatTarget DefaultTarget, EKataCheatTarget& OutTarget)
    {
        const FString Trimmed = Text.TrimStartAndEnd();
        if (Trimmed.IsEmpty())
        {
            OutTarget = DefaultTarget;
            return true;
        }

        for (const EKataCheatTarget Candidate : { EKataCheatTarget::Self, EKataCheatTarget::Target, EKataCheatTarget::Nearest, EKataCheatTarget::All })
        {
            if (Trimmed.Equals(GetCheatTargetName(Candidate), ESearchCase::IgnoreCase))
            {
                OutTarget = Candidate;
                return true;
            }
        }
        return false;
    }

    TArray<AActor*> ResolveCheatTargets(const APlayerController* PlayerController, EKataCheatTarget Target, const FGameplayTagContainer& ExcludedTags)
    {
        TArray<AActor*> Result;
        if (PlayerController == nullptr)
        {
            return Result;
        }

        APawn* PlayerPawn = PlayerController->GetPawn();
        if (Target == EKataCheatTarget::Self)
        {
            if (PlayerPawn != nullptr)
            {
                Result.Add(PlayerPawn);
            }
            return Result;
        }

        if (Target == EKataCheatTarget::Target)
        {
            const UKataTargetingComponent* Targeting = PlayerPawn != nullptr ? PlayerPawn->FindComponentByClass<UKataTargetingComponent>() : nullptr;
            if (AActor* CurrentTarget = Targeting != nullptr ? Targeting->GetCurrentTarget() : nullptr)
            {
                Result.Add(CurrentTarget);
            }
            return Result;
        }

        UWorld* World = PlayerController->GetWorld();
        if (World == nullptr)
        {
            return Result;
        }

        // 캐릭터 판정·피해는 ASC를 거치므로 ASC가 없는 Pawn은 치트 대상이 아니다.
        for (TActorIterator<APawn> It(World); It; ++It)
        {
            APawn* Pawn = *It;
            if (Pawn == PlayerPawn || !IsValid(Pawn))
            {
                continue;
            }
            const UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
            if (AbilitySystem == nullptr)
            {
                continue;
            }
            if (!ExcludedTags.IsEmpty() && AbilitySystem->HasAnyMatchingGameplayTags(ExcludedTags))
            {
                continue;
            }
            Result.Add(Pawn);
        }

        if (Target == EKataCheatTarget::Nearest)
        {
            if (PlayerPawn == nullptr || Result.IsEmpty())
            {
                Result.Reset();
                return Result;
            }

            const FVector Origin = PlayerPawn->GetActorLocation();
            AActor* Nearest = nullptr;
            double NearestDistanceSquared = TNumericLimits<double>::Max();
            for (AActor* Candidate : Result)
            {
                const double DistanceSquared = FVector::DistSquared(Origin, Candidate->GetActorLocation());
                if (DistanceSquared < NearestDistanceSquared)
                {
                    NearestDistanceSquared = DistanceSquared;
                    Nearest = Candidate;
                }
            }
            Result.Reset();
            Result.Add(Nearest);
        }
        return Result;
    }

    bool ParseCheatToggle(const FString& Text, bool bCurrent, bool& bOutEnabled)
    {
        const FString Trimmed = Text.TrimStartAndEnd();
        if (Trimmed.IsEmpty())
        {
            bOutEnabled = !bCurrent;
            return true;
        }
        if (Trimmed == TEXT("1") || Trimmed.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Trimmed.Equals(TEXT("on"), ESearchCase::IgnoreCase))
        {
            bOutEnabled = true;
            return true;
        }
        if (Trimmed == TEXT("0") || Trimmed.Equals(TEXT("false"), ESearchCase::IgnoreCase) || Trimmed.Equals(TEXT("off"), ESearchCase::IgnoreCase))
        {
            bOutEnabled = false;
            return true;
        }
        return false;
    }

    const TCHAR* GetCheatTargetName(EKataCheatTarget Target)
    {
        switch (Target)
        {
        case EKataCheatTarget::Self:
            return TEXT("Self");
        case EKataCheatTarget::Target:
            return TEXT("Target");
        case EKataCheatTarget::Nearest:
            return TEXT("Nearest");
        case EKataCheatTarget::All:
            return TEXT("All");
        }
        return TEXT("Unknown");
    }
}

#endif
