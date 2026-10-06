#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

enum class EKataGASInspectionPage : uint8
{
    Tags,
    Attributes,
    Abilities,
    Effects
};

/** 화면은 이 값만 읽으며 수집 대상 UObject의 수명을 연장하지 않는다. */
struct FKataGASInspectionRow
{
    FString Key;
    EKataGASInspectionPage Page = EKataGASInspectionPage::Tags;
    FString Name;
    FString State;
    FString Value;
    FString Tags;
    FString Source;
    FString Detail;
    FSoftObjectPath AssetPath;
    bool bActive = false;
    bool bBlocked = false;
    TArray<TSharedPtr<FKataGASInspectionRow>> Children;

    // 원본 자식 값은 보존하고 화면의 검색 결과만 따로 유지한다.
    TArray<TSharedPtr<FKataGASInspectionRow>> VisibleChildren;

    FString GetSearchText() const
    {
        return Name + TEXT(" ") + State + TEXT(" ") + Value + TEXT(" ") + Tags
            + TEXT(" ") + Source + TEXT(" ") + Detail;
    }
};

struct FKataGASInspectionSnapshot
{
    FDateTime CapturedAt;
    FString Target;
    FString TargetPath;
    bool bActorInfoReady = false;
    FString World;
    FString Readiness;
    TArray<TSharedPtr<FKataGASInspectionRow>> Rows;
};

class UAbilitySystemComponent;
class UWorld;

struct FKataGASInspectionWorld
{
    FName Context;
    FString Label;
    TWeakObjectPtr<UWorld> World;
};

struct FKataGASInspectionTarget
{
    FString Label;
    TWeakObjectPtr<UAbilitySystemComponent> ASC;
};
