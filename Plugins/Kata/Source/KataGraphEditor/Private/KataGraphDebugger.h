#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtrTemplates.h"

class UKataEdNode;
class UKataGraphBase;
class UKataGraphComponent;
class UKataGraphInstance;
class UKataGraphNodeBase;

/** 실행 노드와 저작 페이지를 대응시키는 식별자 계산. 저장 재구성과 디버거가 함께 쓴다. */
namespace KataGraphDebugIds
{
    /**
     * 그래프 페이지의 식별자. 경로 이름에서 결정적으로 만든다.
     *
     * 저장 필드를 두지 않아 기존 에셋을 고치지 않아도 된다. 에셋이나 내장 원본의 경로가 바뀌면 값도 바뀌므로
     * 그 페이지를 쓰는 부모를 다시 저장해야 SubGraph 안쪽 강조가 다시 맞는다.
     */
    FGuid GetPageId(const UKataGraphBase* Page);

    /** 루트에서 이 페이지까지 거치는 내장 페이지 식별자. 루트 자신은 빈 배열이다. */
    TArray<FGuid> GetPagePath(const UKataGraphBase* Page);
}

/**
 * 그래프 에디터 한 개의 PIE 디버그 상태.
 *
 * 디버그 대상은 PIE 월드의 UKataGraphComponent다. 그래프를 다시 시작하면 인스턴스가 바뀌므로 컴포넌트를 들고
 * 매번 활성 인스턴스를 읽는다. 대상과 인스턴스는 약한 참조이며 PIE 종료 시 선택을 비운다.
 */
class FKataGraphDebugger
{
public:
    explicit FKataGraphDebugger(UKataGraphBase* InRootGraph);
    ~FKataGraphDebugger();

    /** PIE 월드에서 활성 인스턴스가 이 루트 그래프를 실행하는 컴포넌트를 모은다. */
    void GatherCandidates(TArray<UKataGraphComponent*>& OutComponents) const;

    /** nullptr을 지정하면 사용자가 대상 없음을 고른 것으로 보고 자동 선택하지 않는다. */
    void SetDebugTarget(UKataGraphComponent* Component);
    UKataGraphComponent* GetDebugTarget() const { return DebugTarget.Get(); }

    /** 대상 컴포넌트의 활성 인스턴스가 이 루트 그래프를 실행할 때만 반환한다. */
    UKataGraphInstance* GetDebugInstance() const;

    static FText GetComponentLabel(const UKataGraphComponent* Component);
    FText GetDebugTargetLabel() const;

    /**
     * 페이지 편집기 노드의 강조를 갱신한다. PIE가 아니거나 대상이 없으면 모두 해제한다.
     * 대상이 비었고 후보가 하나뿐이면 그 후보를 자동으로 고른다.
     */
    void Tick(UKataGraphBase* Page);

    /** 실행 노드가 저작된 페이지와 표시할 편집기 노드를 찾는다. 외장 SubGraph 안이면 그 포트를 준다. */
    bool ResolveNode(const UKataGraphNodeBase* Node, UKataGraphBase*& OutPage, UKataEdNode*& OutEdNode) const;

private:
    void HandleEndPIE(bool bIsSimulating);
    void ClearHighlights(UKataGraphBase* Page) const;

    TWeakObjectPtr<UKataGraphBase> RootGraph;
    TWeakObjectPtr<UKataGraphComponent> DebugTarget;
    /** 마지막으로 강조를 적용한 페이지. 페이지를 바꾸면 이전 페이지 노드의 강조를 지운다. */
    TWeakObjectPtr<UKataGraphBase> LastPage;
    bool bTargetClearedByUser = false;
    FDelegateHandle EndPIEHandle;
};
