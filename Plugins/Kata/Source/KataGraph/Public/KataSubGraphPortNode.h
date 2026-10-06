#pragma once

#include "CoreMinimal.h"
#include "KataNode.h"
#include "KataSubGraphPortNode.generated.h"

class UKataGraph;

/**
 * 포트가 선택한 내장 서브그래프 원본. 소유는 UKataGraph::EmbeddedSubGraphs가 담당한다.
 *
 * 에디터의 단일 선택 콤보를 위한 구조체다. 이 참조에 Instanced를 붙이면 포트 복제 시
 * 같은 원본을 공유하지 않고 원본까지 복제할 수 있으므로 비소유 참조로 유지한다.
 */
USTRUCT()
struct KATAGRAPH_API FKataEmbeddedSubGraphReference
{
    GENERATED_BODY()

#if WITH_EDITORONLY_DATA
    /** 포트 소속 그래프가 직접 소유한 내장 원본. 비어 있으면 아직 선택하지 않은 상태다. */
    UPROPERTY(EditAnywhere, Category = "Kata")
    TObjectPtr<UKataGraph> Graph = nullptr;
#endif
};

/**
 * 다른 그래프를 이 그래프에서 사용하기 위한 포트 노드. 액션을 실행하지 않는다.
 *
 * 같은 서브그래프를 가리키는 포트를 여러 개 놓을 수 있다. 진입 지점은 그것을 쓰는 노드 옆에,
 * 이탈 지점은 목적지 옆에 두어 그래프를 가로지르는 긴 엣지를 없애려는 것이다. 포트가 몇 개든
 * 저장할 때 펼쳐지는 서브그래프 사본은 하나이고 모든 포트가 그 사본으로 이어진다.
 *
 * 포트는 그래프 편집과 서브그래프 펼침에만 쓰인다. 펼치고 나면 사라지므로 실행 중에는 존재하지 않는다.
 */
UCLASS(Abstract, BlueprintType)
class KATAGRAPH_API UKataSubGraphPortNode : public UKataNode
{
    GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
    /** 켜면 내장 원본을 선택한다. 기본값은 외장이므로 기존 에셋의 참조는 유지된다. */
    UPROPERTY(EditAnywhere, Category = "Kata",
        meta = (DisplayName = "Use Embedded SubGraph", ToolTip = "Select an embedded subgraph owned by this graph instead of an external graph asset."))
    bool bUseEmbeddedSubGraph = false;

    /** 내장 모드의 선택. 모드를 바꾸면 비활성 쪽 참조는 비운다. */
    UPROPERTY(EditAnywhere, Category = "Kata",
        meta = (EditCondition = "bUseEmbeddedSubGraph", EditConditionHides,
            ToolTip = "Embedded subgraph reference. The owning graph manages the source object."))
    FKataEmbeddedSubGraphReference EmbeddedSubGraph;
#endif

    /** 외장 모드에서 가리키는 그래프 에셋. 기존 프로퍼티 이름과 직렬화 참조를 유지한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata",
        meta = (EditCondition = "!bUseEmbeddedSubGraph", EditConditionHides,
            ToolTip = "Graph asset expanded into this graph on save. Ports sharing this asset share one expanded copy."))
    TObjectPtr<UKataGraph> SubGraph;

#if WITH_EDITOR
    /**
     * 활성 모드가 가리키는 저작 원본을 반환한다. 선택이 없거나 참조가 잘못되면 nullptr이다.
     *
     * 내장은 소속 그래프의 직접 소유 목록과 Outer를 모두 확인한다. 외장은 그래프 에셋이어야 하고
     * 자기 또는 조상 그래프를 가리킬 수 없다. 값을 고치거나 로그를 남기지 않으며 간접 순환은 검사하지 않는다.
     * 반환한 원본의 수명은 소유 그래프 또는 외장 에셋 참조가 관리한다.
     */
    UKataGraph* GetReferencedSubGraph() const;

    /**
     * 제목은 언제나 가리키는 서브그래프를 따른다.
     *
     * 직접 입력한 제목을 저장하면 서브그래프를 바꿔도
     * 이전 이름이 남는다. 제목 편집을 시작했다가 종료하기만 해도 저장될 수 있으므로
     * 사용자가 제목을 바꿀 의도가 없어도 이름이 저장될 수 있다. UKataActionNode가 액션 에셋에 대해 택한 방식과 같다.
     */
    virtual FText GetNodeTitle() const override { return GetDescription(); }
    virtual bool IsNameEditable() const override { return false; }

    /**
     * 선택 모드 변경 시 비활성 참조를 비우고, 직접 참조 계약을 어긴 선택을 해제한다.
     *
     * 내장은 소속 그래프의 직접 소유 목록에 있어야 한다. 외장 선택의 자기·조상 참조는 재귀적인
     * 의존이므로 거부한다. 사용자 프로퍼티 편집의 트랜잭션 안에서 처리하며 간접 순환 검사는 빌드 단계가 담당한다.
     */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

/**
 * 서브그래프의 진입 포트.
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
 * 서브그래프의 이탈 포트.
 *
 * 이 노드에서 나가는 전이는 서브그래프 안 어느 노드에서든 출발한 전이로 처리한다. 펼칠 때 서브그래프
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
