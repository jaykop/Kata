#pragma once

#include "CoreMinimal.h"
#include "KataNode.h"
#include "KataSubGraphPortNode.generated.h"

class UKataGraph;

/**
 * 다른 그래프를 이 그래프에 끌어다 쓰기 위한 문. 액션을 실행하지 않는다.
 *
 * 같은 서브그래프를 가리키는 포트를 여러 개 놓을 수 있다. 진입 지점은 그것을 쓰는 노드 옆에,
 * 이탈 지점은 목적지 옆에 두어 그래프를 가로지르는 긴 엣지를 없애려는 것이다. 포트가 몇 개든
 * 저장할 때 펼쳐지는 서브그래프 사본은 한 벌이고 모든 포트가 그 사본으로 이어진다.
 *
 * 포트는 저작과 펼침에만 쓰인다. 펼치고 나면 사라지므로 실행 중에는 존재하지 않는다.
 */
UCLASS(Abstract, BlueprintType)
class KATAGRAPH_API UKataSubGraphPortNode : public UKataNode
{
    GENERATED_BODY()

public:
    /** 이 포트가 가리키는 서브그래프. 비워 두면 펼칠 때 무시한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata",
        meta = (ToolTip = "Graph asset expanded into this graph on save. Ports sharing this asset share one expanded copy."))
    TObjectPtr<UKataGraph> SubGraph;

#if WITH_EDITOR
    /**
     * 제목은 언제나 가리키는 서브그래프를 따른다.
     *
     * 직접 지은 이름을 허용하면 한 번 커밋된 뒤로 제목이 그 자리에 굳어, 서브그래프를 바꿔도
     * 옛 이름이 남는다. 노드를 클릭해 편집 상태로 들어갔다 빠져나오기만 해도 커밋이 일어나므로
     * 저작자가 의도하지 않아도 쉽게 굳는다. UKataActionNode가 액션 에셋에 대해 택한 방식과 같다.
     */
    virtual FText GetNodeTitle() const override { return GetDescription(); }
    virtual bool IsNameEditable() const override { return false; }

    /**
     * 자기가 속한 그래프를 가리키면 되돌린다.
     *
     * 자기 참조를 펼치면 끝없이 자기를 복제하므로 받아들일 수 없다. 오브젝트 피커가 같은 종류의
     * 에셋을 모두 보여 주어 자기 자신도 고를 수 있으므로, 고른 직후에 알려 주고 비운다.
     * 펼침 단계도 같은 경우를 막지만 그때는 저장할 때까지 아무 일도 일어나지 않아 원인을 알기 어렵다.
     */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

/**
 * 서브그래프로 들어가는 문.
 *
 * 이 노드로 들어온 전이는 서브그래프의 진입 노드로 이어진다. 진입 노드는 머무를 수 없으므로
 * 기존 전이 해석이 그대로 통과해 서브그래프 안의 액션까지 내려간다.
 *
 * 목적지가 서브그래프 안이라 이 그래프에서 나가는 엣지를 그릴 곳이 없다. 그래서 나가는 연결을 막는다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata SubGraph Port In"))
class KATAGRAPH_API UKataSubGraphPortInNode : public UKataSubGraphPortNode
{
    GENERATED_BODY()

public:
    UKataSubGraphPortInNode();

    virtual FText GetDescription_Implementation() const override;
};

/**
 * 서브그래프에서 나오는 문.
 *
 * 이 노드에서 나가는 전이는 서브그래프 안 어느 노드에서든 출발한 것으로 친다. 펼칠 때 서브그래프
 * 사본을 출발지로 갖는 별칭으로 바뀐다. 피격이나 사망처럼 도중에 가로채는 전이가 여기에 해당하고,
 * 트리거를 비운 자동 전이를 걸면 서브그래프가 정상적으로 끝났을 때 돌아가는 경로가 된다.
 *
 * 출발지이지 목적지가 아니므로 들어오는 연결을 막는다. 서브그래프로 들어갈 때는 Port In을 쓴다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata SubGraph Port Out"))
class KATAGRAPH_API UKataSubGraphPortOutNode : public UKataSubGraphPortNode
{
    GENERATED_BODY()

public:
    UKataSubGraphPortOutNode();

    /** 펼치면 서브그래프의 머무를 수 있는 노드들로 바뀌므로 별칭의 출발지가 될 수 있다. */
    virtual bool CanBeAliasSource() const override { return true; }

    virtual FText GetDescription_Implementation() const override;
};
