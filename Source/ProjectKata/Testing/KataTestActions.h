// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "KataTestActions.generated.h"

class UKataAction;

/** 코드로 만드는 검증용 액션의 종류. None이면 에셋 참조를 그대로 쓴다. */
UENUM(BlueprintType)
enum class EKataTestAction : uint8
{
    /** 코드 하네스를 쓰지 않는다. */
    None,
    /** A·B·C 세 태스크로 시작·종료 경계와 단계 순서를 확인한다. */
    Basic,
    /** 프로퍼티별 오버라이드와 자식의 태스크 추가를 확인한다. */
    Override,
    /** 완료 의존성으로 시작 시각이 밀리는지 확인한다. */
    Dependency,
    /** 인스턴스 내부 타임라인 반복을 확인한다. */
    Loop,
    /** 진단 경로를 확인한다. 실행은 거절되어야 한다. */
    Invalid
};

/**
 * 에셋 없이 상속·오버라이드·순서·루프·진단을 확인하려고 코드로 액션 트리를 만드는 팩토리.
 *
 * 클래스 상속이 아니라 ParentAction 객체 체인으로 부모·자식 관계를 구성한다.
 * 각 항목의 Task Id를 고정 GUID로 두어 자식이 안정적으로 같은 항목을 가리킨다.
 * 프로젝트 전용 테스트 코드이며 플러그인에 포함하지 않는다.
 */
namespace KataTestActions
{
    /**
     * 선택한 종류의 액션과 그 부모 체인을 만든다.
     *
     * @param Kind  만들 액션의 종류. None이면 nullptr를 반환한다.
     * @param Outer 만들어진 액션들의 Outer. 호출자가 참조를 들고 있어야 GC되지 않는다.
     */
    UKataAction* Make(EKataTestAction Kind, UObject* Outer);

    /** A: 0.0 ~ 1.0, Gameplay 단계. */
    FKataTaskId TaskIdA();

    /** B: 0.5 ~ 0.8, Gameplay 단계. */
    FKataTaskId TaskIdB();

    /** C: 0.5 ~ 1.0, Animation 단계. A/B와 같은 시각에 시작해 단계 순서를 드러낸다. */
    FKataTaskId TaskIdC();

    /** D: Override가 추가한 순간 태스크. 1.2초, Presentation 단계. */
    FKataTaskId TaskIdD();

    /** 콘솔 명령에서 받은 이름을 종류로 바꾼다. 알 수 없으면 None을 반환한다. */
    EKataTestAction ParseName(const FString& Name);

    /** 로그와 콘솔 도움말에 쓰는 표시 이름. */
    FString GetName(EKataTestAction Kind);

    /** None을 뺀 전체 종류. 콘솔 목록 출력에 쓴다. */
    TArray<EKataTestAction> GetAllKinds();
}
