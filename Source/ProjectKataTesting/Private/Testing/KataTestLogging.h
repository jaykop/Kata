// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UKataAction;

/** ProjectKataTesting의 테스트 액터와 콘솔 명령이 공유하는 진단 출력 헬퍼. */
namespace KataTestLogging
{
    /**
     * 실행할 액션을 찾는다. 내장 테스트 액션 이름을 먼저 보고, 없으면 에셋 경로로 불러온다.
     *
     * @param NameOrPath 내장 액션 이름(Basic, Override 등) 또는 Kata Action 에셋 경로.
     * @param Outer      내장 액션을 만들 때 사용할 Outer. 호출자가 결과 참조를 유지해야 GC되지 않는다.
     */
    UKataAction* FindAction(const FString& NameOrPath, UObject* Outer);

    /** 해석된 설정, 정렬된 태스크와 진단을 LogKata로 출력한다. */
    void DumpResolvedAction(UKataAction* Action);
}
