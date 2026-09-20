// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Definition/KataDefinition.h"
#include "KataTestDefinitions.generated.h"

/**
 * 검증용 정의 계층.
 *
 * 에셋 없이 상속·오버라이드·순서·루프·진단을 확인하려고 만든 프로젝트 전용 테스트 코드다.
 * 각 항목의 Task Id를 고정 GUID로 두어 자식이 안정적으로 같은 항목을 가리킨다.
 * 자식 정의에 필요한 정보만 노출하도록 부모가 ID 접근자를 제공한다.
 */
UCLASS(meta = (DisplayName = "Kata Test: Basic Timeline"))
class PROJECTKATA_API UKataDefinition_TestBasic : public UKataDefinition
{
    GENERATED_BODY()

public:
    UKataDefinition_TestBasic();

    /** A: 0.0 ~ 1.0, Gameplay 단계. */
    static FKataTaskId TaskIdA();

    /** B: 0.5 ~ 0.8, Gameplay 단계. */
    static FKataTaskId TaskIdB();

    /** C: 0.5 ~ 1.0, Animation 단계. A/B와 같은 시각에 시작해 단계 순서를 드러낸다. */
    static FKataTaskId TaskIdC();
};

/**
 * 프로퍼티별 오버라이드와 자식의 태스크 추가를 확인한다.
 * B의 StartTime만 바꾸고 나머지 값과 부모의 다른 항목은 그대로 따른다.
 */
UCLASS(meta = (DisplayName = "Kata Test: Property Override"))
class PROJECTKATA_API UKataDefinition_TestOverride : public UKataDefinition_TestBasic
{
    GENERATED_BODY()

public:
    UKataDefinition_TestOverride();

    /** D: 자식이 추가한 순간 태스크. 1.2초, Presentation 단계. */
    static FKataTaskId TaskIdD();
};

/**
 * 완료 의존성을 확인한다.
 * C의 Dependencies만 오버라이드해 B의 완료를 기다리게 한다.
 * C는 시작 시각 0.5가 아니라 B가 끝나는 0.8에 시작해야 한다.
 */
UCLASS(meta = (DisplayName = "Kata Test: Completion Dependency"))
class PROJECTKATA_API UKataDefinition_TestDependency : public UKataDefinition_TestBasic
{
    GENERATED_BODY()

public:
    UKataDefinition_TestDependency();
};

/** 인스턴스 내부 타임라인 반복을 확인한다. 두 번 반복하고 종료한다. */
UCLASS(meta = (DisplayName = "Kata Test: Loop"))
class PROJECTKATA_API UKataDefinition_TestLoop : public UKataDefinition_TestBasic
{
    GENERATED_BODY()

public:
    UKataDefinition_TestLoop();
};

/**
 * 진단 경로를 확인한다. 실행은 거절되어야 한다.
 * C를 제거한 뒤 다시 오버라이드해 OrphanedOverride를 만들고,
 * 지원하지 않는 업데이트 시점을 요구하는 태스크를 추가해 Error를 만든다.
 */
UCLASS(meta = (DisplayName = "Kata Test: Diagnostics"))
class PROJECTKATA_API UKataDefinition_TestInvalid : public UKataDefinition_TestBasic
{
    GENERATED_BODY()

public:
    UKataDefinition_TestInvalid();
};
