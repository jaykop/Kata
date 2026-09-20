// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class UKataDefinition;

/** 테스트 액터와 콘솔 명령이 공유하는 진단 출력 헬퍼. */
namespace KataTestLogging
{
    /**
     * 짧은 이름, C++ 클래스 경로, Blueprint 에셋 경로를 모두 받아 정의 클래스를 찾는다.
     * Blueprint 경로에 `_C`가 없으면 붙여서 한 번 더 시도한다.
     */
    UClass* FindDefinitionClass(const FString& ClassNameOrPath);

    /** 해석 결과의 고유 정보, 정렬된 태스크, 진단을 LogKata로 출력한다. */
    void DumpResolvedDefinition(TSubclassOf<UKataDefinition> DefinitionClass);
}
