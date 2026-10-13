#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "KataMVVMAuthoringToolset.generated.h"

class UWidgetBlueprint;

/** Widget Blueprint에 추가된 ViewModel 하나의 요약. */
USTRUCT(BlueprintType)
struct FKataMVVMViewModelContextInfo
{
    GENERATED_BODY()

    /** 바인딩과 SetViewModelCreationType에서 ViewModel을 가리키는 이름. */
    UPROPERTY(BlueprintReadOnly, Category = "MVVM")
    FName ViewModelName;

    UPROPERTY(BlueprintReadOnly, Category = "MVVM")
    FString ViewModelClassPath;

    /** EMVVMBlueprintViewModelContextCreationType의 열거자 이름. */
    UPROPERTY(BlueprintReadOnly, Category = "MVVM")
    FString CreationType;

    /** Resolver 인스턴스의 클래스 경로. Resolver가 없으면 빈 문자열이다. */
    UPROPERTY(BlueprintReadOnly, Category = "MVVM")
    FString ResolverClassPath;
};

/**
 * 엔진 MVVMToolset에 없는 ViewModel 생성 방식 지정을 MCP 도구로 제공한다.
 * 샘플의 HUD 위젯 저작용이며 Kata 플러그인은 이 툴셋을 참조하지 않는다. 모듈 시작 시 UToolsetRegistry에 등록한다.
 * 함수 위의 설명은 MCP 도구 설명으로 노출되므로 메타데이터처럼 영어로 쓴다.
 */
UCLASS(BlueprintType, Hidden)
class UKataMVVMAuthoringToolset : public UToolsetDefinition
{
    GENERATED_BODY()

public:
    /*
    Lists the viewmodels added to a WidgetBlueprint with their name, class, creation type and resolver class.

    @param WidgetBlueprint - WidgetBlueprint to inspect
    @return One entry per viewmodel context.
    */
    UFUNCTION(BlueprintCallable, meta = (AICallable), Category = "Kata|MVVM")
    static TArray<FKataMVVMViewModelContextInfo> ListViewModelContexts(UWidgetBlueprint* WidgetBlueprint);

    /*
    Sets how a viewmodel of a WidgetBlueprint is created at runtime. Optional and setter flags follow the engine details panel rules.

    @param WidgetBlueprint - WidgetBlueprint that owns the viewmodel
    @param ViewModelName - Viewmodel name as returned by ListViewModelContexts
    @param CreationType - One of Manual, CreateInstance, GlobalViewModelCollection, PropertyPath, Resolver, Context
    @param ResolverClass - Resolver class used when CreationType is Resolver, e.g. /Script/KataFramework.KataHUDViewModelResolver. If null, the project default resolver is used
    @return True if the creation type was applied.
    */
    UFUNCTION(BlueprintCallable, meta = (AICallable), Category = "Kata|MVVM")
    static bool SetViewModelCreationType(UWidgetBlueprint* WidgetBlueprint, FName ViewModelName, const FString& CreationType, UClass* ResolverClass);
};
