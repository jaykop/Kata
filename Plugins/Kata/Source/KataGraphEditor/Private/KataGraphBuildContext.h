#pragma once

#include "CoreMinimal.h"

class UKataGraphBase;

/** 저작 참조를 검사한 뒤 내장 원본부터 루트 순서로 실행 데이터를 재구성한다. */
struct FKataGraphBuildContext
{
    /**
     * 참조·외장 최신성 오류가 있으면 재구성 전에 false를 반환하고 기존 실행 데이터를 유지한다.
     * bForSave가 true일 때만 루트의 새 세대와 의존 기록을 발급한다. 외장 원본은 수정하지 않는다.
     */
    static bool Rebuild(UKataGraphBase* Root, bool bForSave = false);

    /**
     * 현재 외장 의존과 저장 기준을 비교한다. 객체를 수정하거나 실행 데이터를 재구성하지 않는다.
     * bOutNeedsSave는 원본을 사용해 부모를 갱신할 수 있고 저장 기준이 낡았을 때만 true다.
     * 미저장 원본·참조 오류 등 갱신을 막는 경고는 표시만 반환하며 부모 저장을 반복 요구하지 않는다.
     */
    static FText GetDependencyStatus(UKataGraphBase* Root, bool& bOutNeedsSave);
};
